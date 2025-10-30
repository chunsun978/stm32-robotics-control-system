#ifndef INC_MODULES_MOTOR_MOTORSTATEMACHINE_HPP_
#define INC_MODULES_MOTOR_MOTORSTATEMACHINE_HPP_

#include <cstdint>
#include <functional>

/**
 * @brief Motor control state machine
 * 
 * Manages motor states and transitions with proper event handling.
 * Ensures safe state transitions and provides hooks for state entry/exit.
 */
class MotorStateMachine {
public:
    /**
     * @brief Motor control states
     */
    enum class State {
        UNINITIALIZED,  // System not ready
        IDLE,           // Motor disabled, ready to enable
        READY,          // Motor enabled, waiting for command
        ACCELERATING,   // Ramping up speed
        RUNNING,        // Constant velocity
        DECELERATING,   // Ramping down speed
        STOPPING,       // Emergency stop in progress
        ERROR,          // Fault condition
        HOMING          // Homing sequence active
    };
    
    /**
     * @brief Events that trigger state transitions
     */
    enum class Event {
        INITIALIZE,     // System initialization complete
        ENABLE,         // Enable motor driver
        DISABLE,        // Disable motor driver
        START_MOTION,   // Begin motion profile
        MOTION_COMPLETE,// Motion finished
        STOP,           // Commanded stop
        EMERGENCY_STOP, // E-stop triggered
        ERROR_DETECTED, // Fault detected
        ERROR_CLEARED,  // Fault cleared
        HOME_COMMAND,   // Start homing
        HOME_COMPLETE   // Homing finished
    };
    
    /**
     * @brief State transition callback
     * Parameters: (from_state, to_state, event)
     */
    using TransitionCallback = std::function<void(State, State, Event)>;
    
    /**
     * @brief State entry/exit callback
     * Parameter: (current_state)
     */
    using StateCallback = std::function<void(State)>;

    MotorStateMachine();
    
    /**
     * @brief Process an event and potentially transition state
     * @param event Event to process
     * @return true if state changed
     */
    bool processEvent(Event event);
    
    /**
     * @brief Get current state
     */
    State getState() const { return current_state_; }
    
    /**
     * @brief Get previous state
     */
    State getPreviousState() const { return previous_state_; }
    
    /**
     * @brief Check if in a specific state
     */
    bool isState(State state) const { return current_state_ == state; }
    
    /**
     * @brief Check if motor can accept motion commands
     */
    bool canMove() const;
    
    /**
     * @brief Check if motor is in motion
     */
    bool isMoving() const;
    
    /**
     * @brief Check if error state
     */
    bool isError() const { return current_state_ == State::ERROR; }
    
    /**
     * @brief Reset to idle state (clears errors)
     */
    void reset();
    
    /**
     * @brief Set callback for state transitions
     */
    void setTransitionCallback(TransitionCallback callback) {
        transition_callback_ = callback;
    }
    
    /**
     * @brief Set callback for state entry
     */
    void setStateEntryCallback(StateCallback callback) {
        state_entry_callback_ = callback;
    }
    
    /**
     * @brief Set callback for state exit
     */
    void setStateExitCallback(StateCallback callback) {
        state_exit_callback_ = callback;
    }
    
    /**
     * @brief Get state name as string (for debugging)
     */
    static const char* getStateName(State state);
    
    /**
     * @brief Get event name as string (for debugging)
     */
    static const char* getEventName(Event event);

private:
    State current_state_;
    State previous_state_;
    
    TransitionCallback transition_callback_;
    StateCallback state_entry_callback_;
    StateCallback state_exit_callback_;
    
    /**
     * @brief Check if transition is valid
     */
    bool isTransitionValid(State from, State to, Event event) const;
    
    /**
     * @brief Execute state transition
     */
    void transitionTo(State new_state, Event event);
};

#endif /* INC_MODULES_MOTOR_MOTORSTATEMACHINE_HPP_ */

