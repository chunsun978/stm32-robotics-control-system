/**
 * @file motor_control_with_encoder.cpp
 * @brief Enhanced Motor Control Test with Encoder RPM Measurement
 * 
 * Extended test duration with real-time encoder feedback for data collection.
 */

#include "main.h"
#include "motor/StepperMotor.hpp"
#include "motor/SCurveProfile.hpp"
#include "Motor/QuadratureEncoder.hpp"
#include "Motor/PowerMonitor.hpp"
#include <stdio.h>
#include <memory>
#include <cmath>

extern TIM_HandleTypeDef htim2;  // Step/PWM timer

// These peripherals are optional - they'll be defined in main.c after CubeMX configuration
// Provide weak dummy definitions so code compiles even without CubeMX configuration
TIM_HandleTypeDef htim3 __attribute__((weak));  // Encoder timer (configure TIM3 in CubeMX)
ADC_HandleTypeDef hadc1 __attribute__((weak));  // ADC for power monitoring (configure ADC1 in CubeMX)

// Global instances
static std::unique_ptr<StepperMotor> g_motor;
static std::unique_ptr<Motor::QuadratureEncoder> g_encoder;
static std::unique_ptr<Motor::PowerMonitor> g_power_monitor;

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
 * @brief Initialize encoder for feedback (static - file local)
 */
static Motor::QuadratureEncoder& initializeEncoder() {
    Motor::EncoderConfig config;
    config.timer = &htim3;                 // TIM3 in encoder mode
    config.pulses_per_rev = 600;           // E6B2-CWZ6C is 600 PPR
    config.counts_per_rev = 600 * 4;       // 2400 counts per revolution (quadrature)
    config.reverse_direction = false;      // Set true if encoder counts backwards
    
    g_encoder = std::make_unique<Motor::QuadratureEncoder>(config);
    
    if (!g_encoder->init()) {
        printf("ERROR: Encoder initialization failed!\r\n");
    }
    
    return *g_encoder;
}

/**
 * @brief Initialize power monitor for 24V supply (static - file local)
 */
static Motor::PowerMonitor& initializePowerMonitor() {
    Motor::PowerMonitorConfig config;
    config.adc = &hadc1;                     // ADC1
    config.adc_channel = ADC_CHANNEL_1;      // PA1 = ADC1_IN1
    config.voltage_divider_ratio = 11.47f;   // (157kΩ + 15kΩ) / 15kΩ (USER'S ACTUAL CIRCUIT)
    config.min_voltage_threshold = 22.0f;    // Minimum 22V (user's PSU is ~27-30V)
    config.max_voltage_threshold = 32.0f;    // Maximum 32V (allow headroom for PSU)
    
    g_power_monitor = std::make_unique<Motor::PowerMonitor>(config);
    
    if (!g_power_monitor->init()) {
        printf("ERROR: Power monitor initialization failed!\r\n");
    }
    
    return *g_power_monitor;
}

/**
 * @brief Calculate RPM from encoder velocity
 * @param counts_per_sec Encoder counts per second
 * @param counts_per_rev Encoder counts per revolution
 * @return RPM
 */
static float calculateRPM(float counts_per_sec, uint32_t counts_per_rev) {
    // RPM = (counts/sec) / (counts/rev) * 60
    return (counts_per_sec / counts_per_rev) * 60.0f;
}

/**
 * @brief Extended S-Curve test with encoder RPM measurement
 * @param motor Reference to motor
 * @param encoder Reference to encoder
 * @param power_monitor Reference to power monitor
 * @param distance Distance in steps
 * @param max_vel Maximum velocity (steps/sec)
 * @param max_acc Maximum acceleration (steps/sec²)
 * @param label Test label for printing
 * @param reverse Motor direction
 */
