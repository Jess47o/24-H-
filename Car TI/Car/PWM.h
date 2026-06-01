#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void Motor_Init(void);
void Motor_Forward(uint16_t speed);
void Motor_SetSpeed(int16_t left_speed, int16_t right_speed);
void Motor_Stop(void);

#endif