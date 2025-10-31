#include "motor/SCurveProfile.hpp"
#include <algorithm>
#include <cmath>

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
    
    // **SIMPLIFIED 3-PHASE S-CURVE**
    // Phase 1: Acceleration (0 → v_max with jerk)
    // Phase 2: Constant velocity (v_max)
    // Phase 3: Deceleration (v_max → 0 with jerk)
    
    // Time to accelerate from 0 to v_max with jerk
    float t_accel = v_max_ / a_max_;
    
    // Distance during acceleration (trapezoidal approximation)
    float s_accel = 0.5f * v_max_ * t_accel;
    
    // Distance during deceleration (symmetric)
    float s_decel = s_accel;
    
    // Remaining distance at constant velocity
    float s_const = target_pos_ - s_accel - s_decel;
    
    // Time at constant velocity
    float t_const = 0.0f;
    if (s_const > 0) {
        t_const = s_const / v_max_;
    } else {
        // Can't reach v_max, reduce peak velocity
        s_const = 0.0f;
        t_const = 0.0f;
        
        // Recalculate for shorter distance
        // v_peak^2 = 2 * a_max * (distance/2)
        float v_peak = std::sqrt(a_max_ * target_pos_);
        v_peak = std::min(v_peak, v_max_);
        
        t_accel = v_peak / a_max_;
        s_accel = 0.5f * v_peak * t_accel;
        s_decel = s_accel;
    }
    
    float t_decel = t_accel;  // Symmetric
    
    // Set phase timings (simplified 3-phase)
    t_[0] = 0.0f;
    t_[1] = t_accel;           // End of acceleration
    t_[2] = t_[1] + t_const;   // End of constant velocity  
    t_[3] = t_[2] + t_decel;   // End of deceleration
    t_[4] = t_[3];
    t_[5] = t_[3];
    t_[6] = t_[3];
    t_[7] = t_[3];
    
    total_time_ = t_[3];
    is_valid_ = true;
    
    return true;
}

SCurveProfile::State SCurveProfile::getStateAtTime(float time_sec) const {
    State state;
    state.position = 0.0f;
    state.velocity = 0.0f;
    state.acceleration = 0.0f;
    state.phase = 0;
    state.is_complete = false;
    
    if (!is_valid_) {
        return state;
    }
    
    float t = time_sec;
    
    // Clamp time to valid range
    if (t < 0.0f) t = 0.0f;
    if (t > total_time_) {
        t = total_time_;
        state.is_complete = true;
    }
    
    // **3-PHASE CALCULATION**
    
    if (t <= t_[1]) {
        // Phase 1: ACCELERATION (0 → v_max)
        state.phase = 1;
        float t_accel = t_[1];
        float progress = t / t_accel;  // 0.0 to 1.0
        
        state.velocity = v_max_ * progress;  // Linear ramp
        state.acceleration = a_max_;
        state.position = 0.5f * v_max_ * t * progress;  // Trapezoidal area
        
    } else if (t <= t_[2]) {
        // Phase 2: CONSTANT VELOCITY
        state.phase = 4;
        float t_const = t - t_[1];
        float s_accel = 0.5f * v_max_ * t_[1];
        
        state.velocity = v_max_;
        state.acceleration = 0.0f;
        state.position = s_accel + v_max_ * t_const;
        
    } else {
        // Phase 3: DECELERATION (v_max → 0)
        state.phase = 7;
        float t_decel_phase = t - t_[2];
        float t_decel_total = t_[3] - t_[2];
        float progress = t_decel_phase / t_decel_total;  // 0.0 to 1.0
        
        float s_accel = 0.5f * v_max_ * t_[1];
        float s_const = v_max_ * (t_[2] - t_[1]);
        
        state.velocity = v_max_ * (1.0f - progress);  // Linear ramp down
        state.acceleration = -a_max_;
        state.position = s_accel + s_const + v_max_ * t_decel_phase * (1.0f - 0.5f * progress);
    }
    
    // Clamp velocity to never be negative
    if (state.velocity < 0.0f) state.velocity = 0.0f;
    
    return state;
}

SCurveProfile::State SCurveProfile::calculateStateInPhase(float t, uint32_t phase) const {
    // Not used in simplified version
    return getStateAtTime(t);
}

float SCurveProfile::positionAtPhaseEnd(uint32_t phase) const {
    if (phase == 0 || phase > 7) return 0.0f;
    return getStateAtTime(t_[phase]).position;
}