static void extendedTestWithEncoder(
    StepperMotor& motor, 
    Motor::QuadratureEncoder& encoder,
    Motor::PowerMonitor& power_monitor,
    float distance,
    float max_vel,
    float max_acc,
    const char* label,
    bool reverse = false
) {
    printf("\n=== %s ===\r\n", label);
    printf("Distance: %.0f steps, Max Vel: %.0f steps/sec, Max Acc: %.0f steps/sec²\r\n",
           distance, max_vel, max_acc);
    
    // Check power before starting
    if (!power_monitor.isPowerGood()) {
        printf("❌ ERROR: 24V power not detected or out of range!\r\n");
        printf("   Measured: %.1fV (Required: 22-28V)\r\n", power_monitor.getLastVoltage());
        printf("   Please check power supply connection.\r\n");
        printf("\r\n");
        return;
    }
    printf("✓ Power Good: %.1fV\r\n", power_monitor.getLastVoltage());
    
    // Reset encoder
    encoder.reset();
    
    // Configure S-curve profile
    SCurveProfile profile;
    SCurveProfile::Config config;
    config.max_velocity = max_vel;
    config.max_acceleration = max_acc;
    config.max_jerk = max_acc * 10.0f;  // Jerk = 10x acceleration
    config.start_velocity = 0.0f;
    
    if (!profile.calculate(distance, config)) {
        printf("ERROR: Profile calculation failed!\r\n");
        return;
    }
    
    float total_time = profile.getTotalTime();
    printf("Profile Duration: %.2f seconds\r\n", total_time);
    printf("\r\n");
    
    // CSV Header
    printf("Time(s),Cmd_Vel(steps/s),Cmd_Pos(steps),Enc_Count,Enc_Vel(counts/s),Enc_RPM,Supply_V,Phase,Status\r\n");
    
    // Enable motor
    motor.setEnabled(true);
    motor.setDirection(reverse ? false : true);
    HAL_Delay(100);
    
    // Execute motion with encoder feedback
    uint32_t start_tick = HAL_GetTick();
    float elapsed = 0.0f;
    float last_velocity = 0.0f;
    const float MIN_VELOCITY = 50.0f;
    const float UPDATE_INTERVAL = 0.05f;  // 50ms = 20Hz update rate
    bool motor_started = false;
    
    int32_t last_encoder_count = encoder.getCount();
    uint32_t last_encoder_tick = start_tick;
    
    while (elapsed < total_time) {
        uint32_t current_tick = HAL_GetTick();
        elapsed = (current_tick - start_tick) / 1000.0f;
        
        // Get commanded state from profile
        SCurveProfile::State state = profile.getStateAtTime(elapsed);
        
        // Update motor speed if velocity changed significantly
        if (state.velocity >= MIN_VELOCITY) {
            if (std::abs(state.velocity - last_velocity) > 50.0f) {
                motor.setStepRate(state.velocity);
                last_velocity = state.velocity;
            }
            motor_started = true;
        } else if (motor_started && state.velocity < MIN_VELOCITY) {
            // Motion complete
            break;
        }
        
        // Measure encoder velocity
        int32_t current_encoder_count = encoder.getCount();
        float delta_time = (current_tick - last_encoder_tick) / 1000.0f;
        
        float encoder_velocity = 0.0f;
        float encoder_rpm = 0.0f;
        
        if (delta_time >= UPDATE_INTERVAL) {
            int32_t delta_counts = current_encoder_count - last_encoder_count;
            encoder_velocity = delta_counts / delta_time;  // counts per second
            encoder_rpm = calculateRPM(encoder_velocity, 2400);  // 600 PPR × 4
            
            last_encoder_count = current_encoder_count;
            last_encoder_tick = current_tick;
            
            // Check power supply voltage
            float supply_voltage = power_monitor.getLastVoltage();
            const char* status = "OK";
            
            // Periodically update voltage reading (every ~50ms for faster detection)
            // Note: Slower than ideal due to SimpleFOC shield input capacitors
            static uint32_t last_voltage_check = 0;
            if (current_tick - last_voltage_check > 50) {
                supply_voltage = power_monitor.readVoltage();
                last_voltage_check = current_tick;
                
                // Check if power failed
                if (!power_monitor.isPowerGood()) {
                    status = "POWER_FAIL";
                    motor.stop();
                    printf("%.3f,%.1f,%.1f,%ld,%.1f,%.2f,%.1f,%lu,%s\r\n",
                           elapsed, state.velocity, state.position,
                           current_encoder_count, encoder_velocity, encoder_rpm,
                           supply_voltage, state.phase, status);
                    printf("\r\n");
                    printf("❌ EMERGENCY STOP: Power failure detected!\r\n");
                    printf("   Voltage dropped to %.1fV (threshold: 22V)\r\n", supply_voltage);
                    printf("   Motor stopped for safety.\r\n");
                    printf("\r\n");
                    return;  // Exit test immediately
                }
            }
            
            // CSV Output: Time, Cmd_Vel, Cmd_Pos, Enc_Count, Enc_Vel, RPM, Supply_V, Phase, Status
            printf("%.3f,%.1f,%.1f,%ld,%.1f,%.2f,%.1f,%lu,%s\r\n",
                   elapsed,
                   state.velocity,
                   state.position,
                   current_encoder_count,
                   encoder_velocity,
                   encoder_rpm,
                   supply_voltage,
                   state.phase,
                   status);
        }
        
        HAL_Delay(50);  // 50ms loop
    }
    
    // Stop motor
    motor.stop();
    
    // Final encoder reading
    int32_t final_count = encoder.getCount();
    printf("\r\n");
    printf("=== Test Complete ===\r\n");
    printf("Final Encoder Count: %ld\r\n", final_count);
    printf("Revolutions: %.2f\r\n", final_count / 2400.0f);
    printf("\r\n");
}

