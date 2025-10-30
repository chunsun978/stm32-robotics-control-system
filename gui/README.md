# GUI Directory

This directory contains multiple GUI options for controlling the STM32 robotics system.

## GUI Options

### 1. Web GUI (`web/`)
**Technology**: HTML/CSS/JavaScript + WebSerial API  
**Best For**: Cross-platform, no installation required  
**Features**:
- Browser-based interface
- WebSerial API for direct USB connection
- Real-time motor control
- Position/velocity plotting
- Mobile-friendly responsive design

**Status**: 🚧 Planned

---

### 2. Python GUI (`python/`)
**Technology**: Python + PyQt5/Tkinter + PySerial  
**Best For**: Rapid development, data logging, testing  
**Features**:
- Native desktop application
- Serial communication via PySerial
- Data logging to CSV
- Real-time plotting (matplotlib)
- Easy to extend/modify

**Status**: 🚧 Planned

---

### 3. Qt C++ GUI (`qt/`)
**Technology**: Qt 6 C++ + QSerialPort  
**Best For**: Professional desktop application, performance  
**Features**:
- Native C++ performance
- Qt Designer for UI
- Cross-platform (Windows/Linux/Mac)
- Advanced visualizations
- DLL/shared library support

**Status**: 🚧 Planned

---

## Communication Protocol

All GUIs will communicate with STM32 via UART (115200 baud):

### Command Format
```
COMMAND <param1> <param2> ...
```

### Commands
- `MOVE <steps>` - Move relative
- `GOTO <position>` - Move absolute
- `VEL <steps_per_sec>` - Set max velocity
- `ACCEL <steps_per_sec2>` - Set acceleration
- `JERK <steps_per_sec3>` - Set jerk
- `STOP` - Emergency stop
- `ENABLE` / `DISABLE` - Motor enable
- `STATUS` - Get current state
- `HOME` - Homing sequence

### Response Format
```
OK: <message>
ERROR: <message>
STATUS: pos=<pos> vel=<vel> state=<state>
```

## Getting Started

Choose your preferred GUI technology and see the README in that subdirectory for setup instructions.

## Future Features

- [ ] Multi-motor coordination
- [ ] Trajectory recording/playback
- [ ] G-code interpreter
- [ ] PID tuning interface
- [ ] Real-time plotting
- [ ] Configuration save/load
- [ ] Firmware update via GUI

