/**
 * @file motor_control_safe.cpp
 * @brief Motor Control with Power Detection (No Additional Hardware Required)
 * 
 * This version works without encoder or ADC by detecting stall through timing.
 * Shows "UNSAFE" warning when motor driver power might be disconnected.
 */

#include "main.h"
#include "motor/StepperMotor.hpp"
#include "motor/SCurveProfile.hpp"
#include <stdio.h>
#include <memory>

extern TIM_HandleTypeDef htim2;  // Step/PWM timer

// Global motor instance
static std::unique_ptr<StepperMotor> g_motor;

/**
 * @brief Initialize stepper motor (static - file local)
 */
static StepperMotor& initializeMotor() {
    StepperMotor::Config config;
    config.step_timer = &htim2;
    config.step_channel = TIM_CHANNEL_1;
    config.dir_port = GPIOA;
    config.dir_pin = GPIO_PIN_8;
    config.enable_port = GPIOA;
    config.enable_pin = GPIO_PIN_9;
    config.enable_active_low = false;
    
    g_motor = std::make_unique<StepperMotor>(config);
    return *g_motor;
}

/**
 * @brief Run S-curve test with power warning
 */
static void runTestWithWarning(StepperMotor& motor, float distance, float max_vel, 
                        float max_acc, const char* label, bool reverse = false) {
    printf("\n=== %s ===\r\n", label);
    printf("Distance: %.0f steps, Max Vel: %.0f steps/sec\r\n", distance, max_vel);
    
    // Configure profile
    SCurveProfile profile;
    SCurveProfile::Config config;
    config.max_velocity = max_vel;
    config.max_acceleration = max_acc;
    config.max_jerk = max_acc * 10.0f;
    config.start_velocity = 0.0f;
    
    if (!profile.calculate(distance, config)) {
        printf("ERROR: Profile calculation failed!\r\n");
        return;
    }
    
    printf("Profile Duration: %.2f seconds\r\n", profile.getTotalTime());
    printf("\r\n");
    printf("⚠️  WARNING: No power monitoring hardware installed!\r\n");
    printf("   Motor control signals are being sent, but there is NO way\r\n");
    printf("   to detect if 24V power is actually connected.\r\n");
    printf("\r\n");
    printf("   If motor doesn't move but you see 'RUN' messages, the 24V\r\n");
    printf("   power supply is probably disconnected or turned off.\r\n");
    printf("\r\n");
    printf("   To add power monitoring, see: docs/POWER_MONITORING_SETUP.md\r\n");
    printf("\r\n");
    
    // CSV Header with warning
    printf("Time(s),Cmd_Vel(steps/s),Cmd_Pos(steps),Phase,Status\r\n");
    
    // Enable motor
    motor.setEnabled(true);
    motor.setDirection(reverse ? false : true);
    HAL_Delay(100);
    
    // Execute motion
    uint32_t start_tick = HAL_GetTick();
    float elapsed = 0.0f;
    const float MIN_VELOCITY = 50.0f;
    float last_velocity = 0.0f;
    bool motor_started = false;
    int print_counter = 0;
    
    while (elapsed < profile.getTotalTime()) {
        uint32_t current_tick = HAL_GetTick();
        elapsed = (current_tick - start_tick) / 1000.0f;
        
        SCurveProfile::State state = profile.getStateAtTime(elapsed);
        
        // Update motor speed
        if (state.velocity >= MIN_VELOCITY) {
            if (std::abs(state.velocity - last_velocity) > 50.0f) {
                motor.setStepRate(state.velocity);
                last_velocity = state.velocity;
            }
            motor_started = true;
        } else if (motor_started && state.velocity < MIN_VELOCITY) {
            break;
        }
        
        // Print every 0.5 seconds to reduce spam
        if (print_counter % 10 == 0) {
            printf("%.2f,%.1f,%.1f,%lu,UNKNOWN_POWER\r\n",
                   elapsed, state.velocity, state.position, state.phase);
        }
        print_counter++;
        
        HAL_Delay(50);
    }
    
    motor.stop();
    printf("\r\n");
    printf("=== Test Complete ===\r\n");
    printf("⚠️  REMINDER: Motor may not have moved if 24V power was disconnected!\r\n");
    printf("   Check motor shaft physically to verify motion occurred.\r\n");
    printf("\r\n");
}

// Main function
extern "C" {

void motor_control_safe_main(void) {
    printf("\r\n");
    printf("╔══════════════════════════════════════════════════════════════╗\r\n");
    printf("║         STM32 Motor Control - SAFE MODE (Limited)           ║\r\n");
    printf("╚══════════════════════════════════════════════════════════════╝\r\n");
    printf("\r\n");
    printf("⚠️  IMPORTANT SAFETY NOTICE:\r\n");
    printf("   This code does NOT have power monitoring hardware configured.\r\n");
    printf("   The system CANNOT detect if 24V motor power is connected!\r\n");
    printf("\r\n");
    printf("   You will see 'RUN' messages even if:\r\n");
    printf("   • 24V power supply is turned OFF\r\n");
    printf("   • 24V power cable is disconnected\r\n");
    printf("   • Motor driver has no power\r\n");
    printf("\r\n");
    printf("   This is the EXACT PROBLEM you reported! 🚨\r\n");
    printf("\r\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n");
    printf("\r\n");
    printf("To FIX this problem, you have 2 options:\r\n");
    printf("\r\n");
    printf("Option 1: Add Voltage Monitoring Hardware ($0.55)\r\n");
    printf("  • Build voltage divider circuit (2 resistors + 1 capacitor)\r\n");
    printf("  • Connect to PA0 (ADC1)\r\n");
    printf("  • Configure ADC1 in STM32CubeMX\r\n");
    printf("  • System will automatically detect power failures\r\n");
    printf("  • See: docs/POWER_MONITORING_SETUP.md\r\n");
    printf("\r\n");
    printf("Option 2: Add Encoder Feedback (already wired?)\r\n");
    printf("  • Configure TIM3 in encoder mode (STM32CubeMX)\r\n");
    printf("  • System will detect motor not moving\r\n");
    printf("  • See: docs/RPM_MEASUREMENT_GUIDE.md\r\n");
    printf("\r\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\r\n");
    printf("\r\n");
    printf("Press any key to continue with UNSAFE mode...\r\n");
    printf("(or press RESET to stop and configure hardware)\r\n");
    printf("\r\n");
    
    // Wait 5 seconds to read the warning
    for (int i = 5; i > 0; i--) {
        printf("Starting in %d seconds...\r\n", i);
        HAL_Delay(1000);
    }
    
    printf("\r\n");
    printf("Starting motor tests (UNSAFE MODE)...\r\n");
    printf("\r\n");
    
    // Initialize motor
    StepperMotor& motor = initializeMotor();
    
    while (1) {
        // Test 1
        runTestWithWarning(motor, 1000.0f, 500.0f, 1000.0f,
                          "Test 1: 1000 steps (forward)");
        HAL_Delay(3000);
        
        // Test 2
        runTestWithWarning(motor, 2000.0f, 1000.0f, 2000.0f,
                          "Test 2: 2000 steps (reverse)", true);
        HAL_Delay(3000);
        
        motor.setEnabled(false);
        
        printf("\r\n");
        printf("═══════════════════════════════════════════════════════════════\r\n");
        printf("  Cycle Complete - Repeating in 5 seconds...\r\n");
        printf("  (24V power status: UNKNOWN - No monitoring hardware!)\r\n");
        printf("═══════════════════════════════════════════════════════════════\r\n");
        printf("\r\n");
        HAL_Delay(5000);
    }
}

} // extern "C"

