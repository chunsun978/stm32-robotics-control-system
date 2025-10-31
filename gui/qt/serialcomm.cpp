/**
 * @file serialcomm.cpp
 * @brief Serial Communication Implementation
 */

#include "serialcomm.h"

/**
 * @brief Constructor
 */
SerialComm::SerialComm(QObject *parent)
    : QObject(parent)
#ifdef QT_SERIALPORT_LIB
    , serial(new QSerialPort(this))
#else
    , serial(nullptr)
#endif
    , m_isConnected(false)
{
#ifdef QT_SERIALPORT_LIB
    connect(serial, &QSerialPort::readyRead, this, &SerialComm::onReadyRead);
    connect(serial, &QSerialPort::errorOccurred, this, &SerialComm::onErrorOccurred);
#endif
}

/**
 * @brief Destructor
 */
SerialComm::~SerialComm()
{
#ifdef QT_SERIALPORT_LIB
    if (serial && serial->isOpen()) {
        serial->close();
    }
#endif
}

/**
 * @brief Connect to serial port
 * @param portName Port name (e.g., "COM5", "/dev/ttyACM0")
 * @param baudRate Baud rate (e.g., 115200)
 * @return True if connection successful
 */
bool SerialComm::connect(const QString &portName, int baudRate)
{
#ifdef QT_SERIALPORT_LIB
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);
    
    if (serial->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
        emit connected();
        return true;
    } else {
        m_isConnected = false;
        return false;
    }
#else
    Q_UNUSED(portName);
    Q_UNUSED(baudRate);
    emit error("Qt SerialPort module not installed. Cannot connect.");
    return false;
#endif
}

/**
 * @brief Disconnect from serial port
 */
void SerialComm::disconnect()
{
#ifdef QT_SERIALPORT_LIB
    if (serial && serial->isOpen()) {
        serial->close();
    }
#endif
    m_isConnected = false;
    buffer.clear();
    emit disconnected();
}

/**
 * @brief Check if connected
 */
bool SerialComm::isConnected() const
{
#ifdef QT_SERIALPORT_LIB
    return m_isConnected && serial && serial->isOpen();
#else
    return false;
#endif
}

/**
 * @brief Send command to serial port
 * @param cmd Command string
 */
void SerialComm::sendCommand(const QString &cmd)
{
#ifdef QT_SERIALPORT_LIB
    if (!isConnected()) return;
    
    QByteArray data = cmd.toUtf8() + "\r\n";
    serial->write(data);
#else
    Q_UNUSED(cmd);
#endif
}

/**
 * @brief Handle incoming data
 */
void SerialComm::onReadyRead()
{
#ifdef QT_SERIALPORT_LIB
    buffer.append(serial->readAll());
    
    // Process complete lines (terminated with \n)
    int newlineIndex;
    while ((newlineIndex = buffer.indexOf('\n')) != -1) {
        QByteArray line = buffer.left(newlineIndex);
        buffer.remove(0, newlineIndex + 1);
        
        // Remove carriage return if present
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        
        if (!line.isEmpty()) {
            emit dataReceived(line);
        }
    }
#endif
}

#ifdef QT_SERIALPORT_LIB
/**
 * @brief Handle serial port errors
 */
void SerialComm::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    
    QString errorMsg;
    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorMsg = "Device not found";
            break;
        case QSerialPort::PermissionError:
            errorMsg = "Permission denied";
            break;
        case QSerialPort::OpenError:
            errorMsg = "Failed to open port";
            break;
        case QSerialPort::WriteError:
            errorMsg = "Write error";
            break;
        case QSerialPort::ReadError:
            errorMsg = "Read error";
            break;
        case QSerialPort::ResourceError:
            errorMsg = "Resource error (device disconnected?)";
            disconnect();
            break;
        default:
            errorMsg = "Unknown error";
    }
    
    emit this->error(errorMsg);
}
#endif

