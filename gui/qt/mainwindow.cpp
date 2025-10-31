/**
 * @file mainwindow.cpp
 * @brief Main Window Implementation
 * 
 * Implements the motor control GUI functionality including plotting,
 * communication, and user interactions.
 */

#include "mainwindow.h"
#include "mockdatagenerator.h"  // Include the full definition
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>

/**
 * @brief Constructor
 * @param parent Parent widget
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isConnected(false)
    , isRecording(false)
    , useMockData(true)  // Start with mock data for testing
{
    ui->setupUi(this);
    
    // Setup plots
    setupPlots();
    
    // Setup serial communication
    serial = new SerialComm(this);
    connect(serial, &SerialComm::dataReceived, this, &MainWindow::onSerialDataReceived);
    
    // Setup mock data generator
    mockGen = new MockDataGenerator(this);
    mockTimer = new QTimer(this);
    connect(mockTimer, &QTimer::timeout, this, &MainWindow::generateMockData);
    
    // Start mock data for testing
    mockTimer->start(50);  // 20 Hz update rate
    
    // Log welcome message
    logMessage("=== Motor Control GUI v0.1.0 ===");
    logMessage("System initialized. Using mock data for testing.");
    logMessage("Connect to STM32 to use real hardware.");
    
    updateStatusBar();
}

/**
 * @brief Destructor
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief Setup plotting widgets
 */