/**
 * @brief Constant velocity test for RPM verification
 * @param motor Reference to motor
 * @param encoder Reference to encoder
 * @param power_monitor Reference to power monitor
 * @param velocity Constant velocity (steps/sec)
 * @param duration Test duration (seconds)
 * @param label Test label
 */
static void constantVelocityTest(
    StepperMotor& motor,
    Motor::QuadratureEncoder& encoder,
    Motor::PowerMonitor& power_monitor,
    float velocity,
    float duration,
    const char* label
) {
    printf("\n=== %s ===\r\n", label);
    printf("Constant Velocity: %.0f steps/sec for %.1f seconds\r\n", velocity, duration);
    
    // Check power before starting
    if (!power_monitor.isPowerGood()) {
        printf("❌ ERROR: 24V power not detected or out of range!\r\n");
        printf("   Measured: %.1fV (Required: 22-28V)\r\n", power_monitor.getLastVoltage());
        printf("   Please check power supply connection.\r\n");
        printf("\r\n");
        return;
    }
    printf("✓ Power Good: %.1fV\r\n", power_monitor.getLastVoltage());
    printf("\r\n");
    printf("Time(s),Cmd_Vel(steps/s),Enc_Count,Enc_Vel(counts/s),Enc_RPM,Supply_V,Status\r\n");
    
    // Reset encoder
    encoder.reset();
    
    // Enable motor and set constant speed
    motor.setEnabled(true);
    motor.setDirection(true);
    motor.setStepRate(velocity);
    HAL_Delay(100);
    
    uint32_t start_tick = HAL_GetTick();
    int32_t last_encoder_count = encoder.getCount();
    uint32_t last_encoder_tick = start_tick;
    
    while ((HAL_GetTick() - start_tick) / 1000.0f < duration) {
        uint32_t current_tick = HAL_GetTick();
        float elapsed = (current_tick - start_tick) / 1000.0f;
        
        int32_t current_encoder_count = encoder.getCount();
        float delta_time = (current_tick - last_encoder_tick) / 1000.0f;
        
        if (delta_time >= 0.1f) {  // 100ms update
            int32_t delta_counts = current_encoder_count - last_encoder_count;
            float encoder_velocity = delta_counts / delta_time;
            float encoder_rpm = calculateRPM(encoder_velocity, 2400);
            
            // Check power supply voltage
            float supply_voltage = power_monitor.readVoltage();
            const char* status = "OK";
            
            // Check if power failed
            if (!power_monitor.isPowerGood()) {
                status = "POWER_FAIL";
                motor.stop();
                printf("%.3f,%.1f,%ld,%.1f,%.2f,%.1f,%s\r\n",
                       elapsed, velocity, current_encoder_count,
                       encoder_velocity, encoder_rpm, supply_voltage, status);
                printf("\r\n");
                printf("❌ EMERGENCY STOP: Power failure detected!\r\n");
                printf("   Voltage dropped to %.1fV (threshold: 22V)\r\n", supply_voltage);
                printf("   Motor stopped for safety.\r\n");
                printf("\r\n");
                return;  // Exit test immediately
            }
            
            printf("%.3f,%.1f,%ld,%.1f,%.2f,%.1f,%s\r\n",
                   elapsed,
                   velocity,
                   current_encoder_count,
                   encoder_velocity,
                   encoder_rpm,
                   supply_voltage,
                   status);
            
            last_encoder_count = current_encoder_count;
            last_encoder_tick = current_tick;
        }
        
        HAL_Delay(100);
    }
    
    motor.stop();
    
    int32_t final_count = encoder.getCount();
    printf("\r\n");
    printf("=== Test Complete ===\r\n");
    printf("Final Encoder Count: %ld\r\n", final_count);
    printf("Revolutions: %.2f\r\n", final_count / 2400.0f);
    printf("Average RPM: %.2f\r\n", (final_count / 2400.0f) / (duration / 60.0f));
    printf("\r\n");
}

