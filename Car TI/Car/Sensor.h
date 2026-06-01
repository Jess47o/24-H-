#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>

void Gray_Init(void);

uint8_t Gray_Read_L3(void);
uint8_t Gray_Read_L2(void);
uint8_t Gray_Read_L1(void);
uint8_t Gray_Read_M(void);
uint8_t Gray_Read_R1(void);
uint8_t Gray_Read_R2(void);
uint8_t Gray_Read_R3(void);

uint8_t Gray_Read_All(void);

#endif