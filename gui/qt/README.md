# Motor Control GUI - Qt Application

Professional motor control interface with real-time S-curve visualization, PID tuning, and performance analysis.

![Build Status](https://img.shields.io/badge/Status-Phase_1-yellow)
![Qt Version](https://img.shields.io/badge/Qt-6.5+-blue)
![C++](https://img.shields.io/badge/C++-17-blue)

---

## Features

- ✅ Real-time position and velocity plotting
- ✅ PID gain tuning interface
- ✅ Serial communication (QSerialPort)
- ✅ Mock data generator for testing without hardware
- ✅ Command logging console
- ✅ Run recording and playback
- 🔄 Performance metrics (coming in Phase 5)
- 🔄 Multi-run comparison (coming in Phase 5)

---

## Requirements

### Software

- **Qt 6.5+** (Qt 5.15+ should also work)
- **Qt Creator** (recommended) or command-line build tools
- **C++17 compiler**
  - Windows: MSVC 2019+ or MinGW
  - Linux: GCC 7+ or Clang 5+
  - macOS: Xcode 10+

### Qt Modules Required

- Qt Core
- Qt Widgets
- Qt GUI
- Qt SerialPort
- Qt PrintSupport (for QCustomPlot)

### Third-Party Libraries

- **QCustomPlot** - High-performance plotting widget
  - Download: [https://www.qcustomplot.com/](https://www.qcustomplot.com/)
  - Version: 2.1.1 or later

---

## Setup Instructions

### 1. Install Qt

**Windows:**
```bash
# Download Qt Online Installer from qt.io
# Install Qt 6.5+ with MSVC 2019 or MinGW component
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install qt6-base-dev qt6-serialport-dev libgl1-mesa-dev
```

**macOS:**
```bash
brew install qt@6
```

### 2. Download QCustomPlot

1. Go to [https://www.qcustomplot.com/index.php/download](https://www.qcustomplot.com/index.php/download)
2. Download **QCustomPlot-source** (latest version)
3. Extract `qcustomplot.h` and `qcustomplot.cpp`
4. Copy both files to this directory (`gui/qt/`)

**Your directory should now have:**
```
gui/qt/
├── qcustomplot.h          ← (downloaded)
├── qcustomplot.cpp        ← (downloaded)
├── MotorControlGUI.pro
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── mainwindow.ui
├── serialcomm.h
├── serialcomm.cpp
├── mockdatagenerator.h
└── mockdatagenerator.cpp
```

### 3. Build the Project

**Option A: Qt Creator (Recommended)**

1. Open Qt Creator
2. File → Open File or Project
3. Select `MotorControlGUI.pro`
4. Configure project (select Qt 6 kit)
5. Build → Build Project (Ctrl+B)
6. Run → Run (Ctrl+R)

**Option B: Command Line (qmake)**

```bash
cd gui/qt
mkdir build
cd build

# Configure
qmake ../MotorControlGUI.pro

# Build
make         # Linux/macOS
nmake        # Windows (MSVC)
mingw32-make # Windows (MinGW)

# Run
./MotorControlGUI        # Linux/macOS
MotorControlGUI.exe      # Windows
```

**Option C: Command Line (CMake) - TODO**

CMake support coming soon.

---

## Usage

### Testing Without Hardware

The GUI starts in **Mock Data Mode** by default:

1. Launch the application
2. Click **Plan Motion** (uses default parameters)
3. Click **▶ Start**
4. Watch real-time S-curve plots!

**Mock data includes:**
- Realistic S-curve trajectory
- Simulated tracking errors (~1-3 steps)
- PID output simulation
- All 3 motion phases (accel, const, decel)

### Connecting to STM32

1. Flash the firmware to your STM32F411
2. Connect via USB
3. In GUI, select the correct port (e.g., `COM5`, `/dev/ttyACM0`)
4. Select baud rate: **115200**
5. Click **Connect**

**Expected console output:**
```
[14:32:15] === Motor Control GUI v0.1.0 ===
[14:32:15] System initialized. Using mock data for testing.
[14:32:20] Connected to COM5 at 115200 baud
[14:32:20] > GET_VERSION
[14:32:20] < VERSION,0,1,0,1
```

---

## GUI Layout

```
┌──────────────────────────────────────────────────────────┐
│  Motor Control System v0.1.0             [_] [□] [X]     │
├──────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌───────────────────────────────────┐  │
│  │ Connection  │  │                                   │  │
│  │ [COM5    ▼] │  │   Position Tracking (S-curve)     │  │
│  │ [115200  ▼] │  │   Yellow = Target, Blue = Actual  │  │
│  │ [Connect]   │  │                                   │  │
│  ├─────────────┤  └───────────────────────────────────┘  │
│  │ Motion      │  ┌───────────────────────────────────┐  │
│  │ Steps: 1000 │  │   Velocity Profile                │  │
│  │ Vel:   500  │  │   Smooth S-curve ramp             │  │
│  │ Acc:   1000 │  │                                   │  │
│  │[Plan Motion]│  └───────────────────────────────────┘  │
│  ├─────────────┤  ┌───────────────────────────────────┐  │
│  │ PID Tuning  │  │ Current:                          │  │
│  │ P: 1.5      │  │ Pos: 523.5  Vel: 45.2  Err: 2.1   │  │
│  │ I: 0.1      │  └───────────────────────────────────┘  │
│  │ D: 0.05     │  ┌───────────────────────────────────┐  │
│  │ F: 0.8      │  │ Console Log                       │  │
│  │[Apply Gains]│  │ > MOVE 1000 500 1000              │  │
│  ├─────────────┤  │ < OK                              │  │
│  │ Control     │  │ > START                           │  │
│  │ [▶ Start]  │   └-──────────────────────────────────┘  │
│  │ [■ Stop]    │                                         │
│  │ [⏹ E-STOP] │                                        │
│  └─────────────┘                                         │
└──────────────────────────────────────────────────────────┘
```

---

## Protocol

Communication with STM32 uses simple ASCII protocol:

### Commands (GUI → STM32)

```
MOVE <steps> <vel> <acc>   - Plan and execute motion
START                      - Start planned motion
STOP                       - Stop motion
ESTOP                      - Emergency stop
HOME                       - Run homing sequence
SET_P <value>              - Set proportional gain
SET_I <value>              - Set integral gain
SET_D <value>              - Set derivative gain
SET_F <value>              - Set feedforward gain
GET_VERSION                - Query firmware version
GET_STATUS                 - Query current state
```

### Responses (STM32 → GUI)

```
OK                                              - Command accepted
ERROR                                           - Command failed
DATA,<time>,<tgt_pos>,<act_pos>,<tgt_vel>,...  - Real-time telemetry
VERSION,<major>,<minor>,<patch>,<protocol>      - Version info
STATUS,<state>,<pos>,<vel>,<error>,<kp>,...    - Current state
```

---

## Project Structure

```
gui/qt/
├── MotorControlGUI.pro        - Qt project file
├── main.cpp                   - Application entry point
├── mainwindow.h/.cpp/.ui      - Main window (UI + logic)
├── serialcomm.h/.cpp          - Serial port communication
├── mockdatagenerator.h/.cpp   - Mock data for testing
├── qcustomplot.h/.cpp         - Plotting library (download)
└── README.md                  - This file
```

---

## Development Roadmap

### Phase 1: GUI Skeleton ✅ (Current)
- [x] Qt project setup
- [x] Main window layout
- [x] QCustomPlot integration
- [x] Mock data generator
- [x] Real-time plotting

### Phase 2: Serial Communication 🔄 (Next)
- [ ] STM32 command parser
- [ ] Protocol testing
- [ ] Error handling

### Phase 3: Telemetry Integration
- [ ] Live data from STM32
- [ ] High-speed plotting
- [ ] Buffer management

### Phase 4: PID Control
- [ ] Encoder feedback
- [ ] Closed-loop control
- [ ] Real-time tuning

### Phase 5: Analysis
- [ ] Run recording
- [ ] Performance metrics
- [ ] Multi-run comparison

---

## Troubleshooting

### Build Errors

**"Cannot find qcustomplot.h"**
- Download QCustomPlot and copy files to `gui/qt/`

**"undefined reference to QSerialPort"**
- Install Qt SerialPort module: `sudo apt install qt6-serialport-dev`

**"C++17 required"**
- Check your compiler supports C++17 or add to .pro file: `CONFIG += c++17`

### Runtime Issues

**"Cannot open serial port"**
- Check device is connected
- Check correct port name
- On Linux, add user to dialout group: `sudo usermod -a -G dialout $USER`
- On Windows, check Device Manager for COM port

**"No data in plots"**
- GUI starts in mock mode - should work immediately
- Check console for error messages
- Try reconnecting or restarting GUI

---

## Contributing

This is part of the STM32 Robotics Control System project.

**Main Project**: [stm32-robotics-control-system](https://github.com/chunsun978/stm32-robotics-control-system)

---

## License

MIT License - See main project for details.

---

## Next Steps

1. ✅ Build and run GUI with mock data
2. ⏭️ Test all UI controls
3. ⏭️ Flash STM32 firmware
4. ⏭️ Connect and test with real hardware
5. ⏭️ Start Phase 2 (Serial Protocol)

**Happy coding!** 🚀

