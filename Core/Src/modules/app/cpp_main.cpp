/**
 * @file cpp_main.cpp
 * @brief C++ main application code
 * 
 * This file contains your C++ code that interfaces with the STM32 HAL
 */

#include "modules/hal/Led.hpp"
#include "main.h"
#include <stdio.h>  // For printf

// C linkage for functions called from C code
extern "C" {

/**
 * @brief C++ application entry point called from main.c
 * 
 * This function is called from main.c after all hardware initialization
 */
void cpp_main(void) {
    // Create LED object using C++ class
    Led userLed(LD2_GPIO_Port, LD2_Pin);
    
    // Print startup message
    printf("\r\n=== STM32 C++ Demo Started ===\r\n");
    printf("System Clock: %lu Hz\r\n", SystemCoreClock);
    printf("UART Baud Rate: 115200\r\n\r\n");
    
    uint32_t counter = 0;
    
    while (1) {
        counter++;
        
        // Very simple test - long pulses easy to see on scope
        userLed.on();
        printf("LED ON  - PA5 should be HIGH (3.3V)\r\n");
        HAL_Delay(2000);  // 2 seconds ON
        
        userLed.off();
        printf("LED OFF - PA5 should be LOW  (0V)\r\n");
        HAL_Delay(2000);  // 2 seconds OFF
        
        printf("Counter: %lu\r\n\r\n", counter);
    }
}

} // extern "C"

