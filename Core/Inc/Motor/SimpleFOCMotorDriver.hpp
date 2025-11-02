#ifndef SIMPLEFOC_MOTOR_DRIVER_HPP
#define SIMPLEFOC_MOTOR_DRIVER_HPP

#include "stm32f4xx_hal.h"
#include <algorithm>
#include <cmath>

namespace Motor {

/**
 * @brief SimpleFOCShield V3.2 driver for brushed DC motor
 * 
 * Uses 2-phase control (A and B) for DC motor on BLDC shield
 */
class SimpleFOCMotorDriver {
public:
    struct Config {
        TIM_HandleTypeDef* pwm_timer_a;   // TIM2 for channel A
        uint32_t pwm_channel_a;           // TIM_CHANNEL_2 (PB3 = D3)
        TIM_HandleTypeDef* pwm_timer_b;   // TIM4 for channel B
        uint32_t pwm_channel_b;           // TIM_CHANNEL_1 (PB6 = D10, jumper to D5)
        GPIO_TypeDef* enable_port;        // GPIOA
        uint16_t enable_pin;              // GPIO_PIN_10 (D2)
        bool reverse_direction;           // Swap if motor spins backward
    };

    SimpleFOCMotorDriver(const Config& config);
    
    bool init();
    void enable();
    void disable();
    
    /**
     * @brief Set motor speed
     * @param speed: -100 to +100 (percentage)
     */
    void setSpeed(float speed);
    
    /**
     * @brief Set raw PWM duty cycle
     * @param duty: 0.0 to 1.0
     * @param forward: true=forward, false=reverse
     */
    void setPWM(float duty, bool forward);
    
    void stop();      // Coast stop (disable)
    void brake();     // Active brake (short phases)
    
    float getSpeed() const { return current_speed_; }
    bool isEnabled() const { return enabled_; }

private:
    void setPWMDutyCycle(TIM_HandleTypeDef* timer, uint32_t channel, float duty);
    
    TIM_HandleTypeDef* pwm_timer_a_;
    TIM_HandleTypeDef* pwm_timer_b_;
    uint32_t pwm_channel_a_;
    uint32_t pwm_channel_b_;
    GPIO_TypeDef* enable_port_;
    uint16_t enable_pin_;
    bool reverse_direction_;
    float current_speed_;
    bool enabled_;
    
    static constexpr float MIN_SPEED = -100.0f;
    static constexpr float MAX_SPEED = 100.0f;
};

} // namespace Motor

#endif // SIMPLEFOC_MOTOR_DRIVER_HPP

