#ifndef LED_HPP
#define LED_HPP

#include "main.h"

/**
 * @brief C++ LED class wrapper for STM32 HAL
 * 
 * Example of using C++ with STM32 HAL library
 */
class Led {
public:
    /**
     * @brief Constructor - Initialize LED
     * @param port GPIO port (e.g., GPIOA)
     * @param pin GPIO pin (e.g., GPIO_PIN_5)
     */
    Led(GPIO_TypeDef* port, uint16_t pin);
    
    /**
     * @brief Turn LED on
     */
    void on();
    
    /**
     * @brief Turn LED off
     */
    void off();
    
    /**
     * @brief Toggle LED state
     */
    void toggle();
    
    /**
     * @brief Set LED to specific state
     * @param state true = ON, false = OFF
     */
    void set(bool state);
    
    /**
     * @brief Get current LED state
     * @return true if ON, false if OFF
     */
    bool getState() const;

private:
    GPIO_TypeDef* m_port;
    uint16_t m_pin;
    bool m_state;
};

#endif // LED_HPP

