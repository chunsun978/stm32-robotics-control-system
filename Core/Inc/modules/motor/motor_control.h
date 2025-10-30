/**
 * @file motor_control.h
 * @brief Motor control interface
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Motor control test/main function
 */
void motor_control_main(void);

/**
 * @brief Enable or disable the motor
 * @param enable true to enable, false to disable
 */
void motor_enable(bool enable);

/**
 * @brief Set motor direction
 * @param forward true for forward, false for reverse
 */
void motor_set_direction(bool forward);

/**
 * @brief Set motor speed in steps per second
 * @param steps_per_second Desired speed
 */
void motor_set_speed(float steps_per_second);

#ifdef __cplusplus
}
#endif

#endif // MOTOR_CONTROL_H

