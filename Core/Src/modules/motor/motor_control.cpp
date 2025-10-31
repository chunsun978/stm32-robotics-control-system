#include "main.h"
#include "motor/StepperMotor.hpp"
#include "motor/SCurveProfile.hpp"
#include "motor/MotionPlanner.hpp"
#include "motor/MotorStateMachine.hpp"
#include <stdio.h>
#include <memory>

extern TIM_HandleTypeDef htim2;  // Declared in main.c

// Test mode selection
#define TEST_MODE_BASIC   0
#define TEST_MODE_SCURVE  1
#define CURRENT_TEST_MODE TEST_MODE_SCURVE  // Change this to switch modes

// Global instances (C++ style with proper initialization)
static std::unique_ptr<StepperMotor> g_motor;
static std::unique_ptr<MotorStateMachine> g_state_machine;

/**
 * @brief Initialize stepper motor with hardware configuration
 * @return Reference to initialized motor
 */
StepperMotor& initializeMotor() {
    StepperMotor::Config config;
    
    // Step pulse generation (PWM on TIM2 CH1)
    config.step_timer = &htim2;
    config.step_channel = TIM_CHANNEL_1;
    
    // Direction control (PA8)
    config.dir_port = GPIOA;
    config.dir_pin = GPIO_PIN_8;
    
    // Enable control (PA9, active HIGH for this driver)
    config.enable_port = GPIOA;
    config.enable_pin = GPIO_PIN_9;
    config.enable_active_low = false;  // Active HIGH enable (enable when pin is HIGH)
    
    g_motor = std::make_unique<StepperMotor>(config);
    return *g_motor;
}

/**
 * @brief Initialize state machine with callbacks
 * @return Reference to initialized state machine
 */
MotorStateMachine& initializeStateMachine() {
    g_state_machine = std::make_unique<MotorStateMachine>();
    
    // Set up state transition callback
    g_state_machine->setTransitionCallback(
        [](MotorStateMachine::State from, MotorStateMachine::State to, 
           MotorStateMachine::Event event) {
            // This is automatically logged by the state machine
            (void)from; (void)to; (void)event;
        }
    );
    
    // Set up state entry callback
    g_state_machine->setStateEntryCallback(
        [](MotorStateMachine::State state) {
            switch (state) {
                case MotorStateMachine::State::READY:
                    printf("  → Motor ready for commands\r\n");
                    break;
                case MotorStateMachine::State::ERROR:
                    printf("  → ERROR STATE - System halted!\r\n");
                    break;
                default:
                    break;
            }
        }
    );
    
    return *g_state_machine;
}

