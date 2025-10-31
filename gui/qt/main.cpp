/**
 * @file main.cpp
 * @brief Motor Control GUI - Main Entry Point
 * 
 * Professional motor control interface for STM32 robotics system.
 * Features real-time S-curve visualization, PID tuning, and performance analysis.
 * 
 * @author STM32 Robotics Control System
 * @version 0.1.0
 * @date 2025-10-31
 */

#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    QCoreApplication::setOrganizationName("STM32 Robotics");
    QCoreApplication::setApplicationName("Motor Control GUI");
    QCoreApplication::setApplicationVersion("0.1.0");
    
    // Create and show main window
    MainWindow window;
    window.setWindowTitle("Motor Control System - v0.1.0");
    window.show();
    
    return app.exec();
}

