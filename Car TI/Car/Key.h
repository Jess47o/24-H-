#ifndef __KEY_H__
#define __KEY_H__

#include "ti_msp_dl_config.h"
#include <stdint.h>

#define KEY_NONE        0
#define KEY_MODE_1      1
#define KEY_MODE_2      2
#define KEY_MODE_3      3
#define KEY_MODE_4      4

#define KEY_PRESSED     1
#define KEY_RELEASED    0

void Key_Init(void);

uint8_t Key1_Read(void);
uint8_t Key2_Read(void);
uint8_t Key3_Read(void);
uint8_t Key4_Read(void);

uint8_t Key_Select_Mode(void);

#endif