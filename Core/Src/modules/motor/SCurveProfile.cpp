#include "motor/SCurveProfile.hpp"
#include <algorithm>

SCurveProfile::SCurveProfile() 
    : target_pos_(0.0f)
    , v_max_(0.0f)
    , a_max_(0.0f)
    , j_max_(0.0f)
    , v_start_(0.0f)
    , total_time_(0.0f)
    , is_valid_(false)
{
    for (int i = 0; i < 8; i++) {
        t_[i] = 0.0f;
    }
}

bool SCurveProfile::calculate(float target_position, const Config& config) {
    // Store parameters
    target_pos_ = target_position;
    v_max_ = config.max_velocity;
    a_max_ = config.max_acceleration;
    j_max_ = config.max_jerk;
    v_start_ = config.start_velocity;
    
    // Validate inputs
    if (target_pos_ <= 0 || v_max_ <= 0 || a_max_ <= 0 || j_max_ <= 0) {
        is_valid_ = false;
        return false;
    }
    
    // Simplified 7-phase S-curve calculation
    // Assumes symmetric acceleration and deceleration
    
    // Time to reach max acceleration from zero (phase 1)
    float t_jerk_accel = a_max_ / j_max_;
    
    // Time at constant acceleration (phase 2)
    float t_const_accel = (v_max_ - v_start_) / a_max_ - t_jerk_accel;
    if (t_const_accel < 0) {
        t_const_accel = 0;
        // Recalculate with reduced acceleration
        t_jerk_accel = std::sqrt((v_max_ - v_start_) / j_max_);
    }
    
    // Time to reduce acceleration to zero (phase 3)
    float t_jerk_decel_accel = t_jerk_accel;
    
    // Distance covered during acceleration phases
    float s_accel = v_start_ * (t_jerk_accel + t_const_accel + t_jerk_decel_accel)
                  + 0.5f * j_max_ * std::pow(t_jerk_accel, 3)
                  + a_max_ * t_const_accel * (t_jerk_accel + 0.5f * t_const_accel)
                  + 0.5f * a_max_ * t_jerk_decel_accel * (2.0f * t_jerk_accel + t_const_accel + t_jerk_decel_accel);
    
    // Deceleration phases (symmetric to acceleration)
    float t_jerk_accel_decel = t_jerk_accel;
    float t_const_decel = t_const_accel;
    float t_jerk_decel = t_jerk_accel;
    
    float s_decel = s_accel;  // Symmetric
    
    // Distance at constant velocity
    float s_const_vel = target_pos_ - s_accel - s_decel;
    
    // Time at constant velocity
    float t_const_vel = 0.0f;
    if (s_const_vel > 0) {
        t_const_vel = s_const_vel / v_max_;
    } else {
        // Cannot reach max velocity - need to recalculate with lower peak velocity
        // Simplified: use trapezoidal approximation
        float v_peak = std::sqrt(target_pos_ * a_max_ + 0.5f * v_start_ * v_start_);
        v_peak = std::min(v_peak, v_max_);
        
        t_jerk_accel = std::sqrt(v_peak / j_max_);
        t_const_accel = 0.0f;
        t_jerk_decel_accel = t_jerk_accel;
        
        t_const_vel = 0.0f;
        
        t_jerk_accel_decel = t_jerk_accel;
        t_const_decel = t_const_accel;
        t_jerk_decel = t_jerk_accel;
    }
    
    // Set phase timings
    t_[0] = 0.0f;
    t_[1] = t_[0] + t_jerk_accel;
    t_[2] = t_[1] + t_const_accel;
    t_[3] = t_[2] + t_jerk_decel_accel;
    t_[4] = t_[3] + t_const_vel;
    t_[5] = t_[4] + t_jerk_accel_decel;
    t_[6] = t_[5] + t_const_decel;
    t_[7] = t_[6] + t_jerk_decel;
    
    total_time_ = t_[7];
    is_valid_ = true;
    
    return true;
}

SCurveProfile::State SCurveProfile::getStateAtTime(float time_sec) const {
    State state;
    state.position = 0.0f;
    state.velocity = v_start_;
    state.acceleration = 0.0f;
    state.phase = 0;
    state.is_complete = false;
    
    if (!is_valid_) {
        return state;
    }
    
    // Clamp time
    float t = time_sec;
    if (t < 0) t = 0;
    if (t >= total_time_) {
        t = total_time_;
        state.is_complete = true;
        state.position = target_pos_;
        state.velocity = 0.0f;
        state.acceleration = 0.0f;
        state.phase = 7;
        return state;
    }
    
    // Find current phase
    uint32_t phase = 0;
    for (int i = 1; i < 8; i++) {
        if (t <= t_[i]) {
            phase = i;
            break;
        }
    }
    
    state = calculateStateInPhase(t, phase);
    state.phase = phase;
    
    return state;
}

