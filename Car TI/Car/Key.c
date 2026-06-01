#include "Key.h"

/*
 * 当前主程序选择逻辑：
 *
 * 上电后 3 秒内选择：
 * PA21 -> 第二问，接 GND，低电平有效
 * PA24 -> 第三问，接 GND，低电平有效
 * PA2  -> 第四问，接 3.3V，高电平有效
 *
 * 3 秒内不按任何按键，默认进入第一问。
 *
 * PA28 暂时不使用，保留接口。
 */

#define KEY1_PORT          GPIOA
#define KEY1_PIN           DL_GPIO_PIN_21

#define KEY2_PORT          GPIOA
#define KEY2_PIN           DL_GPIO_PIN_24

#define KEY3_PORT          GPIOA
#define KEY3_PIN           DL_GPIO_PIN_2

#define KEY4_PORT          GPIOA
#define KEY4_PIN           DL_GPIO_PIN_28

static void Key_Delay(void)
{
    volatile uint32_t i;

    for (i = 0; i < 60000; i++)
    {
        ;
    }
}

void Key_Init(void)
{
    /*
     * SysConfig 里配置：
     *
     * PA21：GPIO Input Pull-Up
     * PA24：GPIO Input Pull-Up
     * PA2 ：GPIO Input Pull-Down
     * PA28：GPIO Input Pull-Up，暂时不用
     */
}

uint8_t Key1_Read(void)
{
    /* PA21：第二问，接 GND，按下 = 0 */
    if ((DL_GPIO_readPins(KEY1_PORT, KEY1_PIN) & KEY1_PIN) == 0)
    {
        return KEY_PRESSED;
    }
    else
    {
        return KEY_RELEASED;
    }
}

uint8_t Key2_Read(void)
{
    /* PA24：第三问，接 GND，按下 = 0 */
    if ((DL_GPIO_readPins(KEY2_PORT, KEY2_PIN) & KEY2_PIN) == 0)
    {
        return KEY_PRESSED;
    }
    else
    {
        return KEY_RELEASED;
    }
}

uint8_t Key3_Read(void)
{
    /* PA2：第四问，接 3.3V，按下 = 1 */
    if ((DL_GPIO_readPins(KEY3_PORT, KEY3_PIN) & KEY3_PIN) != 0)
    {
        return KEY_PRESSED;
    }
    else
    {
        return KEY_RELEASED;
    }
}

uint8_t Key4_Read(void)
{
    /* PA28：暂时不用，接 GND，按下 = 0 */
    if ((DL_GPIO_readPins(KEY4_PORT, KEY4_PIN) & KEY4_PIN) == 0)
    {
        return KEY_PRESSED;
    }
    else
    {
        return KEY_RELEASED;
    }
}

uint8_t Key_Select_Mode(void)
{
    uint8_t mode = KEY_MODE_1;

    /*
     * 默认第一问。
     *
     * PA21 -> 第二问
     * PA24 -> 第三问
     * PA2  -> 第四问
     */

    Key_Delay();

    if (Key1_Read() == KEY_PRESSED)
    {
        Key_Delay();

        if (Key1_Read() == KEY_PRESSED)
        {
            mode = KEY_MODE_2;
        }
    }
    else if (Key2_Read() == KEY_PRESSED)
    {
        Key_Delay();

        if (Key2_Read() == KEY_PRESSED)
        {
            mode = KEY_MODE_3;
        }
    }
    else if (Key3_Read() == KEY_PRESSED)
    {
        Key_Delay();

        if (Key3_Read() == KEY_PRESSED)
        {
            mode = KEY_MODE_4;
        }
    }

    return mode;
}
