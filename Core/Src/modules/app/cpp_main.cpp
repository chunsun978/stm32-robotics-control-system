/**
 * @file cpp_main.cpp
 * @brief C++ main application code
 * 
 * This file contains your C++ code that interfaces with the STM32 HAL
 */

#include "modules/hal/Led.hpp"
#include "motor/motor_control.h"
#include "main.h"
#include <stdio.h>  // For printf

// C linkage for functions called from C code
extern "C" {

// Declare motor control functions
void motor_control_main(void);                    // Original (no monitoring)
void motor_control_safe_main(void);               // Safe mode with warnings
void motor_control_with_encoder_main(void);       // Full version (needs TIM3 + ADC1)

/**
 * @brief C++ application entry point called from main.c
 * 
 * This function is called from main.c after all hardware initialization
 */
void cpp_main(void) {
    // Print startup message
    printf("\r\n=== STM32 Robotics Control System ===\r\n");
    printf("System Clock: %lu Hz\r\n", SystemCoreClock);
    printf("UART Baud Rate: 115200\r\n\r\n");
    
    // CURRENT: Full power monitoring enabled! 🎉
    motor_control_with_encoder_main();  // ✓ Has voltage monitoring on PA0!
    
    // SAFE MODE: Shows warnings (use if ADC not configured)
    // motor_control_safe_main();
    
    // OLD: Original code (no power monitoring)
    // motor_control_main();
}

} // extern "C"

