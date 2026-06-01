#include "JY901S.h"
#include "ti_msp_dl_config.h"

static float g_yaw_deg = 0.0f;
static float g_yaw_zero_deg = 0.0f;
static uint8_t g_has_yaw = 0;

static int16_t Make_Int16(uint8_t low, uint8_t high)
{
    return (int16_t)((uint16_t)low | ((uint16_t)high << 8));
}

static float Normalize_Angle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static void Parse_Byte(uint8_t data)
{
    static uint8_t frame[11];
    static uint8_t index = 0;
    uint8_t sum = 0;

    if (index == 0 && data != 0x55) return;

    if (index == 1 && data != 0x53)
    {
        index = 0;
        return;
    }

    frame[index++] = data;

    if (index < 11) return;

    for (uint8_t i = 0; i < 10; i++)
    {
        sum += frame[i];
    }

    if (sum == frame[10])
    {
        int16_t yaw_raw = Make_Int16(frame[6], frame[7]);
        g_yaw_deg = (float)yaw_raw / 32768.0f * 180.0f;
        g_has_yaw = 1;
    }

    index = 0;
}

void JY901S_Init(void)
{
    g_yaw_deg = 0.0f;
    g_yaw_zero_deg = 0.0f;
    g_has_yaw = 0;
}

void JY901S_Update(void)
{
    uint16_t limit = 64;

    while ((!DL_UART_Main_isRXFIFOEmpty(UART_2_INST)) && (limit > 0))
    {
        Parse_Byte(DL_UART_Main_receiveData(UART_2_INST));
        limit--;
    }
}

uint8_t JY901S_HasYaw(void)
{
    return g_has_yaw;
}

float JY901S_GetYaw(void)
{
    return g_yaw_deg;
}

void JY901S_SetYawZero(void)
{
    g_yaw_zero_deg = g_yaw_deg;
}

float JY901S_GetYawError(void)
{
    return Normalize_Angle(g_yaw_deg - g_yaw_zero_deg);
}