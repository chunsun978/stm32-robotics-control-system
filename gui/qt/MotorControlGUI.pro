#
# Motor Control GUI - Qt Project File
# Professional motor control interface with real-time S-curve visualization
#

QT       += core gui widgets printsupport
# serialport temporarily disabled - install Qt SerialPort module to enable
# QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Suppress deprecation warnings from QCustomPlot
QMAKE_CXXFLAGS += -Wno-deprecated-declarations


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    # qcustomplot.cpp (pre-compiled as library)
    serialcomm.cpp \
    mockdatagenerator.cpp

HEADERS += \
    mainwindow.h \
    qcustomplot.h \
    serialcomm.h \
    mockdatagenerator.h

# Pre-compiled QCustomPlot library (safe mode)
LIBS += -L$$PWD/qcustomplot_obj -lqcustomplot

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Application info
TARGET = MotorControlGUI
VERSION = 0.1.0

# Windows icon (disabled until icon file is created)
# win32:RC_ICONS = icon.ico

# macOS bundle
# macx {
#     QMAKE_INFO_PLIST = Info.plist
#     ICON = icon.icns
# }


