/**
 * @file PowerMonitor.cpp
 * @brief Implementation of Power Supply Voltage Monitor
 */

#include "Motor/PowerMonitor.hpp"
#include <stdio.h>
#include <cmath>

namespace Motor {

PowerMonitor::PowerMonitor(const PowerMonitorConfig& config)
    : adc_(config.adc)
    , channel_(config.adc_channel)
    , divider_ratio_(config.voltage_divider_ratio)
    , min_threshold_(config.min_voltage_threshold)
    , max_threshold_(config.max_voltage_threshold)
    , last_adc_value_(0)
    , last_voltage_(0.0f)
    , initialized_(false)
{
}

bool PowerMonitor::init() {
    if (!adc_) {
        return false;
    }
    
    // Note: ADC calibration (HAL_ADCEx_Calibration_Start) is not available on STM32F4
    // It's only available on F3, L0, L4, etc. series
    // STM32F4 ADC works fine without explicit calibration
    
    initialized_ = true;
    
    // Take initial reading
    readVoltage();
    
    return true;
}

float PowerMonitor::readVoltage() {
    if (!initialized_ || !adc_) {
        return 0.0f;
    }
    
    // Configure ADC channel
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel_;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    
    if (HAL_ADC_ConfigChannel(adc_, &sConfig) != HAL_OK) {
        return last_voltage_;
    }
    
    // Start ADC conversion
    if (HAL_ADC_Start(adc_) != HAL_OK) {
        return last_voltage_;
    }
    
    // Wait for conversion to complete
    if (HAL_ADC_PollForConversion(adc_, ADC_TIMEOUT) == HAL_OK) {
        // Read ADC value
        last_adc_value_ = HAL_ADC_GetValue(adc_);
        
        // Convert to voltage
        last_voltage_ = convertADCToVoltage(last_adc_value_);
        
        // DEBUG: Print raw ADC value to diagnose issue
        static uint32_t debug_counter = 0;
        if (debug_counter++ % 100 == 0) {  // Print every 100 reads
            printf("[DEBUG] RAW ADC: %lu, ADC_V: %.3fV, Calc_V: %.1fV\r\n",
                   last_adc_value_, 
                   (last_adc_value_ * 3.3f) / 4095.0f,
                   last_voltage_);
        }
    }
    
    // Stop ADC
    HAL_ADC_Stop(adc_);
    
    return last_voltage_;
}

bool PowerMonitor::isPowerGood() {
    float voltage = readVoltage();
    return (voltage >= min_threshold_ && voltage <= max_threshold_);
}

bool PowerMonitor::isCriticalLow() {
    float voltage = readVoltage();
    return (voltage < (min_threshold_ - 2.0f));
}

bool PowerMonitor::isOverVoltage() {
    float voltage = readVoltage();
    return (voltage > max_threshold_);
}

float PowerMonitor::getADCVoltage() const {
    return (last_adc_value_ * ADC_VREF) / ADC_MAX;
}

float PowerMonitor::convertADCToVoltage(uint32_t adc_value) const {
    // Convert ADC value to voltage at ADC pin
    float adc_voltage = (static_cast<float>(adc_value) * ADC_VREF) / ADC_MAX;
    
    // Apply voltage divider ratio to get actual supply voltage
    float supply_voltage = adc_voltage * divider_ratio_;
    
    return supply_voltage;
}

void PowerMonitor::printDiagnostics() const {
    printf("=== Power Monitor Diagnostics ===\r\n");
    printf("  Supply Voltage:  %.2f V\r\n", last_voltage_);
    printf("  ADC Raw Value:   %lu (0x%03lX)\r\n", last_adc_value_, last_adc_value_);
    printf("  ADC Pin Voltage: %.3f V\r\n", getADCVoltage());
    printf("  Divider Ratio:   %.2f\r\n", divider_ratio_);
    printf("  Min Threshold:   %.1f V\r\n", min_threshold_);
    printf("  Max Threshold:   %.1f V\r\n", max_threshold_);
    
    // Status
    if (last_voltage_ >= min_threshold_ && last_voltage_ <= max_threshold_) {
        printf("  Status: ✓ POWER GOOD\r\n");
    } else if (last_voltage_ < min_threshold_) {
        printf("  Status: ✗ LOW VOLTAGE WARNING\r\n");
    } else {
        printf("  Status: ✗ OVER VOLTAGE WARNING\r\n");
    }
    printf("=================================\r\n");
}

} // namespace Motor

