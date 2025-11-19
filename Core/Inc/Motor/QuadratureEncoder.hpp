/**
 * @file QuadratureEncoder.hpp
 * @brief Quadrature Encoder Driver for Position Feedback
 * 
 * Hardware-agnostic encoder interface for closed-loop control.
 * Uses STM32 Timer in encoder mode for robust quadrature decoding.
 */

#ifndef QUADRATURE_ENCODER_HPP
#define QUADRATURE_ENCODER_HPP

#include <cstdint>
#include "stm32f4xx_hal.h"

namespace Motor {

/**
 * @brief Quadrature Encoder Configuration
 */
struct EncoderConfig {
    TIM_HandleTypeDef* timer;       ///< Timer in encoder mode (e.g., TIM3)
    uint32_t pulses_per_rev;        ///< Encoder PPR (e.g., 600)
    uint32_t counts_per_rev;        ///< Counts per rev = PPR × 4 (quadrature)
    bool reverse_direction;         ///< Reverse counting direction
};

/**
 * @brief Quadrature Encoder Class
 * 
 * Reads position from hardware quadrature encoder using STM32 timer.
 * Supports position, velocity, and direction feedback.
 */
class QuadratureEncoder {
public:
    /**
     * @brief Constructor
     * @param config Encoder configuration
     */
    explicit QuadratureEncoder(const EncoderConfig& config);
    
    /**
     * @brief Initialize encoder
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Get current encoder count (raw)
     * @return Raw timer counter value
     */
    int32_t getCount() const;
    
    /**
     * @brief Get position in steps
     * @return Position in motor steps
     */
    int32_t getPosition() const;
    
    /**
     * @brief Get position in degrees
     * @return Position in degrees (0-360)
     */
    float getPositionDegrees() const;
    
    /**
     * @brief Get velocity in counts/sec
     * @param delta_time Time since last update (seconds)
     * @return Velocity in encoder counts per second
     */
    float getVelocity(float delta_time);
    
    /**
     * @brief Get velocity in motor steps/sec
     * @param delta_time Time since last update (seconds)
     * @param steps_per_rev Motor steps per revolution
     * @return Velocity in motor steps per second
     */
    float getVelocitySteps(float delta_time, uint32_t steps_per_rev);
    
    /**
     * @brief Reset encoder position to zero
     */
    void reset();
    
    /**
     * @brief Set current position to specific value
     * @param position New position value
     */
    void setPosition(int32_t position);
    
    /**
     * @brief Check if encoder has index pulse
     * @return true if Z pulse detected (if connected)
     */
    bool hasIndex() const;

private:
    TIM_HandleTypeDef* timer_;
    uint32_t pulses_per_rev_;
    uint32_t counts_per_rev_;
    bool reverse_direction_;
    
    int32_t last_count_;
    int32_t accumulated_position_;
    uint32_t overflow_count_;
    
    static constexpr uint32_t TIMER_MAX_COUNT = 65535;
};

} // namespace Motor

#endif // QUADRATURE_ENCODER_HPP

