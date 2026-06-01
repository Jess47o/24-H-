#include "PWM.h"
#include "ti_msp_dl_config.h"

#define PWM_PERIOD 1000

static uint16_t Speed_To_Compare(int16_t speed)
{
    if (speed < 0)
    {
        speed = 0;
    }

    if (speed > PWM_PERIOD)
    {
        speed = PWM_PERIOD;
    }

    return PWM_PERIOD - speed;
}

void Motor_Init(void)
{
    DL_TimerA_startCounter(PWM_0_INST);
    Motor_Stop();
}

void Motor_SetSpeed(int16_t left_speed, int16_t right_speed)
{
    /*
     * 左电机方向：AIN1=1, AIN2=0
     * PB2 / PB3
     */
    DL_GPIO_setPins(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_2_PIN);
    DL_GPIO_clearPins(GPIO_GRP_1_PORT, GPIO_GRP_1_PIN_3_PIN);

    /*
     * 右电机方向：BIN1=1, BIN2=0
     * PB6 / PB7
     */
    DL_GPIO_setPins(GPIO_GRP_2_PORT, GPIO_GRP_2_PIN_6_PIN);
    DL_GPIO_clearPins(GPIO_GRP_3_PORT, GPIO_GRP_3_PIN_7_PIN);

    // 左电机 PWM：PA8 / Channel 0
    DL_TimerA_setCaptureCompareValue(
        PWM_0_INST,
        Speed_To_Compare(left_speed),
        GPIO_PWM_0_C0_IDX
    );

    // 右电机 PWM：PA9 / Channel 1
    DL_TimerA_setCaptureCompareValue(
        PWM_0_INST,
        Speed_To_Compare(right_speed),
        GPIO_PWM_0_C1_IDX
    );
}

void Motor_Forward(uint16_t speed)
{
    Motor_SetSpeed(speed, speed);
}

void Motor_Stop(void)
{
    DL_TimerA_setCaptureCompareValue(
        PWM_0_INST,
        PWM_PERIOD,
        GPIO_PWM_0_C0_IDX
    );

    DL_TimerA_setCaptureCompareValue(
        PWM_0_INST,
        PWM_PERIOD,
        GPIO_PWM_0_C1_IDX
    );

    DL_GPIO_clearPins(GPIO_GRP_0_PORT, GPIO_GRP_0_PIN_2_PIN);
    DL_GPIO_clearPins(GPIO_GRP_1_PORT, GPIO_GRP_1_PIN_3_PIN);
    DL_GPIO_clearPins(GPIO_GRP_2_PORT, GPIO_GRP_2_PIN_6_PIN);
    DL_GPIO_clearPins(GPIO_GRP_3_PORT, GPIO_GRP_3_PIN_7_PIN);
}