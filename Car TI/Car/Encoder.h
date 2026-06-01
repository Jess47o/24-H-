#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

extern volatile int32_t left_encoder_count;
extern volatile int32_t right_encoder_count;

void Encoder_Init(void);
int32_t Encoder_GetLeft(void);
int32_t Encoder_GetRight(void);
void Encoder_Clear(void);

#endif