SCurveProfile::State SCurveProfile::calculateStateInPhase(float t, uint32_t phase) const {
    State state;
    float t_phase = t - t_[phase - 1];  // Time within current phase
    
    // Calculate cumulative position/velocity at phase start
    float pos_prev = (phase > 1) ? positionAtPhaseEnd(phase - 1) : 0.0f;
    float vel_prev = v_start_;
    
    // Update vel_prev and acc_prev based on previous phases
    if (phase > 1) {
        // Simplified - calculate velocity at end of previous phase
        // This is a placeholder - full implementation would track through all phases
        for (uint32_t p = 1; p < phase; p++) {
            float dt = t_[p] - t_[p-1];
            // Update based on phase type (simplified)
            if (p == 1 || p == 5) {
                // Jerk phase (acceleration increasing)
                vel_prev += 0.5f * j_max_ * dt * dt;
            } else if (p == 2 || p == 6) {
                // Constant acceleration
                vel_prev += a_max_ * dt;
            } else if (p == 3 || p == 7) {
                // Jerk phase (acceleration decreasing)
                vel_prev += a_max_ * dt - 0.5f * j_max_ * dt * dt;
            }
        }
    }
    
    // Calculate state in current phase
    switch(phase) {
        case 1: // Jerk-up (accel increasing)
            state.acceleration = j_max_ * t_phase;
            state.velocity = vel_prev + 0.5f * j_max_ * t_phase * t_phase;
            state.position = pos_prev + vel_prev * t_phase + (1.0f/6.0f) * j_max_ * std::pow(t_phase, 3);
            break;
            
        case 2: // Constant acceleration
            state.acceleration = a_max_;
            state.velocity = vel_prev + a_max_ * t_phase;
            state.position = pos_prev + vel_prev * t_phase + 0.5f * a_max_ * t_phase * t_phase;
            break;
            
        case 3: // Jerk-down (accel decreasing)
            state.acceleration = a_max_ - j_max_ * t_phase;
            state.velocity = vel_prev + a_max_ * t_phase - 0.5f * j_max_ * t_phase * t_phase;
            state.position = pos_prev + vel_prev * t_phase + 0.5f * a_max_ * t_phase * t_phase 
                           - (1.0f/6.0f) * j_max_ * std::pow(t_phase, 3);
            break;
            
        case 4: // Constant velocity
            state.acceleration = 0.0f;
            state.velocity = v_max_;
            state.position = pos_prev + v_max_ * t_phase;
            break;
            
        case 5: // Jerk-up (decel increasing)
            state.acceleration = -j_max_ * t_phase;
            state.velocity = vel_prev - 0.5f * j_max_ * t_phase * t_phase;
            state.position = pos_prev + vel_prev * t_phase - (1.0f/6.0f) * j_max_ * std::pow(t_phase, 3);
            break;
            
        case 6: // Constant deceleration
            state.acceleration = -a_max_;
            state.velocity = vel_prev - a_max_ * t_phase;
            state.position = pos_prev + vel_prev * t_phase - 0.5f * a_max_ * t_phase * t_phase;
            break;
            
        case 7: // Jerk-down (decel decreasing)
            state.acceleration = -a_max_ + j_max_ * t_phase;
            state.velocity = vel_prev - a_max_ * t_phase + 0.5f * j_max_ * t_phase * t_phase;
            state.position = pos_prev + vel_prev * t_phase - 0.5f * a_max_ * t_phase * t_phase 
                           + (1.0f/6.0f) * j_max_ * std::pow(t_phase, 3);
            break;
            
        default:
            state.acceleration = 0.0f;
            state.velocity = 0.0f;
            state.position = 0.0f;
            break;
    }
    
    state.is_complete = false;
    return state;
}

float SCurveProfile::positionAtPhaseEnd(uint32_t phase) const {
    if (phase == 0) return 0.0f;
    
    // Simplified calculation - returns approximate position
    // Full implementation would integrate through all phases
    State state = getStateAtTime(t_[phase]);
    return state.position;
}

