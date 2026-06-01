#include "Sensor.h"
#include "ti_msp_dl_config.h"
#define GRAY_INVERT 0

void Gray_Init(void)
{
    // GPIO 已经在 SYSCFG_DL_init() 里初始化，这里不用写
}
static uint8_t Gray_Read_Pin(GPIO_Regs *port, uint32_t pin)
{
    uint8_t value;

    if (DL_GPIO_readPins(port, pin))
    {
        value = 1;
    }
    else
    {
        value = 0;
    }

#if GRAY_INVERT
    value = !value;
#endif

    return value;
}

uint8_t Gray_Read_L3(void)
{
    return Gray_Read_Pin(GRAY_L3_PORT, GRAY_L3_PIN_24_PIN);
}

uint8_t Gray_Read_L2(void)
{
    return Gray_Read_Pin(GRAY_L2_PORT, GRAY_L2_PIN_20_PIN);
}

uint8_t Gray_Read_L1(void)
{
    return Gray_Read_Pin(GRAY_L1_PORT, GRAY_L1_PIN_19_PIN);
}

uint8_t Gray_Read_M(void)
{
    return Gray_Read_Pin(GRAY_M_PORT, GRAY_M_PIN_18_PIN);
}

uint8_t Gray_Read_R1(void)
{
    return Gray_Read_Pin(GRAY_R1_PORT, GRAY_R1_PINA_18_PIN);//记得这个配置的时候名字是PINA_18
}

uint8_t Gray_Read_R2(void)
{
    return Gray_Read_Pin(GRAY_R2_PORT, GRAY_R2_PIN_17_PIN);
}

uint8_t Gray_Read_R3(void)
{
    return Gray_Read_Pin(GRAY_R3_PORT, GRAY_R3_PIN_16_PIN);
}
uint8_t Gray_Read_All(void)
{
    uint8_t value = 0;

    value |= Gray_Read_L3() << 6;
    value |= Gray_Read_L2() << 5;
    value |= Gray_Read_L1() << 4;
    value |= Gray_Read_M()  << 3;
    value |= Gray_Read_R1() << 2;
    value |= Gray_Read_R2() << 1;
    value |= Gray_Read_R3() << 0;

    return value;
}