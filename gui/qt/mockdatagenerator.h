/**
 * @file mockdatagenerator.h
 * @brief Mock Data Generator for Testing
 * 
 * Generates simulated motor motion data for testing GUI without hardware.
 */

#ifndef MOCKDATAGENERATOR_H
#define MOCKDATAGENERATOR_H

#include <QObject>

// Forward declaration
struct TelemetryPoint;

/**
 * @brief Mock Data Generator Class
 * 
 * Simulates S-curve motion profile with realistic telemetry data.
 */
class MockDataGenerator : public QObject
{
    Q_OBJECT

public:
    explicit MockDataGenerator(QObject *parent = nullptr);
    
    void planMotion(float steps, float max_velocity, float acceleration);
    void start();
    void stop();
    void reset();
    
    TelemetryPoint getNextPoint();
    bool isRunning() const { return running; }

private:
    // Motion parameters
    float target_steps;
    float max_vel;
    float max_accel;
    
    // Current state
    float current_time;
    float current_position;
    float current_velocity;
    uint8_t current_phase;
    
    // S-curve profile timings
    float t_accel;
    float t_const;
    float t_total;
    
    // Control flags
    bool planned;
    bool running;
    
    // Helper functions
    void calculateProfile();
    TelemetryPoint computeStateAtTime(float time_sec);
};

#endif // MOCKDATAGENERATOR_H