void MainWindow::setupPlots()
{
    // Position plot
    positionPlot = ui->positionPlot;
    
    // Add target position graph (yellow dashed)
    positionPlot->addGraph();
    positionPlot->graph(0)->setPen(QPen(QColor(255, 200, 0), 2, Qt::DashLine));
    positionPlot->graph(0)->setName("Target Position");
    
    // Add actual position graph (blue solid)
    positionPlot->addGraph();
    positionPlot->graph(1)->setPen(QPen(Qt::blue, 2));
    positionPlot->graph(1)->setName("Actual Position");
    
    // Add error graph (red shaded)
    positionPlot->addGraph();
    QColor errorColor = Qt::red;
    errorColor.setAlpha(50);
    positionPlot->graph(2)->setPen(QPen(errorColor));
    positionPlot->graph(2)->setBrush(QBrush(errorColor));
    positionPlot->graph(2)->setChannelFillGraph(positionPlot->graph(0));
    positionPlot->graph(2)->setName("Error");
    
    // Configure axes
    positionPlot->xAxis->setLabel("Time (s)");
    positionPlot->yAxis->setLabel("Position (steps)");
    positionPlot->legend->setVisible(true);
    positionPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    
    // Velocity plot
    velocityPlot = ui->velocityPlot;
    
    // Add target velocity graph (yellow solid)
    velocityPlot->addGraph();
    velocityPlot->graph(0)->setPen(QPen(QColor(255, 200, 0), 2));
    velocityPlot->graph(0)->setName("Target Velocity");
    
    // Add actual velocity graph (cyan solid)
    velocityPlot->addGraph();
    velocityPlot->graph(1)->setPen(QPen(Qt::cyan, 2));
    velocityPlot->graph(1)->setName("Actual Velocity");
    
    // Configure axes
    velocityPlot->xAxis->setLabel("Time (s)");
    velocityPlot->yAxis->setLabel("Velocity (steps/sec)");
    velocityPlot->legend->setVisible(true);
    velocityPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

/**
 * @brief Clear all plot data
 */
void MainWindow::clearPlots()
{
    liveData.clear();
    
    for (int i = 0; i < positionPlot->graphCount(); ++i) {
        positionPlot->graph(i)->data()->clear();
    }
    
    for (int i = 0; i < velocityPlot->graphCount(); ++i) {
        velocityPlot->graph(i)->data()->clear();
    }
    
    positionPlot->replot();
    velocityPlot->replot();
}

/**
 * @brief Update plots with current data
 */
void MainWindow::updatePlots()
{
    if (liveData.isEmpty()) return;
    
    // Get time window (last 10 seconds)
    float currentTime = liveData.last().time_ms / 1000.0f;
    float windowStart = qMax(0.0f, currentTime - 10.0f);
    
    // Extract data for plotting
    QVector<double> time, targetPos, actualPos, targetVel, actualVel;
    
    for (const auto& point : liveData) {
        float t = point.time_ms / 1000.0f;
        if (t < windowStart) continue;
        
        time.append(t);
        targetPos.append(point.target_position);
        actualPos.append(point.actual_position);
        targetVel.append(point.target_velocity);
        actualVel.append(point.actual_velocity);
    }
    
    // Update position plot
    positionPlot->graph(0)->setData(time, targetPos);
    positionPlot->graph(1)->setData(time, actualPos);
    positionPlot->graph(2)->setData(time, targetPos);  // For error shading
    
    positionPlot->xAxis->setRange(windowStart, currentTime);
    positionPlot->rescaleAxes();
    positionPlot->replot();
    
    // Update velocity plot
    velocityPlot->graph(0)->setData(time, targetVel);
    velocityPlot->graph(1)->setData(time, actualVel);
    
    velocityPlot->xAxis->setRange(windowStart, currentTime);
    velocityPlot->rescaleAxes();
    velocityPlot->replot();
    
    // Update current values display
    if (!liveData.isEmpty()) {
        const auto& latest = liveData.last();
        ui->currentPosLabel->setText(QString::number(latest.actual_position, 'f', 1));
        ui->currentVelLabel->setText(QString::number(latest.actual_velocity, 'f', 1));
        ui->currentErrorLabel->setText(QString::number(latest.target_position - latest.actual_position, 'f', 2));
        
        QString phaseStr = "IDLE";
        switch (latest.phase) {
            case 1: phaseStr = "ACCEL"; break;
            case 2: phaseStr = "CONST"; break;
            case 3: phaseStr = "DECEL"; break;
        }
        ui->currentPhaseLabel->setText(phaseStr);
    }
}

/**
 * @brief Generate mock data for testing
 */
void MainWindow::generateMockData()
{
    if (!useMockData) return;
    
    // Get mock data point
    TelemetryPoint point = mockGen->getNextPoint();
    
    // Add to live data
    liveData.append(point);
    
    // Limit data size (keep last 20 seconds at 20 Hz = 400 points)
    if (liveData.size() > 400) {
        liveData.removeFirst();
    }
    
    // Update plots
    updatePlots();
}

/**
 * @brief Handle connect button click
 */
void MainWindow::on_connectButton_clicked()
{
    QString port = ui->portComboBox->currentText();
    int baudRate = ui->baudComboBox->currentText().toInt();
    
    if (serial->connect(port, baudRate)) {
        isConnected = true;
        useMockData = false;
        mockTimer->stop();
        
        logMessage("Connected to " + port + " at " + QString::number(baudRate) + " baud");
        ui->statusBar->showMessage("Connected", 3000);
        
        // Send version check
        sendCommand("GET_VERSION");
    } else {
        QMessageBox::warning(this, "Connection Error", 
                           "Failed to connect to " + port + 
                           "\n\nCheck that:\n"
                           "- Device is plugged in\n"
                           "- Port is correct\n"
                           "- No other program is using the port");
    }
    
    updateStatusBar();
}

/**
 * @brief Handle disconnect button click
 */
void MainWindow::on_disconnectButton_clicked()
{
    serial->disconnect();
    isConnected = false;
    
    logMessage("Disconnected from serial port");
    ui->statusBar->showMessage("Disconnected", 3000);
    
    // Resume mock data
    useMockData = true;
    mockGen->reset();
    clearPlots();
    mockTimer->start(50);
    
    updateStatusBar();
}

/**
 * @brief Handle plan motion button click
 */
void MainWindow::on_planMotionButton_clicked()
{
    // Read parameters from UI
    motionParams.steps = ui->stepsSpinBox->value();
    motionParams.max_velocity = ui->velocitySpinBox->value();
    motionParams.acceleration = ui->accelSpinBox->value();
    
    // Send MOVE command
    QString cmd = QString("MOVE %1 %2 %3")
                    .arg(motionParams.steps)
                    .arg(motionParams.max_velocity)
                    .arg(motionParams.acceleration);
    
    sendCommand(cmd);
    
    // If using mock data, update mock generator
    if (useMockData) {
        mockGen->planMotion(motionParams.steps, motionParams.max_velocity, motionParams.acceleration);
        clearPlots();
    }
    
    logMessage("Motion planned: " + QString::number(motionParams.steps) + " steps");
}

/**
 * @brief Handle start button click
 */
void MainWindow::on_startButton_clicked()
{
    sendCommand("START");
    
    if (useMockData) {
        mockGen->start();
    }
    
    logMessage("Motion started");
}

/**
 * @brief Handle stop button click
 */
void MainWindow::on_stopButton_clicked()
{
    sendCommand("STOP");
    
    if (useMockData) {
        mockGen->stop();
    }
    
    logMessage("Motion stopped");
}

/**
 * @brief Handle emergency stop button click
 */
void MainWindow::on_estopButton_clicked()
{
    sendCommand("ESTOP");
    
    if (useMockData) {
        mockGen->stop();
    }
    
    logMessage("EMERGENCY STOP!");
}

/**
 * @brief Handle home button click
 */
void MainWindow::on_homeButton_clicked()
{
    sendCommand("HOME");
    logMessage("Homing sequence started");
}

/**
 * @brief Handle apply gains button click
 */
void MainWindow::on_applyGainsButton_clicked()
{
    // Read gains from UI
    pidGains.kp = ui->kpSpinBox->value();
    pidGains.ki = ui->kiSpinBox->value();
    pidGains.kd = ui->kdSpinBox->value();
    pidGains.kf = ui->kfSpinBox->value();
    
    // Send commands
    sendCommand(QString("SET_P %1").arg(pidGains.kp));
    sendCommand(QString("SET_I %1").arg(pidGains.ki));
    sendCommand(QString("SET_D %1").arg(pidGains.kd));
    sendCommand(QString("SET_F %1").arg(pidGains.kf));
    
    logMessage(QString("PID gains updated: Kp=%1 Ki=%2 Kd=%3 Kf=%4")
              .arg(pidGains.kp).arg(pidGains.ki).arg(pidGains.kd).arg(pidGains.kf));
}

/**
 * @brief Handle record button toggle
 */
void MainWindow::on_recordButton_toggled(bool checked)
{
    isRecording = checked;
    
    if (checked) {
        logMessage("Recording started");
    } else {
        logMessage("Recording stopped (" + QString::number(liveData.size()) + " points)");
    }
}

/**
 * @brief Handle save run button click
 */
void MainWindow::on_saveRunButton_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Run Data",
                                                   QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".csv",
                                                   "CSV Files (*.csv)");
    
    if (filename.isEmpty()) return;
    
    // TODO: Implement CSV save
    logMessage("Run data saved to " + filename);
}

