/**
 * @file QuadratureEncoder.cpp
 * @brief Quadrature Encoder Implementation
 */

#include "Motor/QuadratureEncoder.hpp"
#include <cmath>

namespace Motor {

QuadratureEncoder::QuadratureEncoder(const EncoderConfig& config)
    : timer_(config.timer)
    , pulses_per_rev_(config.pulses_per_rev)
    , counts_per_rev_(config.counts_per_rev)
    , reverse_direction_(config.reverse_direction)
    , last_count_(0)
    , accumulated_position_(0)
    , overflow_count_(0)
{
}

bool QuadratureEncoder::init() {
    if (!timer_) {
        return false;
    }
    
    // Start encoder mode (quadrature decoder)
    if (HAL_TIM_Encoder_Start(timer_, TIM_CHANNEL_ALL) != HAL_OK) {
        return false;
    }
    
    // Reset counter to middle of range to allow both directions
    __HAL_TIM_SET_COUNTER(timer_, TIMER_MAX_COUNT / 2);
    last_count_ = TIMER_MAX_COUNT / 2;
    
    return true;
}

int32_t QuadratureEncoder::getCount() const {
    uint32_t raw_count = __HAL_TIM_GET_COUNTER(timer_);
    
    // Convert to signed, accounting for direction
    int32_t count = static_cast<int32_t>(raw_count);
    
    if (reverse_direction_) {
        count = -count;
    }
    
    return count;
}

int32_t QuadratureEncoder::getPosition() const {
    return accumulated_position_;
}

float QuadratureEncoder::getPositionDegrees() const {
    if (counts_per_rev_ == 0) {
        return 0.0f;
    }
    
    float degrees = (static_cast<float>(accumulated_position_) / counts_per_rev_) * 360.0f;
    
    // Normalize to 0-360
    while (degrees < 0) degrees += 360.0f;
    while (degrees >= 360.0f) degrees -= 360.0f;
    
    return degrees;
}

float QuadratureEncoder::getVelocity(float delta_time) {
    if (delta_time <= 0.0f) {
        return 0.0f;
    }
    
    int32_t current_count = getCount();
    int32_t delta_count = current_count - last_count_;
    
    // Handle overflow/underflow
    if (delta_count > static_cast<int32_t>(TIMER_MAX_COUNT / 2)) {
        delta_count -= static_cast<int32_t>(TIMER_MAX_COUNT);
    } else if (delta_count < -static_cast<int32_t>(TIMER_MAX_COUNT / 2)) {
        delta_count += static_cast<int32_t>(TIMER_MAX_COUNT);
    }
    
    // Update accumulated position
    accumulated_position_ += delta_count;
    last_count_ = current_count;
    
    // Calculate velocity in counts per second
    return static_cast<float>(delta_count) / delta_time;
}

float QuadratureEncoder::getVelocitySteps(float delta_time, uint32_t steps_per_rev) {
    float velocity_counts = getVelocity(delta_time);
    
    if (counts_per_rev_ == 0) {
        return 0.0f;
    }
    
    // Convert from encoder counts/sec to motor steps/sec
    return velocity_counts * (static_cast<float>(steps_per_rev) / counts_per_rev_);
}

void QuadratureEncoder::reset() {
    __HAL_TIM_SET_COUNTER(timer_, TIMER_MAX_COUNT / 2);
    last_count_ = TIMER_MAX_COUNT / 2;
    accumulated_position_ = 0;
    overflow_count_ = 0;
}

void QuadratureEncoder::setPosition(int32_t position) {
    accumulated_position_ = position;
    last_count_ = __HAL_TIM_GET_COUNTER(timer_);
}

bool QuadratureEncoder::hasIndex() const {
    // TODO: Implement Z pulse detection if needed
    // Would require GPIO interrupt on Z channel
    return false;
}

} // namespace Motor

