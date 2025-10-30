#ifndef INC_MODULES_MOTOR_SCURVEPROFILE_HPP_
#define INC_MODULES_MOTOR_SCURVEPROFILE_HPP_

#include <cstdint>
#include <cmath>

/**
 * @brief 7-phase S-curve motion profile generator
 * 
 * Generates smooth acceleration/deceleration profiles with controlled jerk.
 * 
 * Phase 1: Jerk-up (acceleration increasing)
 * Phase 2: Constant acceleration
 * Phase 3: Jerk-down (acceleration decreasing)
 * Phase 4: Constant velocity
 * Phase 5: Jerk-up (deceleration increasing)
 * Phase 6: Constant deceleration
 * Phase 7: Jerk-down (deceleration decreasing)
 */
class SCurveProfile {
public:
    struct Config {
        float max_velocity;       // steps/sec
        float max_acceleration;   // steps/sec²
        float max_jerk;          // steps/sec³
        float start_velocity;    // steps/sec (usually 0)
    };
    
    struct State {
        float position;          // steps
        float velocity;          // steps/sec
        float acceleration;      // steps/sec²
        uint32_t phase;         // Current phase (1-7)
        bool is_complete;       // Motion finished
    };

    SCurveProfile();
    
    /**
     * @brief Calculate profile for a given target position
     * @param target_position Target position in steps
     * @param config Motion constraints
     * @return true if profile is valid
     */
    bool calculate(float target_position, const Config& config);
    
    /**
     * @brief Get state at a specific time
     * @param time_sec Time since motion start (seconds)
     * @return Current state
     */
    State getStateAtTime(float time_sec) const;
    
    /**
     * @brief Get total motion time
     */
    float getTotalTime() const { return total_time_; }
    
    /**
     * @brief Check if profile is valid
     */
    bool isValid() const { return is_valid_; }

private:
    // Phase timing
    float t_[8];  // Time at end of each phase (t_[0] = 0, t_[7] = total time)
    
    // Motion parameters
    float target_pos_;
    float v_max_;
    float a_max_;
    float j_max_;
    float v_start_;
    
    float total_time_;
    bool is_valid_;
    
    // Helper functions
    void calculatePhaseTimings();
    State calculateStateInPhase(float t, uint32_t phase) const;
    float positionAtPhaseEnd(uint32_t phase) const;
};

#endif /* INC_MODULES_MOTOR_SCURVEPROFILE_HPP_ */

