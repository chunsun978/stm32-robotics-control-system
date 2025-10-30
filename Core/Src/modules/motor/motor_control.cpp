#include "main.h"
#include <stdio.h>

// Motor pins (update based on your wiring)
#define MOTOR_DIR_PORT  GPIOA
#define MOTOR_DIR_PIN   GPIO_PIN_8
#define MOTOR_EN_PORT   GPIOA
#define MOTOR_EN_PIN    GPIO_PIN_9

extern TIM_HandleTypeDef htim2;  // Declared in main.c

// Simple motor control functions
void motor_enable(bool enable) {
    // Active HIGH enable: HIGH = enable, LOW = disable
    GPIO_PinState state = enable ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(MOTOR_EN_PORT, MOTOR_EN_PIN, state);
}

void motor_set_direction(bool forward) {
    GPIO_PinState state = forward ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(MOTOR_DIR_PORT, MOTOR_DIR_PIN, state);
}

void motor_set_speed(float steps_per_second) {
    if (steps_per_second <= 0) {
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        return;
    }
    
    // Calculate timer settings for desired frequency
    // PWM frequency = Timer Clock / (Prescaler * Period)
    
    uint32_t timer_clock = HAL_RCC_GetPCLK1Freq() * 2; // APB1 timer clock (~84 MHz)
    uint32_t target_freq = (uint32_t)steps_per_second;
    
    uint32_t prescaler = 99;  // 84 MHz / 100 = 840 kHz
    uint32_t period = (timer_clock / (prescaler + 1)) / target_freq;
    
    if (period < 2) period = 2;  // Minimum period
    if (period > 65535) period = 65535;  // Max for 16-bit timer
    
    // Configure timer
    __HAL_TIM_SET_PRESCALER(&htim2, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&htim2, period - 1);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, period / 2);  // 50% duty cycle
    
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

// C linkage for main
extern "C" {

void motor_control_main(void) {
    printf("\r\n=== Stepper Motor Control ===\r\n");
    printf("System Clock: %lu Hz\r\n", SystemCoreClock);
    printf("APB1 Timer Clock: %lu Hz\r\n\r\n", HAL_RCC_GetPCLK1Freq() * 2);
    
    // Enable motor driver
    motor_enable(true);
    HAL_Delay(100);
    
    while (1) {
        // Ensure EN stays HIGH (sanity check)
        motor_enable(true);
        
        // Forward 100 steps/s
        printf("Forward 100 steps/s (10s)\r\n");
        motor_set_direction(true);
        motor_set_speed(100.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (2s)\r\n");
        motor_set_speed(0);
        HAL_Delay(2000);
        
        // Reverse 200 steps/s
        printf("Reverse 200 steps/s (10s)\r\n");
        motor_set_direction(false);
        motor_set_speed(200.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (2s)\r\n");
        motor_set_speed(0);
        HAL_Delay(2000);
        
        // Fast forward 500 steps/s
        printf("Fast forward 500 steps/s (10s)\r\n");
        motor_set_direction(true);
        motor_set_speed(500.0f);
        HAL_Delay(10000);
        
        // Stop
        printf("Stop (3s)\r\n\r\n");
        motor_set_speed(0);
        HAL_Delay(3000);
    }
}

} // extern "C"