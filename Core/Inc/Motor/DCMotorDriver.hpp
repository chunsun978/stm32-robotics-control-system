/**
 * @file DCMotorDriver.hpp
 * @brief DC Motor Driver with H-Bridge Control
 * 
 * Hardware-agnostic DC motor interface using PWM speed control
 * and direction control via H-bridge (L298N, BTS7960, etc.)
 */

#ifndef DC_MOTOR_DRIVER_HPP
#define DC_MOTOR_DRIVER_HPP

#include <cstdint>
#include "stm32f4xx_hal.h"

namespace Motor {

/**
 * @brief DC Motor Configuration
 */
struct DCMotorConfig {
    TIM_HandleTypeDef* pwm_timer;   ///< Timer for PWM (e.g., TIM1)
    uint32_t pwm_channel;           ///< PWM channel (TIM_CHANNEL_1, etc.)
    GPIO_TypeDef* dir1_port;        ///< Direction pin 1 port (IN1)
    uint16_t dir1_pin;              ///< Direction pin 1 (GPIO_PIN_x)
    GPIO_TypeDef* dir2_port;        ///< Direction pin 2 port (IN2)
    uint16_t dir2_pin;              ///< Direction pin 2 (GPIO_PIN_x)
    bool reverse_direction;         ///< Reverse motor direction
};

/**
 * @brief DC Motor Driver Class
 * 
 * Controls brushed DC motor via H-bridge driver.
 * Supports speed (PWM) and direction control.
 */
class DCMotorDriver {
public:
    /**
     * @brief Constructor
     * @param config Motor configuration
     */
    explicit DCMotorDriver(const DCMotorConfig& config);
    
    /**
     * @brief Initialize motor driver
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Set motor speed
     * @param speed Speed from -100 to +100 (negative = reverse)
     */
    void setSpeed(float speed);
    
    /**
     * @brief Set motor PWM duty cycle directly
     * @param duty_cycle PWM duty 0.0 to 1.0
     * @param forward true for forward, false for reverse
     */
    void setPWM(float duty_cycle, bool forward);
    
    /**
     * @brief Stop motor (coast)
     */
    void stop();
    
    /**
     * @brief Brake motor (short both terminals)
     */
    void brake();
    
    /**
     * @brief Get current speed setting
     * @return Speed -100 to +100
     */
    float getSpeed() const { return current_speed_; }
    
    /**
     * @brief Check if motor is running
     * @return true if speed > 0
     */
    bool isRunning() const { return current_speed_ != 0.0f; }

private:
    TIM_HandleTypeDef* pwm_timer_;
    uint32_t pwm_channel_;
    GPIO_TypeDef* dir1_port_;
    uint16_t dir1_pin_;
    GPIO_TypeDef* dir2_port_;
    uint16_t dir2_pin_;
    bool reverse_direction_;
    
    float current_speed_;
    
    void setDirection(bool forward);
    void setPWMDutyCycle(float duty);
    
    static constexpr float MAX_SPEED = 100.0f;
    static constexpr float MIN_SPEED = -100.0f;
};

} // namespace Motor

#endif // DC_MOTOR_DRIVER_HPP

