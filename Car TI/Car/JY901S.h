#ifndef JY901S_H
#define JY901S_H

#include <stdint.h>

void JY901S_Init(void);
void JY901S_Update(void);
uint8_t JY901S_HasYaw(void);
float JY901S_GetYaw(void);
void JY901S_SetYawZero(void);
float JY901S_GetYawError(void);

#endif