// S-curve test with smooth acceleration/deceleration (C++ style with state machine)
void test_scurve_motion(StepperMotor& motor, MotorStateMachine& sm) {
    printf("\r\n=== S-Curve Motion Control Test (with State Machine) ===\r\n");
    printf("Smooth acceleration and deceleration with controlled jerk\r\n\r\n");
    
    // Enable motor and transition state
    sm.processEvent(MotorStateMachine::Event::ENABLE);
    motor.setEnabled(true);
    printf("  Motor enabled: PA9 should be HIGH\r\n");
    HAL_Delay(500);  // Give time to see enable signal on logic analyzer
    
    // Create S-curve profile
    SCurveProfile profile;
    SCurveProfile::Config config;
    
    // Test 1: Move 1000 steps with moderate speed
    printf("\nTest 1: Move 1000 steps (smooth acceleration)\r\n");
    config.max_velocity = 500.0f;       // steps/sec
    config.max_acceleration = 1000.0f;  // steps/sec²
    config.max_jerk = 5000.0f;          // steps/sec³
    config.start_velocity = 0.0f;
    
    if (profile.calculate(1000.0f, config)) {
        printf("  Profile calculated successfully!\r\n");
        printf("  Total time: %.2f sec\r\n", profile.getTotalTime());
        printf("  Max velocity: %.1f steps/sec\r\n", config.max_velocity);
        
        motor.setDirection(true);  // Forward
        
        // Start motion - transition to ACCELERATING
        sm.processEvent(MotorStateMachine::Event::START_MOTION);
        
        // Execute motion by updating speed every 50ms
        uint32_t start = HAL_GetTick();
        float elapsed = 0.0f;
        bool reached_cruise = false;
        
        printf("  Starting motion loop...\r\n");
        
        // **TEST: Set constant speed at start and never change it**
        printf("  TEST: Setting constant 300 steps/sec for entire profile duration\r\n");
        motor.setStepRate(300.0f);  // Set once
        
        while (elapsed < profile.getTotalTime()) {
            elapsed = (HAL_GetTick() - start) / 1000.0f;
            
            SCurveProfile::State state = profile.getStateAtTime(elapsed);
            
            // DON'T update speed - keep it constant to test if problem is with updating
            // motor.setStepRate(state.velocity);
            
            // Track state transitions based on profile phase
            if (!reached_cruise && state.phase == 4) {
                sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);  // ACCELERATING -> RUNNING
                reached_cruise = true;
            }
            
            // Print status every 200ms
            static uint32_t last_print = 0;
            if (HAL_GetTick() - last_print > 200) {
                printf("  t=%.2fs, pos=%.1f, vel=%.1f, acc=%.1f, phase=%lu, state=%s\r\n",
                       elapsed, state.position, state.velocity, 
                       state.acceleration, state.phase,
                       MotorStateMachine::getStateName(sm.getState()));
                last_print = HAL_GetTick();
            }
            
            HAL_Delay(20);  // Update every 20ms (smoother than 50ms)
        }
        
        // Motion complete - transition to DECELERATING then READY
        sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);
        motor.stop();
        sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);
        
        printf("  Motion complete!\r\n");
    } else {
        printf("  ERROR: Test 1 profile calculation FAILED!\r\n");
    }
    
    HAL_Delay(2000);
    
    // Test 2: Move 2000 steps with higher speed
    printf("\nTest 2: Move 2000 steps (faster profile)\r\n");
    config.max_velocity = 1000.0f;
    config.max_acceleration = 2000.0f;
    config.max_jerk = 10000.0f;
    
    if (profile.calculate(2000.0f, config)) {
        printf("  Total time: %.2f sec\r\n", profile.getTotalTime());
        printf("  Max velocity: %.1f steps/sec\r\n", config.max_velocity);
        
        // Re-enable motor for Test 2
        motor.setEnabled(true);  // ← FIX: Enable motor again!
        motor.setDirection(false);  // Reverse
        sm.processEvent(MotorStateMachine::Event::START_MOTION);
        
        uint32_t start = HAL_GetTick();
        float elapsed = 0.0f;
        bool reached_cruise = false;
        
        while (elapsed < profile.getTotalTime()) {
            elapsed = (HAL_GetTick() - start) / 1000.0f;
            
            SCurveProfile::State state = profile.getStateAtTime(elapsed);
            motor.setStepRate(state.velocity);
            
            // Track state transitions
            if (!reached_cruise && state.phase == 4) {
                sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);
                reached_cruise = true;
            }
            
            static uint32_t last_print = 0;
            if (HAL_GetTick() - last_print > 200) {
                printf("  t=%.2fs, pos=%.1f, vel=%.1f, acc=%.1f, phase=%lu, state=%s\r\n",
                       elapsed, state.position, state.velocity, 
                       state.acceleration, state.phase,
                       MotorStateMachine::getStateName(sm.getState()));
                last_print = HAL_GetTick();
            }
            
            HAL_Delay(50);
        }
        
        sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);
        motor.stop();
        sm.processEvent(MotorStateMachine::Event::MOTION_COMPLETE);
        printf("  Motion complete!\r\n");
    }
    
    HAL_Delay(2000);
}

