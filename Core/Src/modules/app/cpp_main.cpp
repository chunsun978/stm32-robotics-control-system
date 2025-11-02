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

// Declare motor control test function
void motor_control_with_encoder_main(void);       // Full version with encoder + power monitoring

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
    
    // Run motor control test with encoder feedback and power monitoring
    motor_control_with_encoder_main();
}

} // extern "C"

