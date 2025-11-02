#include "Motor/SimpleFOCMotorDriver.hpp"

namespace Motor {

SimpleFOCMotorDriver::SimpleFOCMotorDriver(const Config& config)
    : pwm_timer_a_(config.pwm_timer_a)
    , pwm_timer_b_(config.pwm_timer_b)
    , pwm_channel_a_(config.pwm_channel_a)
    , pwm_channel_b_(config.pwm_channel_b)
    , enable_port_(config.enable_port)
    , enable_pin_(config.enable_pin)
    , reverse_direction_(config.reverse_direction)
    , current_speed_(0.0f)
    , enabled_(false)
{
}

bool SimpleFOCMotorDriver::init() {
    if (!pwm_timer_a_ || !pwm_timer_b_) {
        return false;
    }
    
    // Start PWM on both channels (different timers!)
    if (HAL_TIM_PWM_Start(pwm_timer_a_, pwm_channel_a_) != HAL_OK) {
        return false;
    }
    if (HAL_TIM_PWM_Start(pwm_timer_b_, pwm_channel_b_) != HAL_OK) {
        return false;
    }
    
    // Initialize stopped
    stop();
    
    return true;
}

void SimpleFOCMotorDriver::enable() {
    HAL_GPIO_WritePin(enable_port_, enable_pin_, GPIO_PIN_SET);
    enabled_ = true;
}

void SimpleFOCMotorDriver::disable() {
    HAL_GPIO_WritePin(enable_port_, enable_pin_, GPIO_PIN_RESET);
    enabled_ = false;
    current_speed_ = 0.0f;
}

void SimpleFOCMotorDriver::setSpeed(float speed) {
    // Clamp speed
    speed = std::max(MIN_SPEED, std::min(MAX_SPEED, speed));
    
    // Apply direction reversal
    if (reverse_direction_) {
        speed = -speed;
    }
    
    current_speed_ = speed;
    
    // Determine direction and duty
    bool forward = speed >= 0.0f;
    float duty = std::abs(speed) / 100.0f;
    
    // Enable driver
    enable();
    
    // Set PWM for 2-phase DC control
    if (forward) {
        // Forward: A=PWM, B=0
        setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, duty);
        setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, 0.0f);
    } else {
        // Reverse: A=0, B=PWM
        setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, 0.0f);
        setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, duty);
    }
}

void SimpleFOCMotorDriver::setPWM(float duty, bool forward) {
    duty = std::max(0.0f, std::min(1.0f, duty));
    
    if (reverse_direction_) {
        forward = !forward;
    }
    
    enable();
    
    if (forward) {
        setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, duty);
        setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, 0.0f);
    } else {
        setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, 0.0f);
        setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, duty);
    }
    
    current_speed_ = (forward ? 1.0f : -1.0f) * duty * 100.0f;
}

void SimpleFOCMotorDriver::stop() {
    setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, 0.0f);
    setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, 0.0f);
    disable();
    current_speed_ = 0.0f;
}

void SimpleFOCMotorDriver::brake() {
    // Active brake: both phases at same PWM
    setPWMDutyCycle(pwm_timer_a_, pwm_channel_a_, 0.5f);
    setPWMDutyCycle(pwm_timer_b_, pwm_channel_b_, 0.5f);
    enable();
    current_speed_ = 0.0f;
}

void SimpleFOCMotorDriver::setPWMDutyCycle(TIM_HandleTypeDef* timer, uint32_t channel, float duty) {
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(timer);
    uint32_t pulse = static_cast<uint32_t>(duty * arr);
    __HAL_TIM_SET_COMPARE(timer, channel, pulse);
}

} // namespace Motor

