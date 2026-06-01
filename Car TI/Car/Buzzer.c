#include "Buzzer.h"
#include "ti_msp_dl_config.h"

#define USER_BUZZER_PORT    GPIOA
#define USER_BUZZER_PIN     DL_GPIO_PIN_13

void Buzzer_Init(void)
{
    Buzzer_Off();
}

void Buzzer_On(void)
{
    // 低电平触发：拉低响
    DL_GPIO_clearPins(USER_BUZZER_PORT, USER_BUZZER_PIN);
}

void Buzzer_Off(void)
{
    // 拉高不响
    DL_GPIO_setPins(USER_BUZZER_PORT, USER_BUZZER_PIN);
}

void Buzzer_Toggle(void)
{
    DL_GPIO_togglePins(USER_BUZZER_PORT, USER_BUZZER_PIN);
}

void Buzzer_Beep(void)
{
    Buzzer_On();
    DL_Common_delayCycles(3200000);
    Buzzer_Off();
}