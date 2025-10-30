#include "motor/MotionPlanner.hpp"
#include <cmath>

MotionPlanner::MotionPlanner()
    : state_(State::IDLE)
    , current_position_(0.0f)
    , current_velocity_(0.0f)
    , target_position_(0.0f)
    , start_time_ms_(0)
    , update_freq_hz_(1000)
    , dt_(0.001f)
    , htim_(nullptr)
    , speed_callback_(nullptr)
    , direction_callback_(nullptr)
{
}

void MotionPlanner::init(TIM_HandleTypeDef* htim, uint32_t update_freq_hz) {
    htim_ = htim;
    update_freq_hz_ = update_freq_hz;
    dt_ = 1.0f / static_cast<float>(update_freq_hz);
    state_ = State::IDLE;
}

bool MotionPlanner::moveTo(float target_steps, float max_velocity, 
                           float max_acceleration, float max_jerk) {
    if (state_ == State::RUNNING) {
        return false;  // Already running
    }
    
    // Calculate relative move from current position
    float distance = target_steps - current_position_;
    bool forward = distance >= 0;
    float abs_distance = std::fabs(distance);
    
    if (abs_distance < 0.1f) {
        // Already at target
        state_ = State::COMPLETED;
        return true;
    }
    
    // Set direction
    if (direction_callback_) {
        direction_callback_(forward);
    }
    
    // Setup S-curve profile
    SCurveProfile::Config config;
    config.max_velocity = max_velocity;
    config.max_acceleration = max_acceleration;
    config.max_jerk = max_jerk;
    config.start_velocity = 0.0f;  // Start from rest
    
    if (!profile_.calculate(abs_distance, config)) {
        state_ = State::ERROR;
        return false;
    }
    
    // Start motion
    target_position_ = target_steps;
    start_time_ms_ = HAL_GetTick();
    state_ = State::RUNNING;
    
    return true;
}

void MotionPlanner::stop() {
    if (speed_callback_) {
        speed_callback_(0.0f);
    }
    state_ = State::IDLE;
    current_velocity_ = 0.0f;
}

MotionPlanner::Status MotionPlanner::getStatus() const {
    Status status;
    status.state = state_;
    status.current_position = current_position_;
    status.current_velocity = current_velocity_;
    status.target_position = target_position_;
    
    if (state_ == State::RUNNING && profile_.isValid()) {
        float elapsed = (HAL_GetTick() - start_time_ms_) / 1000.0f;
        status.progress = elapsed / profile_.getTotalTime();
        if (status.progress > 1.0f) status.progress = 1.0f;
    } else {
        status.progress = (state_ == State::COMPLETED) ? 1.0f : 0.0f;
    }
    
    return status;
}

void MotionPlanner::update() {
    if (state_ != State::RUNNING) {
        return;
    }
    
    // Calculate elapsed time
    uint32_t now = HAL_GetTick();
    float elapsed_sec = (now - start_time_ms_) / 1000.0f;
    
    // Get state from S-curve profile
    SCurveProfile::State profile_state = profile_.getStateAtTime(elapsed_sec);
    
    if (profile_state.is_complete) {
        // Motion complete
        current_position_ = target_position_;
        current_velocity_ = 0.0f;
        updateMotorSpeed(0.0f);
        state_ = State::COMPLETED;
        return;
    }
    
    // Update current state
    bool forward = (target_position_ >= current_position_);
    
    if (forward) {
        current_position_ = target_position_ - profile_.getTotalTime() * profile_state.velocity 
                          + elapsed_sec * profile_state.velocity;
    } else {
        current_position_ = target_position_ + profile_.getTotalTime() * profile_state.velocity 
                          - elapsed_sec * profile_state.velocity;
    }
    
    current_velocity_ = profile_state.velocity;
    
    // Update motor speed
    updateMotorSpeed(profile_state.velocity);
}

void MotionPlanner::updateMotorSpeed(float velocity) {
    if (speed_callback_) {
        speed_callback_(std::fabs(velocity));
    }
}

void MotionPlanner::setSpeedCallback(void (*callback)(float speed)) {
    speed_callback_ = callback;
}

void MotionPlanner::setDirectionCallback(void (*callback)(bool forward)) {
    direction_callback_ = callback;
}

