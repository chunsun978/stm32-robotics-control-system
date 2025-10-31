/**
 * @file mainwindow.h
 * @brief Main Window Class Definition
 * 
 * Provides the main GUI interface for motor control system.
 * Handles real-time plotting, serial communication, and user interactions.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>
#include "qcustomplot.h"
#include "serialcomm.h"

// Forward declaration to avoid circular dependency
class MockDataGenerator;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief Telemetry data point structure
 * 
 * Represents a single data point from the motor control system.
 */
struct TelemetryPoint {
    float time_ms;              ///< Time in milliseconds
    float target_position;      ///< Planned S-curve position
    float actual_position;      ///< Actual encoder position
    float target_velocity;      ///< Planned S-curve velocity
    float actual_velocity;      ///< Measured velocity
    float acceleration;         ///< Acceleration value (steps/sec²)
    float pid_output;           ///< PID controller output (-100 to 100%)
    uint8_t phase;              ///< Motion phase (0=idle, 1=accel, 2=const, 3=decel)
};

/**
 * @brief PID gains structure
 */
struct PIDGains {
    float kp = 1.0f;            ///< Proportional gain
    float ki = 0.1f;            ///< Integral gain
    float kd = 0.05f;           ///< Derivative gain
    float kf = 0.8f;            ///< Feedforward gain
};

/**
 * @brief Motion parameters structure
 */
struct MotionParams {
    float steps = 1000.0f;          ///< Target distance in steps
    float max_velocity = 500.0f;    ///< Maximum velocity (steps/sec)
    float acceleration = 1000.0f;   ///< Acceleration (steps/sec²)
};

/**
 * @brief Main Window Class
 * 
 * Central widget for the motor control GUI.
 * Manages all UI elements, plotting, and communication.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Connection controls
    void on_connectButton_clicked();
    void on_disconnectButton_clicked();
    
    // Motion controls
    void on_planMotionButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_estopButton_clicked();
    void on_homeButton_clicked();
    
    // PID tuning
    void on_applyGainsButton_clicked();
    
    // Recording
    void on_recordButton_toggled(bool checked);
    void on_saveRunButton_clicked();
    void on_loadRunButton_clicked();
    
    // Internal slots
    void updatePlots();
    void onSerialDataReceived(const QByteArray &data);
    void generateMockData();

private:
    Ui::MainWindow *ui;
    
    // Plotting
    QCustomPlot *positionPlot;
    QCustomPlot *velocityPlot;
    void setupPlots();
    void clearPlots();
    
    // Serial communication
    SerialComm *serial;
    bool isConnected;
    
    // Mock data generator (for testing without hardware)
    MockDataGenerator *mockGen;
    QTimer *mockTimer;
    bool isRecording;
    bool useMockData;
    
    // Data storage
    QVector<TelemetryPoint> liveData;
    
    // Parameters
    PIDGains pidGains;
    MotionParams motionParams;
    
    // Helper functions
    void sendCommand(const QString &cmd);
    void parseTelemetry(const QString &line);
    void logMessage(const QString &msg);
    void updateStatusBar();
};

#endif // MAINWINDOW_H

