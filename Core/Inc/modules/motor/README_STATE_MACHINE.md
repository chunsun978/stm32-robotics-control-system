# Motor Control State Machine

## Overview

The `MotorStateMachine` class manages all motor control states and ensures safe, predictable state transitions. This prevents invalid operations and makes debugging easier.

## State Diagram

```
UNINITIALIZED
     |
     | INITIALIZE
     v
   IDLE ←──────────────┐
     |                 |
     | ENABLE          | DISABLE
     v                 |
   READY ←─────────────┴─────────────┐
     |                               |
     | START_MOTION                  |
     v                               |
ACCELERATING ──────────────────┐     |
     |                         |     |
     | MOTION_COMPLETE         |     |
     v                         |     |
  RUNNING ────────────────┐    |     |
     |                    |    |     |
     | MOTION_COMPLETE    |    |     |
     v                    |    |     |
DECELERATING ─────────────┘    |     |
     |                         |     |
     | MOTION_COMPLETE         |     |
     └─────────────────────────┴─────┘

  STOPPING ──────────── ERROR
     |                    |
     | MOTION_COMPLETE    | ERROR_CLEARED
     v                    v
   READY                IDLE

  HOMING
     |
     | HOME_COMPLETE
     v
   READY
```

## States

### UNINITIALIZED
**Initial state** - System not ready
- **Allowed Transitions**: IDLE (on INITIALIZE)
- **Motor State**: Disabled
- **Use**: Boot/startup phase

### IDLE
**Motor disabled**, system ready
- **Allowed Transitions**: READY (on ENABLE)
- **Motor State**: Disabled
- **Use**: Safe state when motor not needed

### READY
**Motor enabled**, waiting for command
- **Allowed Transitions**: 
  - ACCELERATING (on START_MOTION)
  - HOMING (on HOME_COMMAND)
  - IDLE (on DISABLE)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, stopped
- **Use**: Ready to accept motion commands

### ACCELERATING
Speed ramping up (S-curve acceleration phase)
- **Allowed Transitions**:
  - RUNNING (on MOTION_COMPLETE)
  - DECELERATING (on STOP)
  - STOPPING (on EMERGENCY_STOP)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, accelerating
- **Use**: During speed ramp-up

### RUNNING
Constant velocity cruise
- **Allowed Transitions**:
  - DECELERATING (on MOTION_COMPLETE or STOP)
  - STOPPING (on EMERGENCY_STOP)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, constant speed
- **Use**: Steady-state motion

### DECELERATING
Speed ramping down (S-curve deceleration phase)
- **Allowed Transitions**:
  - READY (on MOTION_COMPLETE)
  - STOPPING (on EMERGENCY_STOP)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, decelerating
- **Use**: During speed ramp-down

### STOPPING
Emergency stop in progress
- **Allowed Transitions**:
  - READY (on MOTION_COMPLETE)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, rapid deceleration
- **Use**: E-stop response

### HOMING
Homing sequence active
- **Allowed Transitions**:
  - READY (on HOME_COMPLETE)
  - STOPPING (on EMERGENCY_STOP)
  - ERROR (on ERROR_DETECTED)
- **Motor State**: Enabled, seeking home position
- **Use**: Finding reference position

### ERROR
Fault condition detected
- **Allowed Transitions**: IDLE (on ERROR_CLEARED)
- **Motor State**: Disabled
- **Use**: Safety state on faults

## Events

| Event | Description | Triggers From |
|-------|-------------|---------------|
| `INITIALIZE` | System initialization complete | Boot sequence |
| `ENABLE` | Enable motor driver | User command |
| `DISABLE` | Disable motor driver | User command |
| `START_MOTION` | Begin motion profile | Motion command |
| `MOTION_COMPLETE` | Motion finished | Profile executor |
| `STOP` | Commanded stop | User command |
| `EMERGENCY_STOP` | E-stop triggered | Safety system |
| `ERROR_DETECTED` | Fault detected | Error handler |
| `ERROR_CLEARED` | Fault cleared | User reset |
| `HOME_COMMAND` | Start homing | User command |
| `HOME_COMPLETE` | Homing finished | Homing sequence |

## Usage Examples

### Basic Initialization

