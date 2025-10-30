#ifndef INC_MODULES_MOTOR_STEPPERMOTOR_HPP_
#define INC_MODULES_MOTOR_STEPPERMOTOR_HPP_

#include "stm32f4xx_hal.h"
#include <cstdint>

/**
 * @brief Stepper motor controller class with PWM-based step generation
 * 
 * Encapsulates all hardware access for stepper motor control.
 * Follows RAII principles and modern C++ design patterns.
 */
class StepperMotor {
public:
    /**
     * @brief Configuration for stepper motor pins
     */
    struct Config {
        // Step pulse generation (PWM)
        TIM_HandleTypeDef* step_timer;
        uint32_t step_channel;
        
        // Direction control
        GPIO_TypeDef* dir_port;
        uint16_t dir_pin;
        
        // Enable control
        GPIO_TypeDef* enable_port;
        uint16_t enable_pin;
        bool enable_active_low;  // true if LOW = enabled
    };
    
    /**
     * @brief Construct stepper motor controller
     * @param config Hardware configuration
     */
    explicit StepperMotor(const Config& config);
    
    /**
     * @brief Destructor - ensures motor is disabled
     */
    ~StepperMotor();
    
    // Prevent copying (motor controls unique hardware)
    StepperMotor(const StepperMotor&) = delete;
    StepperMotor& operator=(const StepperMotor&) = delete;
    
    // Allow moving
    StepperMotor(StepperMotor&&) = default;
    StepperMotor& operator=(StepperMotor&&) = default;
    
    /**
     * @brief Enable/disable motor driver
     * @param enabled true to enable motor
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Set rotation direction
     * @param forward true for forward, false for reverse
     */
    void setDirection(bool forward);
    
    /**
     * @brief Set step rate (velocity)
     * @param steps_per_sec Desired speed in steps/second (0 to stop)
     */
    void setStepRate(float steps_per_sec);
    
    /**
     * @brief Stop motor immediately
     */
    void stop();
    
    /**
     * @brief Get current commanded step rate
     */
    float getStepRate() const { return current_step_rate_; }
    
    /**
     * @brief Check if motor is enabled
     */
    bool isEnabled() const { return is_enabled_; }
    
    /**
     * @brief Get current direction
     */
    bool isForward() const { return is_forward_; }

private:
    Config config_;
    float current_step_rate_;
    bool is_enabled_;
    bool is_forward_;
    
    // Helper to calculate timer settings
    void updatePWMFrequency(float frequency_hz);
    uint32_t getTimerClock() const;
};

#endif /* INC_MODULES_MOTOR_STEPPERMOTOR_HPP_ */

