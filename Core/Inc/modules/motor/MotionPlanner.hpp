#ifndef INC_MODULES_MOTOR_MOTIONPLANNER_HPP_
#define INC_MODULES_MOTOR_MOTIONPLANNER_HPP_

#include "motor/SCurveProfile.hpp"
#include "stm32f4xx_hal.h"
#include <cstdint>

/**
 * @brief Real-time motion planner with S-curve profiles
 * 
 * Executes S-curve motion profiles in real-time using timer interrupts.
 * Updates motor speed dynamically to follow the calculated trajectory.
 */
class MotionPlanner {
public:
    enum class State {
        IDLE,
        RUNNING,
        COMPLETED,
        ERROR
    };
    
    struct Status {
        State state;
        float current_position;
        float current_velocity;
        float target_position;
        float progress;  // 0.0 to 1.0
    };

    MotionPlanner();
    
    /**
     * @brief Initialize motion planner
     * @param htim Timer handle for position updates
     * @param update_freq_hz Update frequency (Hz), typically 1000-10000
     */
    void init(TIM_HandleTypeDef* htim, uint32_t update_freq_hz);
    
    /**
     * @brief Start a motion to target position
     * @param target_steps Target position in steps
     * @param max_velocity Maximum velocity (steps/sec)
     * @param max_acceleration Maximum acceleration (steps/sec²)
     * @param max_jerk Maximum jerk (steps/sec³)
     * @return true if motion started successfully
     */
    bool moveTo(float target_steps, float max_velocity, 
                float max_acceleration, float max_jerk);
    
    /**
     * @brief Stop current motion
     */
    void stop();
    
    /**
     * @brief Get current status
     */
    Status getStatus() const;
    
    /**
     * @brief Update function - call from timer interrupt
     * Should be called at fixed intervals (e.g., 1kHz)
     */
    void update();
    
    /**
     * @brief Set motor control callbacks
     */
    void setSpeedCallback(void (*callback)(float speed));
    void setDirectionCallback(void (*callback)(bool forward));
    
    /**
     * @brief Check if motion is complete
     */
    bool isComplete() const { return state_ == State::COMPLETED || state_ == State::IDLE; }
    
    /**
     * @brief Reset position to zero
     */
    void resetPosition() { current_position_ = 0.0f; }

private:
    SCurveProfile profile_;
    State state_;
    
    float current_position_;
    float current_velocity_;
    float target_position_;
    
    uint32_t start_time_ms_;
    uint32_t update_freq_hz_;
    float dt_;  // Time step (seconds)
    
    TIM_HandleTypeDef* htim_;
    
    // Callbacks for motor control
    void (*speed_callback_)(float speed);
    void (*direction_callback_)(bool forward);
    
    void updateMotorSpeed(float velocity);
};

#endif /* INC_MODULES_MOTOR_MOTIONPLANNER_HPP_ */

