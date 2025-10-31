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
    uint32_t timer_clock = getTimerClock();
    uint32_t target_freq = static_cast<uint32_t>(frequency_hz);
    
    const uint32_t prescaler = 99;  // 84 MHz / 100 = 840 kHz base freq
    uint32_t period = (timer_clock / (prescaler + 1)) / target_freq;
    
    // Clamp period to valid range
    period = std::max(static_cast<uint32_t>(2), std::min(period, static_cast<uint32_t>(65535)));
    
    // ALWAYS stop, reconfigure, and restart
    // On-the-fly updates with UG event seem unreliable
    HAL_TIM_PWM_Stop(config_.step_timer, config_.step_channel);
    
    config_.step_timer->Instance->PSC = prescaler;
    config_.step_timer->Instance->ARR = period - 1;
    config_.step_timer->Instance->CCR1 = period / 2;
    
    // Generate update event to load prescaler
    config_.step_timer->Instance->EGR = TIM_EGR_UG;
    __HAL_TIM_CLEAR_FLAG(config_.step_timer, TIM_FLAG_UPDATE);
    
    // Start PWM
    HAL_TIM_PWM_Start(config_.step_timer, config_.step_channel);
}

uint32_t StepperMotor::getTimerClock() const {
    // For STM32F411, APB1 timers run at 2x APB1 clock
    return HAL_RCC_GetPCLK1Freq() * 2;
}