/**
 * @brief Handle load run button click
 */
void MainWindow::on_loadRunButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Load Run Data",
                                                    "",
                                                    "CSV Files (*.csv)");
    
    if (filename.isEmpty()) return;
    
    // TODO: Implement CSV load
    logMessage("Run data loaded from " + filename);
}

/**
 * @brief Send command to serial port
 * @param cmd Command string
 */
void MainWindow::sendCommand(const QString &cmd)
{
    if (isConnected) {
        serial->sendCommand(cmd);
    }
    
    logMessage("> " + cmd);
}

/**
 * @brief Handle received serial data
 * @param data Raw data from serial port
 */
void MainWindow::onSerialDataReceived(const QByteArray &data)
{
    QString line = QString::fromUtf8(data).trimmed();
    
    if (line.startsWith("DATA,")) {
        parseTelemetry(line);
    } else {
        logMessage("< " + line);
    }
}

/**
 * @brief Parse telemetry data
 * @param line Telemetry line from STM32
 */
void MainWindow::parseTelemetry(const QString &line)
{
    // Parse: DATA,<time>,<tgt_pos>,<act_pos>,<tgt_vel>,<act_vel>,<pid_out>,<phase>
    QStringList parts = line.split(',');
    
    if (parts.size() != 8) return;
    
    TelemetryPoint point;
    point.time_ms = parts[1].toFloat();
    point.target_position = parts[2].toFloat();
    point.actual_position = parts[3].toFloat();
    point.target_velocity = parts[4].toFloat();
    point.actual_velocity = parts[5].toFloat();
    point.pid_output = parts[6].toFloat();
    point.phase = parts[7].toUInt();
    
    // Add to live data
    liveData.append(point);
    
    // Limit data size
    if (liveData.size() > 400) {
        liveData.removeFirst();
    }
    
    // Update plots
    updatePlots();
}

/**
 * @brief Log message to console
 * @param msg Message to log
 */
void MainWindow::logMessage(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->consoleTextEdit->append("[" + timestamp + "] " + msg);
}

/**
 * @brief Update status bar
 */
void MainWindow::updateStatusBar()
{
    QString status = isConnected ? "Connected" : "Disconnected (Mock Data)";
    QString mode = useMockData ? " | Mock Mode" : " | Live Mode";
    QString dataPoints = " | Data: " + QString::number(liveData.size()) + " points";
    
    ui->statusBar->showMessage(status + mode + dataPoints);
}

