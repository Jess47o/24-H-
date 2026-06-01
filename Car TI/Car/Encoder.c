#include "Encoder.h"
#include "ti_msp_dl_config.h"

#define ENCODER_PORT       GPIOA

#define LEFT_A_PIN         DL_GPIO_PIN_0
#define LEFT_B_PIN         DL_GPIO_PIN_1

#define RIGHT_A_PIN        DL_GPIO_PIN_26
#define RIGHT_B_PIN        DL_GPIO_PIN_27

volatile int32_t left_encoder_count = 0;
volatile int32_t right_encoder_count = 0;

void Encoder_Init(void)
{
    /*
     * 四个编码器引脚都在 PORTA
     * 所以开 GPIOA 中断即可
     */
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
}

int32_t Encoder_GetLeft(void)
{
    return left_encoder_count;
}

int32_t Encoder_GetRight(void)
{
    return right_encoder_count;
}

void Encoder_Clear(void)
{
    left_encoder_count = 0;
    right_encoder_count = 0;
}

/*
 * 注意：整个工程里面只能有一个 GROUP1_IRQHandler
 */
void GROUP1_IRQHandler(void)
{
    uint32_t status;

    status = DL_GPIO_getEnabledInterruptStatus(
        ENCODER_PORT,
        LEFT_A_PIN | RIGHT_A_PIN
    );

    // 左编码器 A 相 PA0 触发
    if (status & LEFT_A_PIN)
    {
        if (DL_GPIO_readPins(ENCODER_PORT, LEFT_B_PIN))
        {
            left_encoder_count++;
        }
        else
        {
            left_encoder_count--;
        }

        DL_GPIO_clearInterruptStatus(ENCODER_PORT, LEFT_A_PIN);
    }

    // 右编码器 A 相 PA26 触发
    if (status & RIGHT_A_PIN)
    {
        if (DL_GPIO_readPins(ENCODER_PORT, RIGHT_B_PIN))
        {
            right_encoder_count++;
        }
        else
        {
            right_encoder_count--;
        }

        DL_GPIO_clearInterruptStatus(ENCODER_PORT, RIGHT_A_PIN);
    }
}