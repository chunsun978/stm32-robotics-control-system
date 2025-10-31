/**
 * @file mockdatagenerator.cpp
 * @brief Mock Data Generator Implementation
 * 
 * Generates simulated S-curve motion with realistic tracking and small errors.
 */

#include "mockdatagenerator.h"
#include "mainwindow.h"  // For TelemetryPoint definition
#include <cmath>
#include <QRandomGenerator>

/**
 * @brief Constructor
 */
MockDataGenerator::MockDataGenerator(QObject *parent)
    : QObject(parent)
    , target_steps(1000.0f)
    , max_vel(500.0f)
    , max_accel(1000.0f)
    , current_time(0.0f)
    , current_position(0.0f)
    , current_velocity(0.0f)
    , current_phase(0)
    , planned(false)
    , running(false)
{
    calculateProfile();
}

/**
 * @brief Plan a motion
 */
void MockDataGenerator::planMotion(float steps, float max_velocity, float acceleration)
{
    target_steps = std::abs(steps);
    max_vel = max_velocity;
    max_accel = acceleration;
    
    calculateProfile();
    planned = true;
}

/**
 * @brief Start motion
 */
void MockDataGenerator::start()
{
    if (!planned) return;
    
    running = true;
    current_time = 0.0f;
    current_position = 0.0f;
    current_velocity = 0.0f;
    current_phase = 1;  // Start in acceleration phase
}

/**
 * @brief Stop motion
 */
void MockDataGenerator::stop()
{
    running = false;
    current_phase = 0;
}

/**
 * @brief Reset to initial state
 */
void MockDataGenerator::reset()
{
    running = false;
    planned = false;
    current_time = 0.0f;
    current_position = 0.0f;
    current_velocity = 0.0f;
    current_phase = 0;
}

/**
 * @brief Calculate S-curve profile timings
 */
void MockDataGenerator::calculateProfile()
{
    // Simplified 3-phase S-curve (similar to firmware)
    float distance = std::abs(target_steps);
    
    // Time to accelerate to max velocity
    t_accel = max_vel / max_accel;
    float s_accel = 0.5f * max_accel * t_accel * t_accel;
    
    // Check if we can reach max velocity
    if (s_accel * 2 > distance) {
        // Triangular profile (no constant velocity phase)
        t_accel = std::sqrt(distance / max_accel);
        max_vel = max_accel * t_accel;  // Peak velocity
        s_accel = distance / 2.0f;
        t_const = 0.0f;
    } else {
        // Trapezoidal profile
        float s_const = distance - (2 * s_accel);
        t_const = s_const / max_vel;
    }
    
    t_total = t_accel + t_const + t_accel;
}

/**
 * @brief Compute ideal state at given time
 */
TelemetryPoint MockDataGenerator::computeStateAtTime(float time_sec)
{
    TelemetryPoint point;
    point.time_ms = time_sec * 1000.0f;
    point.phase = 0;
    point.target_position = 0.0f;
    point.target_velocity = 0.0f;
    point.acceleration = 0.0f;
    point.pid_output = 0.0f;
    
    if (time_sec >= t_total) {
        // Motion complete
        point.target_position = target_steps;
        point.target_velocity = 0.0f;
        point.phase = 0;
        return point;
    }
    
    float t = time_sec;
    
    // Phase 1: Acceleration
    if (t <= t_accel) {
        point.phase = 1;
        point.acceleration = max_accel;
        point.target_velocity = max_accel * t;
        point.target_position = 0.5f * max_accel * t * t;
    }
    // Phase 2: Constant Velocity
    else if (t <= t_accel + t_const) {
        point.phase = 2;
        float t_phase = t - t_accel;
        point.acceleration = 0.0f;
        point.target_velocity = max_vel;
        point.target_position = (0.5f * max_accel * t_accel * t_accel) + (max_vel * t_phase);
    }
    // Phase 3: Deceleration
    else {
        point.phase = 3;
        float t_phase = t - (t_accel + t_const);
        point.acceleration = -max_accel;
        point.target_velocity = max_vel - (max_accel * t_phase);
        
        // Clamp to zero
        if (point.target_velocity < 0) {
            point.target_velocity = 0;
        }
        
        float s_before = (0.5f * max_accel * t_accel * t_accel) + (max_vel * t_const);
        point.target_position = s_before + (max_vel * t_phase) - (0.5f * max_accel * t_phase * t_phase);
    }
    
    return point;
}

/**
 * @brief Get next data point
 * 
 * Simulates realistic motor behavior with small tracking errors.
 */
TelemetryPoint MockDataGenerator::getNextPoint()
{
    if (!running) {
        // Return idle state
        TelemetryPoint point;
        point.time_ms = current_time * 1000.0f;
        point.target_position = 0.0f;
        point.actual_position = current_position;
        point.target_velocity = 0.0f;
        point.actual_velocity = 0.0f;
        point.acceleration = 0.0f;
        point.pid_output = 0.0f;
        point.phase = 0;
        return point;
    }
    
    // Get ideal target state
    TelemetryPoint target = computeStateAtTime(current_time);
    
    // Simulate actual position with realistic tracking
    // Good PID control: small lag during acceleration/deceleration
    float position_error = 0.0f;
    float velocity_error = 0.0f;
    
    if (target.phase == 1) {
        // Acceleration: small lag
        position_error = QRandomGenerator::global()->bounded(1, 3) + 
                        (QRandomGenerator::global()->generateDouble() * 0.5);
        velocity_error = QRandomGenerator::global()->bounded(-5, 5) + 
                        (QRandomGenerator::global()->generateDouble() - 0.5);
    } else if (target.phase == 2) {
        // Constant velocity: very good tracking
        position_error = QRandomGenerator::global()->bounded(-1, 1) + 
                        (QRandomGenerator::global()->generateDouble() * 0.5);
        velocity_error = QRandomGenerator::global()->bounded(-2, 2) + 
                        (QRandomGenerator::global()->generateDouble() - 0.5);
    } else if (target.phase == 3) {
        // Deceleration: small lag
        position_error = QRandomGenerator::global()->bounded(2, 4) + 
                        (QRandomGenerator::global()->generateDouble() * 0.5);
        velocity_error = QRandomGenerator::global()->bounded(-5, 5) + 
                        (QRandomGenerator::global()->generateDouble() - 0.5);
    }
    
    // Simulate PID output (feedforward + correction)
    float pid_output = (target.target_velocity * 0.8f) + (position_error * 1.5f);
    pid_output = std::max(-100.0f, std::min(100.0f, pid_output));
    
    // Build telemetry point
    TelemetryPoint point;
    point.time_ms = target.time_ms;
    point.target_position = target.target_position;
    point.actual_position = target.target_position - position_error;
    point.target_velocity = target.target_velocity;
    point.actual_velocity = target.target_velocity + velocity_error;
    point.acceleration = target.acceleration;
    point.pid_output = pid_output;
    point.phase = target.phase;
    
    // Update current state
    current_time += 0.05f;  // 50ms increment (20 Hz)
    current_position = point.actual_position;
    current_velocity = point.actual_velocity;
    current_phase = point.phase;
    
    // Check if motion complete
    if (current_time >= t_total) {
        stop();
    }
    
    return point;
}