```cpp
#include "motor/MotorStateMachine.hpp"

MotorStateMachine sm;

// Initialize system
sm.processEvent(MotorStateMachine::Event::INITIALIZE);

// Enable motor
sm.processEvent(MotorStateMachine::Event::ENABLE);

// Now ready for motion commands
if (sm.canMove()) {
    sm.processEvent(MotorStateMachine::Event::START_MOTION);
}
```

### With Callbacks

```cpp
// Set transition callback
sm.setTransitionCallback(
    [](State from, State to, Event event) {
        printf("State change: %s -> %s (%s)\r\n",
               MotorStateMachine::getStateName(from),
               MotorStateMachine::getStateName(to),
               MotorStateMachine::getEventName(event));
    }
);

// Set state entry callback
sm.setStateEntryCallback(
    [](State state) {
        if (state == State::ERROR) {
            // Handle error entry
            motor.stop();
            led.setRed();
        }
    }
);
```

### Motion Control Integration

```cpp
// Start motion
if (sm.canMove()) {
    sm.processEvent(Event::START_MOTION);
    
    // Execute S-curve profile
    while (!profile.isComplete()) {
        float velocity = profile.getVelocity();
        motor.setStepRate(velocity);
        
        // Track phase transitions
        if (profile.reachedCruise()) {
            sm.processEvent(Event::MOTION_COMPLETE);  // ACCELERATING -> RUNNING
        }
    }
    
    // Motion done
    sm.processEvent(Event::MOTION_COMPLETE);  // -> DECELERATING
    motor.stop();
    sm.processEvent(Event::MOTION_COMPLETE);  // -> READY
}
```

### Error Handling

```cpp
// Detect error
if (position_error > threshold) {
    sm.processEvent(Event::ERROR_DETECTED);
}

// Check state
if (sm.isError()) {
    printf("System in error state!\r\n");
    // Fix issue...
    sm.processEvent(Event::ERROR_CLEARED);
}
```

## Safety Features

1. **Invalid Transition Prevention**
   - State machine rejects invalid transitions
   - Logs warnings for debugging

2. **State Queries**
   ```cpp
   bool canMove = sm.canMove();        // Can accept motion commands?
   bool moving = sm.isMoving();         // Currently in motion?
   bool error = sm.isError();           // In error state?
   ```

3. **State History**
   ```cpp
   State current = sm.getState();
   State previous = sm.getPreviousState();
   ```

4. **Reset Capability**
   ```cpp
   sm.reset();  // Return to IDLE, clear errors
   ```

## Benefits

### 1. **Predictable Behavior**
- All state transitions are explicit
- No ambiguous states
- Easy to reason about system state

### 2. **Debugging**
- State transitions logged automatically
- Easy to trace issues
- Clear state names for debugging

### 3. **Safety**
- Prevents invalid operations
- Forces proper initialization
- Error states are explicit

### 4. **Extensibility**
- Easy to add new states
- Simple to add new transitions
- Callbacks for custom behavior

### 5. **GUI Integration**
- State can be displayed in GUI
- Buttons enabled/disabled based on state
- Visual feedback of system status

## Integration with Other Components

### With StepperMotor
```cpp
if (sm.getState() == State::READY) {
    motor.setEnabled(true);
}
if (sm.isMoving()) {
    motor.setStepRate(velocity);
}
```

### With MotionPlanner
```cpp
planner.setStateCallback([&](MotionPlanner::State state) {
    if (state == MotionPlanner::State::Accelerating) {
        sm.processEvent(Event::START_MOTION);
    }
});
```

### With GUI
```cpp
// Update GUI based on state
gui.updateState(MotorStateMachine::getStateName(sm.getState()));

// Enable/disable buttons
gui.setMoveButtonEnabled(sm.canMove());
gui.setStopButtonEnabled(sm.isMoving());
```

## Future Enhancements

- [ ] State timeout detection
- [ ] State duration tracking
- [ ] State transition counters
- [ ] Persistent state logging
- [ ] State machine visualization
- [ ] Hierarchical states
- [ ] Parallel state machines (multi-motor)

## Files

- `MotorStateMachine.hpp` - Class definition
- `MotorStateMachine.cpp` - Implementation
- `motor_control.cpp` - Integration example

## References

- UML State Machine Design Pattern
- Embedded State Machine Best Practices
- Real-Time Systems State Management

