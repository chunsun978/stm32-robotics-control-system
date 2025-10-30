#include "Led.hpp"

Led::Led(GPIO_TypeDef* port, uint16_t pin)
    : m_port(port), m_pin(pin), m_state(false)
{
    // Initialize LED to OFF state
    off();
}

void Led::on() {
    HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_SET);
    m_state = true;
}

void Led::off() {
    HAL_GPIO_WritePin(m_port, m_pin, GPIO_PIN_RESET);
    m_state = false;
}

void Led::toggle() {
    HAL_GPIO_TogglePin(m_port, m_pin);
    m_state = !m_state;
}

void Led::set(bool state) {
    if (state) {
        on();
    } else {
        off();
    }
}

bool Led::getState() const {
    return m_state;
}

