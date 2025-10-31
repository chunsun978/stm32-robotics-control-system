/**
 * @file serialcomm.h
 * @brief Serial Communication Class
 * 
 * Handles serial port communication with STM32.
 */

#ifndef SERIALCOMM_H
#define SERIALCOMM_H

#include <QObject>
#include <QByteArray>

// Temporarily disable serial port if module not installed
#ifdef QT_SERIALPORT_LIB
#include <QSerialPort>
#endif

/**
 * @brief Serial Communication Class
 * 
 * Manages serial port connection and data transfer.
 */
class SerialComm : public QObject
{
    Q_OBJECT

public:
    explicit SerialComm(QObject *parent = nullptr);
    ~SerialComm();
    
    bool connect(const QString &portName, int baudRate);
    void disconnect();
    bool isConnected() const;
    
    void sendCommand(const QString &cmd);

signals:
    void dataReceived(const QByteArray &data);
    void connected();
    void disconnected();
    void error(const QString &errorMsg);

private slots:
    void onReadyRead();
#ifdef QT_SERIALPORT_LIB
    void onErrorOccurred(QSerialPort::SerialPortError error);
#endif

private:
#ifdef QT_SERIALPORT_LIB
    QSerialPort *serial;
#else
    void *serial;  // Placeholder when SerialPort not available
#endif
    QByteArray buffer;
    bool m_isConnected;
};

#endif // SERIALCOMM_H

