#include "motor/MotorStateMachine.hpp"
#include <stdio.h>

MotorStateMachine::MotorStateMachine()
    : current_state_(State::UNINITIALIZED)
    , previous_state_(State::UNINITIALIZED)
    , transition_callback_(nullptr)
    , state_entry_callback_(nullptr)
    , state_exit_callback_(nullptr)
{
}

bool MotorStateMachine::processEvent(Event event) {
    State next_state = current_state_;
    
    // State transition logic
    switch (current_state_) {
        case State::UNINITIALIZED:
            if (event == Event::INITIALIZE) {
                next_state = State::IDLE;
            }
            break;
            
        case State::IDLE:
            if (event == Event::ENABLE) {
                next_state = State::READY;
            }
            break;
            
        case State::READY:
            if (event == Event::DISABLE) {
                next_state = State::IDLE;
            } else if (event == Event::START_MOTION) {
                next_state = State::ACCELERATING;
            } else if (event == Event::HOME_COMMAND) {
                next_state = State::HOMING;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::ACCELERATING:
            if (event == Event::MOTION_COMPLETE) {
                next_state = State::RUNNING;
            } else if (event == Event::STOP) {
                next_state = State::DECELERATING;
            } else if (event == Event::EMERGENCY_STOP) {
                next_state = State::STOPPING;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::RUNNING:
            if (event == Event::MOTION_COMPLETE) {
                next_state = State::DECELERATING;
            } else if (event == Event::STOP) {
                next_state = State::DECELERATING;
            } else if (event == Event::EMERGENCY_STOP) {
                next_state = State::STOPPING;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::DECELERATING:
            if (event == Event::MOTION_COMPLETE) {
                next_state = State::READY;
            } else if (event == Event::EMERGENCY_STOP) {
                next_state = State::STOPPING;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::STOPPING:
            if (event == Event::MOTION_COMPLETE) {
                next_state = State::READY;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::HOMING:
            if (event == Event::HOME_COMPLETE) {
                next_state = State::READY;
            } else if (event == Event::EMERGENCY_STOP) {
                next_state = State::STOPPING;
            } else if (event == Event::ERROR_DETECTED) {
                next_state = State::ERROR;
            }
            break;
            
        case State::ERROR:
            if (event == Event::ERROR_CLEARED) {
                next_state = State::IDLE;
            }
            break;
    }
    
    // Execute transition if state changed
    if (next_state != current_state_) {
        if (isTransitionValid(current_state_, next_state, event)) {
            transitionTo(next_state, event);
            return true;
        } else {
            printf("WARNING: Invalid transition from %s to %s on event %s\r\n",
                   getStateName(current_state_),
                   getStateName(next_state),
                   getEventName(event));
            return false;
        }
    }
    
    return false;
}

bool MotorStateMachine::canMove() const {
    return current_state_ == State::READY;
}

bool MotorStateMachine::isMoving() const {
    return current_state_ == State::ACCELERATING ||
           current_state_ == State::RUNNING ||
           current_state_ == State::DECELERATING;
}

void MotorStateMachine::reset() {
    transitionTo(State::IDLE, Event::ERROR_CLEARED);
}

bool MotorStateMachine::isTransitionValid(State from, State to, Event event) const {
    // All transitions defined in processEvent() are valid
    // This function can add additional validation logic if needed
    (void)from;
    (void)to;
    (void)event;
    return true;
}

void MotorStateMachine::transitionTo(State new_state, Event event) {
    State old_state = current_state_;
    
    // Call exit callback for old state
    if (state_exit_callback_) {
        state_exit_callback_(old_state);
    }
    
    // Update state
    previous_state_ = current_state_;
    current_state_ = new_state;
    
    // Call transition callback
    if (transition_callback_) {
        transition_callback_(old_state, new_state, event);
    }
    
    // Call entry callback for new state
    if (state_entry_callback_) {
        state_entry_callback_(new_state);
    }
    
    printf("STATE: %s -> %s (event: %s)\r\n",
           getStateName(old_state),
           getStateName(new_state),
           getEventName(event));
}

const char* MotorStateMachine::getStateName(State state) {
    switch (state) {
        case State::UNINITIALIZED:  return "UNINITIALIZED";
        case State::IDLE:           return "IDLE";
        case State::READY:          return "READY";
        case State::ACCELERATING:   return "ACCELERATING";
        case State::RUNNING:        return "RUNNING";
        case State::DECELERATING:   return "DECELERATING";
        case State::STOPPING:       return "STOPPING";
        case State::ERROR:          return "ERROR";
        case State::HOMING:         return "HOMING";
        default:                    return "UNKNOWN";
    }
}

const char* MotorStateMachine::getEventName(Event event) {
    switch (event) {
        case Event::INITIALIZE:      return "INITIALIZE";
        case Event::ENABLE:          return "ENABLE";
        case Event::DISABLE:         return "DISABLE";
        case Event::START_MOTION:    return "START_MOTION";
        case Event::MOTION_COMPLETE: return "MOTION_COMPLETE";
        case Event::STOP:            return "STOP";
        case Event::EMERGENCY_STOP:  return "EMERGENCY_STOP";
        case Event::ERROR_DETECTED:  return "ERROR_DETECTED";
        case Event::ERROR_CLEARED:   return "ERROR_CLEARED";
        case Event::HOME_COMMAND:    return "HOME_COMMAND";
        case Event::HOME_COMPLETE:   return "HOME_COMPLETE";
        default:                     return "UNKNOWN";
    }
}

