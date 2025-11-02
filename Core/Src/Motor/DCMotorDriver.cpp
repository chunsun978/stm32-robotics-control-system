/**
 * @file DCMotorDriver.cpp
 * @brief DC Motor Driver Implementation
 */

#include "Motor/DCMotorDriver.hpp"
#include <cmath>
#include <algorithm>

namespace Motor {

DCMotorDriver::DCMotorDriver(const DCMotorConfig& config)
    : pwm_timer_(config.pwm_timer)
    , pwm_channel_(config.pwm_channel)
    , dir1_port_(config.dir1_port)
    , dir1_pin_(config.dir1_pin)
    , dir2_port_(config.dir2_port)
    , dir2_pin_(config.dir2_pin)
    , reverse_direction_(config.reverse_direction)
    , current_speed_(0.0f)
{
}

bool DCMotorDriver::init() {
    if (!pwm_timer_) {
        return false;
    }
    
    // Start PWM
    if (HAL_TIM_PWM_Start(pwm_timer_, pwm_channel_) != HAL_OK) {
        return false;
    }
    
    // Initialize with motor stopped
    stop();
    
    return true;
}

void DCMotorDriver::setSpeed(float speed) {
    // Clamp speed to valid range
    speed = std::max(MIN_SPEED, std::min(MAX_SPEED, speed));
    
    // Apply direction reversal if configured
    if (reverse_direction_) {
        speed = -speed;
    }
    
    // Store current speed
    current_speed_ = speed;
    
    // Determine direction and PWM duty
    bool forward = speed >= 0.0f;
    float duty = std::abs(speed) / 100.0f;
    
    // Set direction
    setDirection(forward);
    
    // Set PWM
    setPWMDutyCycle(duty);
}

void DCMotorDriver::setPWM(float duty_cycle, bool forward) {
    // Clamp duty cycle
    duty_cycle = std::max(0.0f, std::min(1.0f, duty_cycle));
    
    // Apply direction reversal if configured
    if (reverse_direction_) {
        forward = !forward;
    }
    
    // Set direction
    setDirection(forward);
    
    // Set PWM
    setPWMDutyCycle(duty_cycle);
    
    // Update speed representation
    current_speed_ = (forward ? 1.0f : -1.0f) * duty_cycle * 100.0f;
}

void DCMotorDriver::stop() {
    // Coast stop: both pins LOW
    HAL_GPIO_WritePin(dir1_port_, dir1_pin_, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(dir2_port_, dir2_pin_, GPIO_PIN_RESET);
    setPWMDutyCycle(0.0f);
    current_speed_ = 0.0f;
}

void DCMotorDriver::brake() {
    // Brake: both pins HIGH (short circuit motor terminals)
    HAL_GPIO_WritePin(dir1_port_, dir1_pin_, GPIO_PIN_SET);
    HAL_GPIO_WritePin(dir2_port_, dir2_pin_, GPIO_PIN_SET);
    setPWMDutyCycle(0.0f);
    current_speed_ = 0.0f;
}

void DCMotorDriver::setDirection(bool forward) {
    if (forward) {
        // Forward: IN1=HIGH, IN2=LOW
        HAL_GPIO_WritePin(dir1_port_, dir1_pin_, GPIO_PIN_SET);
        HAL_GPIO_WritePin(dir2_port_, dir2_pin_, GPIO_PIN_RESET);
    } else {
        // Reverse: IN1=LOW, IN2=HIGH
        HAL_GPIO_WritePin(dir1_port_, dir1_pin_, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(dir2_port_, dir2_pin_, GPIO_PIN_SET);
    }
}

void DCMotorDriver::setPWMDutyCycle(float duty) {
    // Get timer auto-reload register value
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(pwm_timer_);
    
    // Calculate compare value
    uint32_t pulse = static_cast<uint32_t>(duty * arr);
    
    // Set PWM duty cycle
    __HAL_TIM_SET_COMPARE(pwm_timer_, pwm_channel_, pulse);
}

} // namespace Motor

