#include "motor/StepperMotor.hpp"
#include <algorithm>

StepperMotor::StepperMotor(const Config& config)
    : config_(config)
    , current_step_rate_(0.0f)
    , is_enabled_(false)
    , is_forward_(true)
{
    // Initialize motor in disabled state
    setEnabled(false);
    setDirection(true);
    stop();
}

StepperMotor::~StepperMotor() {
    // RAII: Ensure motor is safely disabled when object is destroyed
    stop();
    setEnabled(false);
}

void StepperMotor::setEnabled(bool enabled) {
    is_enabled_ = enabled;
    
    // Determine GPIO state based on active-low configuration
    GPIO_PinState state;
    if (config_.enable_active_low) {
        state = enabled ? GPIO_PIN_RESET : GPIO_PIN_SET;  // Active LOW
    } else {
        state = enabled ? GPIO_PIN_SET : GPIO_PIN_RESET;  // Active HIGH
    }
    
    HAL_GPIO_WritePin(config_.enable_port, config_.enable_pin, state);
}

void StepperMotor::setDirection(bool forward) {
    is_forward_ = forward;
    
    GPIO_PinState state = forward ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(config_.dir_port, config_.dir_pin, state);
}

void StepperMotor::setStepRate(float steps_per_sec) {
    if (steps_per_sec <= 0.0f) {
        stop();
        return;
    }
    
    current_step_rate_ = steps_per_sec;
    updatePWMFrequency(steps_per_sec);
}

void StepperMotor::stop() {
    current_step_rate_ = 0.0f;
    HAL_TIM_PWM_Stop(config_.step_timer, config_.step_channel);
}

void StepperMotor::updatePWMFrequency(float frequency_hz) {
    if (frequency_hz <= 0.0f) {
        stop();
        return;
    }
    
    // Calculate timer settings for desired PWM frequency
    // PWM frequency = Timer Clock / ((Prescaler + 1) * (Period + 1))
    
    uint32_t timer_clock = getTimerClock();
    uint32_t target_freq = static_cast<uint32_t>(frequency_hz);
    
    // Use fixed prescaler for better resolution
    const uint32_t prescaler = 99;  // 84 MHz / 100 = 840 kHz base freq
    uint32_t period = (timer_clock / (prescaler + 1)) / target_freq;
    
    // Clamp period to valid range
    period = std::max(static_cast<uint32_t>(2), std::min(period, static_cast<uint32_t>(65535)));
    
    // Configure timer
    __HAL_TIM_SET_PRESCALER(config_.step_timer, prescaler);
    __HAL_TIM_SET_AUTORELOAD(config_.step_timer, period - 1);
    __HAL_TIM_SET_COMPARE(config_.step_timer, config_.step_channel, period / 2);  // 50% duty
    
    // Start PWM
    HAL_TIM_PWM_Start(config_.step_timer, config_.step_channel);
}

uint32_t StepperMotor::getTimerClock() const {
    // For STM32F411, APB1 timers run at 2x APB1 clock
    return HAL_RCC_GetPCLK1Freq() * 2;
}

