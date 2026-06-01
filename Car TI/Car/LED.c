#include "LED.h"
#include "ti_msp_dl_config.h"

#define USER_LED_PORT    GPIOA
#define USER_LED_PIN     DL_GPIO_PIN_12

void LED_Init(void)
{
    // GPIO 已经在 SYSCFG_DL_init() 里初始化
    // 低电平亮，所以初始化默认关灯
    LED_Off();
}

void LED_On(void)
{
    // 低电平触发：拉低亮灯
    DL_GPIO_clearPins(USER_LED_PORT, USER_LED_PIN);
}

void LED_Off(void)
{
    // 拉高灭灯
    DL_GPIO_setPins(USER_LED_PORT, USER_LED_PIN);
}

void LED_Toggle(void)
{
    DL_GPIO_togglePins(USER_LED_PORT, USER_LED_PIN);
}