// Main test loop
extern "C" {

void motor_control_with_encoder_main(void) {
    printf("\r\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\r\n");
    printf("║  STM32 Motor Control - Extended Test with Encoder Feedback   ║\r\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\r\n");
    printf("\r\n");
    printf("System Configuration:\r\n");
    printf("  • Encoder: E6B2-CWZ6C (600 PPR, 2400 counts/rev)\r\n");
    printf("  • Motor: Stepper Motor via TIM2 PWM\r\n");
    printf("  • Power Monitor: 24V supply (ADC on PA1)\r\n");
    printf("  • Update Rate: 20 Hz (50ms)\r\n");
    printf("  • Data Format: CSV (Time, Cmd_Vel, Enc_Vel, RPM, Voltage)\r\n");
    printf("\r\n");
    
    // Initialize hardware
    StepperMotor& motor = initializeMotor();
    Motor::QuadratureEncoder& encoder = initializeEncoder();
    Motor::PowerMonitor& power_monitor = initializePowerMonitor();
    
    printf("Hardware initialized successfully!\r\n");
    printf("\r\n");
    
    // Display initial power status
    power_monitor.printDiagnostics();
    printf("\r\n");
    
    // Check initial power
    if (!power_monitor.isPowerGood()) {
        printf("❌ ERROR: 24V power supply not detected!\r\n");
        printf("   Measured: %.1fV (Required: 22-28V)\r\n", power_monitor.getLastVoltage());
        printf("\r\n");
        printf("⚠️  IMPORTANT: This code will NOT run motor tests without 24V power.\r\n");
        printf("   The system will wait for valid power before starting tests.\r\n");
        printf("\r\n");
        printf("   If you want to bypass power checking for testing:\r\n");
        printf("   1. Remove voltage divider or comment out power checks in code\r\n");
        printf("   2. Use encoder-only detection mode\r\n");
        printf("\r\n");
        
        // Wait for power to be connected
        while (!power_monitor.isPowerGood()) {
            printf("Waiting for 24V power... (current: %.1fV)\r\n", 
                   power_monitor.readVoltage());
            HAL_Delay(2000);
        }
        
        printf("\r\n✓ Power detected! Starting tests...\r\n\r\n");
    }
    
    while (1) {
        // Test 1: Constant velocity - 300 steps/sec for 15 seconds
        constantVelocityTest(motor, encoder, power_monitor, 300.0f, 15.0f, 
                          "Test 1: Constant 300 steps/sec (15s)");
        motor.setEnabled(false);  // Disable motor after test
        HAL_Delay(3000);
        
        // Test 2: Constant velocity - 600 steps/sec for 15 seconds
        constantVelocityTest(motor, encoder, power_monitor, 600.0f, 15.0f,
                          "Test 2: Constant 600 steps/sec (15s)");
        motor.setEnabled(false);  // Disable motor after test
        HAL_Delay(3000);
        
        // Test 3: S-curve profile - 5000 steps (~10 seconds)
        extendedTestWithEncoder(motor, encoder, power_monitor, 5000.0f, 800.0f, 1000.0f,
                              "Test 3: S-Curve 5000 steps (smooth profile)");
        motor.setEnabled(false);  // Disable motor after test
        HAL_Delay(3000);
        
        // Test 4: S-curve profile - 8000 steps reverse (~12 seconds)
        extendedTestWithEncoder(motor, encoder, power_monitor, 8000.0f, 1000.0f, 1200.0f,
                              "Test 4: S-Curve 8000 steps (reverse, fast)", true);
        motor.setEnabled(false);  // Disable motor after test
        HAL_Delay(3000);
        
        // Test 5: Long constant velocity - 1000 steps/sec for 20 seconds
        constantVelocityTest(motor, encoder, power_monitor, 1000.0f, 20.0f,
                           "Test 5: Constant 1000 steps/sec (20s endurance)");
        motor.setEnabled(false);  // Disable motor after test
        HAL_Delay(3000);
        
        printf("\r\n");
        printf("═══════════════════════════════════════════════════════════════\r\n");
        printf("  Cycle Complete - Repeating in 5 seconds...\r\n");
        printf("═══════════════════════════════════════════════════════════════\r\n");
        printf("\r\n");
        HAL_Delay(5000);
    }
}

} // extern "C"