// Basic speed test (C++ style)
void test_basic_speed(StepperMotor& motor) {
    printf("\r\n=== Basic Speed Control Test ===\r\n");
    
    // Enable motor driver (RAII pattern)
    motor.setEnabled(true);
    HAL_Delay(100);
    
    while (1) {
        // Forward 100 steps/s
        printf("Forward 100 steps/s (10s)\r\n");
        motor.setDirection(true);
        motor.setStepRate(100.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (2s)\r\n");
        motor.stop();
        HAL_Delay(2000);
        
        // Reverse 200 steps/s
        printf("Reverse 200 steps/s (10s)\r\n");
        motor.setDirection(false);
        motor.setStepRate(200.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (2s)\r\n");
        motor.stop();
        HAL_Delay(2000);
        
        // Fast forward 500 steps/s
        printf("Fast forward 500 steps/s (10s)\r\n");
        motor.setDirection(true);
        motor.setStepRate(500.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (3s)\r\n\r\n");
        motor.stop();
        HAL_Delay(3000);
    }
}

// C linkage for main (C++ implementation inside)
extern "C" {

void motor_control_main(void) {
    printf("\r\n=== STM32 Robotics Control System ===\r\n");
    printf("System Clock: %lu Hz\r\n", SystemCoreClock);
    printf("APB1 Timer Clock: %lu Hz\r\n", HAL_RCC_GetPCLK1Freq() * 2);
    printf("C++ Version: Modern C++17\r\n");
    printf("Features: State Machine, S-Curve Profiles, OOP Design\r\n\r\n");
    
    // Initialize motor and state machine with C++ RAII pattern
    StepperMotor& motor = initializeMotor();
    MotorStateMachine& sm = initializeStateMachine();
    
    // Initialize state machine
    sm.processEvent(MotorStateMachine::Event::INITIALIZE);
    
    // === S-CURVE MOTION TEST ===
    printf("\r\n=== S-Curve Motion Test ===\r\n");
    
    while (1) {
        // Test 1: 1000 steps forward
        printf("\n--- Test 1: 1000 steps (smooth) ---\r\n");
        motor.setEnabled(true);
        motor.setDirection(true);
        HAL_Delay(100);
        
        SCurveProfile profile;
        SCurveProfile::Config config;
        config.max_velocity = 500.0f;
        config.max_acceleration = 1000.0f;
        config.start_velocity = 0.0f;
        
        if (profile.calculate(1000.0f, config)) {
            printf("Profile calculated: %.2f sec\r\n", profile.getTotalTime());
            
            uint32_t start = HAL_GetTick();
            float elapsed = 0.0f;
            const float MIN_VELOCITY = 50.0f;
            bool motor_started = false;
            float last_velocity = 0.0f;
            int loop_count = 0;
            
            while (elapsed < profile.getTotalTime()) {
                elapsed = (HAL_GetTick() - start) / 1000.0f;
                SCurveProfile::State state = profile.getStateAtTime(elapsed);
                
                // Printf provides necessary timing delays for PWM stability
                printf("  [%d] t=%.2f v=%.1f", loop_count++, elapsed, state.velocity);
                
                if (state.velocity >= MIN_VELOCITY) {
                    // Only update if velocity changed by more than 50 steps/sec
                    if (std::abs(state.velocity - last_velocity) > 50.0f) {
                        motor.setStepRate(state.velocity);
                        last_velocity = state.velocity;
                        printf(" -> SET\r\n");
                    } else {
                        printf(" -> RUN\r\n");
                    }
                    motor_started = true;
                } else if (motor_started) {
                    printf(" -> STOP\r\n");
                    break;
                } else {
                    printf(" -> WAIT\r\n");
                }
                
                HAL_Delay(50);
            }
            
            // Ensure motor is stopped (in case loop completed naturally)
            motor.stop();
            printf("Complete!\r\n");
        }
        
        HAL_Delay(2000);
        
        // Test 2: 2000 steps reverse
        printf("\n--- Test 2: 2000 steps (faster, reverse) ---\r\n");
        motor.setDirection(false);
        HAL_Delay(100);
        
        config.max_velocity = 1000.0f;
        config.max_acceleration = 2000.0f;
        
        if (profile.calculate(2000.0f, config)) {
            printf("Profile calculated: %.2f sec\r\n", profile.getTotalTime());
            
            uint32_t start = HAL_GetTick();
            float elapsed = 0.0f;
            const float MIN_VELOCITY = 50.0f;
            bool motor_started = false;
            float last_velocity = 0.0f;
            int loop_count = 0;
            
            while (elapsed < profile.getTotalTime()) {
                elapsed = (HAL_GetTick() - start) / 1000.0f;
                SCurveProfile::State state = profile.getStateAtTime(elapsed);
                
                // Printf provides necessary timing delays for PWM stability
                printf("  [%d] t=%.2f v=%.1f", loop_count++, elapsed, state.velocity);
                
                if (state.velocity >= MIN_VELOCITY) {
                    if (std::abs(state.velocity - last_velocity) > 50.0f) {
                        motor.setStepRate(state.velocity);
                        last_velocity = state.velocity;
                        printf(" -> SET\r\n");
                    } else {
                        printf(" -> RUN\r\n");
                    }
                    motor_started = true;
                } else if (motor_started) {
                    printf(" -> STOP\r\n");
                    break;
                } else {
                    printf(" -> WAIT\r\n");
                }
                
                HAL_Delay(50);
            }

            
            // Ensure motor is stopped
            motor.stop();
            printf("Complete!\r\n");
        }
        
        motor.setEnabled(false);
        HAL_Delay(3000);
        
        printf("\n=== Cycle complete, repeating ===\r\n");
    }
}

} // extern "C"