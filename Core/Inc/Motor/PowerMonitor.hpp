/**
 * @file PowerMonitor.hpp
 * @brief 24V Power Supply Voltage Monitor
 * 
 * Monitors motor supply voltage using ADC with voltage divider.
 * Provides power-good detection and voltage threshold checking.
 */

#ifndef POWER_MONITOR_HPP
#define POWER_MONITOR_HPP

#include "stm32f4xx_hal.h"
#include <cstdint>

namespace Motor {

/**
 * @brief Power Monitor Configuration
 */
struct PowerMonitorConfig {
    ADC_HandleTypeDef* adc;           ///< ADC peripheral handle
    uint32_t adc_channel;             ///< ADC channel (e.g., ADC_CHANNEL_0)
    float voltage_divider_ratio;      ///< (R1 + R2) / R2
    float min_voltage_threshold;      ///< Minimum valid voltage (V)
    float max_voltage_threshold;      ///< Maximum safe voltage (V)
};

/**
 * @brief Power Supply Voltage Monitor
 * 
 * Monitors 24V motor supply using ADC and voltage divider.
 * Provides real-time voltage readings and power-good detection.
 * 
 * Example hardware:
 *   24V --[100kΩ]--+--[15kΩ]-- GND
 *                  |
 *                  +-- ADC Pin (PA1)
 * 
 * Divider ratio = (100k + 15k) / 15k = 7.67
 */
class PowerMonitor {
public:
    /**
     * @brief Constructor
     * @param config Power monitor configuration
     */
    explicit PowerMonitor(const PowerMonitorConfig& config);
    
    /**
     * @brief Initialize ADC
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Read current supply voltage
     * @return Voltage in volts (V)
     */
    float readVoltage();
    
    /**
     * @brief Check if power supply is within acceptable range
     * @return true if voltage is between min and max thresholds
     */
    bool isPowerGood();
    
    /**
     * @brief Check if voltage is critically low
     * @return true if voltage < min_threshold - 2V
     */
    bool isCriticalLow();
    
    /**
     * @brief Check if voltage is over threshold
     * @return true if voltage > max_threshold
     */
    bool isOverVoltage();
    
    /**
     * @brief Get last voltage reading (without new ADC conversion)
     * @return Last measured voltage in volts
     */
    float getLastVoltage() const { return last_voltage_; }
    
    /**
     * @brief Get raw ADC value (for debugging/calibration)
     * @return Raw ADC value (0-4095 for 12-bit ADC)
     */
    uint32_t getRawADC() const { return last_adc_value_; }
    
    /**
     * @brief Get voltage at ADC pin (after divider)
     * @return Voltage at ADC input (0-3.3V)
     */
    float getADCVoltage() const;
    
    /**
     * @brief Print diagnostic information
     */
    void printDiagnostics() const;

private:
    ADC_HandleTypeDef* adc_;
    uint32_t channel_;
    float divider_ratio_;
    float min_threshold_;
    float max_threshold_;
    
    uint32_t last_adc_value_;
    float last_voltage_;
    bool initialized_;
    
    // ADC constants
    static constexpr float ADC_VREF = 3.3f;      ///< STM32 ADC reference voltage
    static constexpr uint32_t ADC_MAX = 4095;    ///< 12-bit ADC max value
    static constexpr uint32_t ADC_TIMEOUT = 100; ///< ADC conversion timeout (ms)
    
    /**
     * @brief Convert raw ADC value to supply voltage
     * @param adc_value Raw ADC reading (0-4095)
     * @return Supply voltage in volts
     */
    float convertADCToVoltage(uint32_t adc_value) const;
};

} // namespace Motor

#endif // POWER_MONITOR_HPP

