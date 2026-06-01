#include "ti_msp_dl_config.h"
#include "JY901S.h"
#include "Sensor.h"
#include "PWM.h"
#include "LED.h"
#include "Buzzer.h"
#include "Encoder.h"
#include "Key.h"
#include <stdint.h>

/*
 * 一个程序跑四个问题：
 *
 * 上电后 3 秒内选择：
 * PA21 -> 第二问
 * PA24 -> 第三问
 * PA2  -> 第四问
 * PA28 不使用
 *
 * 3 秒内不按任何按键，自动进入第一问。
 */

/* ===================== 全局毫秒计时 ===================== */

static volatile uint32_t g_ms = 0;

void SysTick_Handler(void)
{
    g_ms++;
}

static void Delay_ms(uint32_t ms)
{
    uint32_t start;

    start = g_ms;

    while ((g_ms - start) < ms)
    {
        JY901S_Update();
    }
}

/* ===================== 通用工具函数 ===================== */

static int32_t Abs32(int32_t x)
{
    if (x < 0)
    {
        return -x;
    }

    return x;
}

static float AbsFloat(float x)
{
    if (x < 0.0f)
    {
        return -x;
    }

    return x;
}

static int16_t Limit_Int16(int32_t value, int16_t min, int16_t max)
{
    if (value > max)
    {
        return max;
    }

    if (value < min)
    {
        return min;
    }

    return (int16_t)value;
}

static float Normalize_Angle(float angle)
{
    while (angle > 180.0f)
    {
        angle -= 360.0f;
    }

    while (angle < -180.0f)
    {
        angle += 360.0f;
    }

    return angle;
}

static int32_t Get_Avg_Encoder(void)
{
    int32_t left;
    int32_t right;

    left = Abs32(Encoder_GetLeft());
    right = Abs32(Encoder_GetRight());

    return (left + right) / 2;
}

static uint8_t Count_Gray_Hit(uint8_t gray)
{
    uint8_t count;
    uint8_t i;

    count = 0;

    for (i = 0; i < 7; i++)
    {
        if (gray & (1 << i))
        {
            count++;
        }
    }

    return count;
}

static int8_t Get_Line_Error(uint8_t gray)
{
    int16_t sum;
    int8_t count;

    sum = 0;
    count = 0;

    if (gray & (1 << 6)) { sum += -3; count++; }
    if (gray & (1 << 5)) { sum += -2; count++; }
    if (gray & (1 << 4)) { sum += -1; count++; }
    if (gray & (1 << 3)) { sum +=  0; count++; }
    if (gray & (1 << 2)) { sum +=  1; count++; }
    if (gray & (1 << 1)) { sum +=  2; count++; }
    if (gray & (1 << 0)) { sum +=  3; count++; }

    if (count == 0)
    {
        return 0;
    }

    return (int8_t)(sum / count);
}

/* ===================== 声光提示 ===================== */

static void Alert_Once(void)
{
    Motor_Stop();

    LED_On();
    Buzzer_On();
    Delay_ms(120);

    LED_Off();
    Buzzer_Off();
    Delay_ms(80);
}

/* ===================== Debug 变量 ===================== */

volatile uint8_t debug_state = 0;
volatile uint8_t debug_lap_count = 0;

volatile uint8_t debug_gray = 0;
volatile uint8_t debug_line_hit_count = 0;

volatile int8_t debug_line_error = 0;
volatile int8_t debug_line_d_error = 0;

volatile int16_t debug_left_speed = 0;
volatile int16_t debug_right_speed = 0;

volatile int32_t debug_left_encoder = 0;
volatile int32_t debug_right_encoder = 0;
volatile int32_t debug_avg_encoder = 0;

volatile float debug_yaw = 0.0f;
volatile float debug_relative_yaw = 0.0f;
volatile float debug_align_error = 0.0f;

/* =========================================================
 * 第一问：PA21
 * A -> B，无黑线直行，检测到 B 点黑线后停车
 * ========================================================= */

#define Q1_BASE_SPEED             300
#define Q1_MAX_TIME_MS            15000U
#define Q1_ENCODER_KP             2
#define Q1_MAX_CORRECTION         120
#define Q1_END_LINE_COUNT         1U
#define Q1_END_CONFIRM_COUNT      3U

static uint8_t q1_arrive_confirm = 0;

static uint8_t Q1_Is_Arrived_B(void)
{
    uint8_t gray;

    gray = Gray_Read_All();

    debug_gray = gray;
    debug_line_hit_count = Count_Gray_Hit(gray);

    if (debug_line_hit_count >= Q1_END_LINE_COUNT)
    {
        if (q1_arrive_confirm < Q1_END_CONFIRM_COUNT)
        {
            q1_arrive_confirm++;
        }
    }
    else
    {
        q1_arrive_confirm = 0;
    }

    if (q1_arrive_confirm >= Q1_END_CONFIRM_COUNT)
    {
        return 1;
    }

    return 0;
}

static void Q1_Straight_Run_By_Encoder(void)
{
    int32_t left_count;
    int32_t right_count;
    int32_t error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    left_count = Abs32(Encoder_GetLeft());
    right_count = Abs32(Encoder_GetRight());

    error = left_count - right_count;

    correction = error * Q1_ENCODER_KP;
    correction = Limit_Int16(correction, -Q1_MAX_CORRECTION, Q1_MAX_CORRECTION);

    left_speed = Q1_BASE_SPEED - correction;
    right_speed = Q1_BASE_SPEED + correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
    debug_left_encoder = Encoder_GetLeft();
    debug_right_encoder = Encoder_GetRight();
    debug_avg_encoder = Get_Avg_Encoder();
}

static void Q1_Arrive_Alert(void)
{
    uint8_t i;

    Motor_Stop();

    for (i = 0; i < 3; i++)
    {
        LED_On();
        Buzzer_On();
        Delay_ms(200);

        LED_Off();
        Buzzer_Off();
        Delay_ms(200);
    }
}

static void Run_Question1(void)
{
    uint32_t start_time;

    Encoder_Clear();

    q1_arrive_confirm = 0;
    start_time = g_ms;
    debug_state = 1;

    while (1)
    {
        if (Q1_Is_Arrived_B())
        {
            Q1_Arrive_Alert();

            while (1)
            {
                Motor_Stop();
            }
        }

        if ((g_ms - start_time) >= Q1_MAX_TIME_MS)
        {
            Q1_Arrive_Alert();

            while (1)
            {
                Motor_Stop();
            }
        }

        Q1_Straight_Run_By_Encoder();

        Delay_ms(10);
    }
}

/* =========================================================
 * 第二问：PA25
 * A->B，B->C，C->D，D->A
 * ========================================================= */

#define Q2_BASE_SPEED                  300
#define Q2_ARC_SPEED                   165
#define Q2_MAX_TIME_MS                 30000U

/*
 * 第二问 C->D 到达 D 点黑线后，
 * 先朝黑线方向小偏一点角度，再进入 D->A 循迹。
 */
#define Q2_D_BIAS_TIME_MS              160U
#define Q2_D_BIAS_SPEED                100
#define Q2_D_BIAS_TURN                 65

#define Q2_COUNTS_PER_CM               20
#define Q2_STRAIGHT_TO_ARC_MIN_CM      70
#define Q2_AB_BACKUP_DISTANCE_CM       90
#define Q2_CD_BACKUP_DISTANCE_CM       98
#define Q2_ARC_MIN_DISTANCE_CM         98
#define Q2_DA_ARC_MIN_DISTANCE_CM      98

#define Q2_C_OUT_BIAS_DEG              5.0f
#define Q2_ENCODER_KP                  1
#define Q2_YAW_KP                      2
#define Q2_MAX_ENCODER_CORRECTION      60
#define Q2_MAX_YAW_CORRECTION          50
#define Q2_MAX_STRAIGHT_CORRECTION     100


#define Q2_LINE_KP_ARC                 32
#define Q2_LINE_KD_ARC                 8
#define Q2_MAX_LINE_CORRECTION_ARC     90
#define Q2_LINE_ON_CONFIRM             2
#define Q2_LINE_LOST_CONFIRM           10U

#define Q2_ALIGN_FORWARD_SPEED         130
#define Q2_ALIGN_KP                    4
#define Q2_ALIGN_MAX_CORRECTION        32
#define Q2_ALIGN_OK_DEG                5.0f
#define Q2_ALIGN_MAX_TIME_MS           500
#define Q2_ALIGN_REVERSE               1

#define Q2_C_ALIGN_OK_DEG              2.0f
#define Q2_C_ALIGN_MAX_TIME_MS         1200
#define Q2_C_ALIGN_MIN_CORRECTION      12
#define Q2_C_ALIGN_MAX_CORRECTION      70
#define Q2_C_ALIGN_FORWARD_SPEED       75


typedef enum
{
    Q2_STATE_A_TO_B = 0,
    Q2_STATE_B_TO_C,
    Q2_STATE_C_ALIGN_TO_D,
    Q2_STATE_C_TO_D,
    Q2_STATE_D_BIAS_TO_LINE,
    Q2_STATE_D_TO_A,
    Q2_STATE_STOP
} Q2_State_t;

static Q2_State_t q2_state = Q2_STATE_A_TO_B;

static uint32_t q2_start_time = 0;
static uint32_t q2_state_time = 0;

static float q2_yaw_start = 0.0f;
static float q2_start_abs_yaw = 0.0f;
static uint8_t q2_line_on_count = 0;
static uint8_t q2_line_lost_count = 0;
static int8_t q2_last_line_error = 0;
static int32_t q2_arc_correction_last = 0;
static float q2_c_align_target = 180.0f;
static int8_t q2_d_entry_line_error = 0;

static uint8_t Q2_Distance_OK(uint16_t cm)
{
    int32_t target_count;

    target_count = (int32_t)cm * Q2_COUNTS_PER_CM;

    return (Get_Avg_Encoder() >= target_count);
}

static float Q2_Get_Relative_Yaw(void)
{
    JY901S_Update();

    if (!JY901S_HasYaw())
    {
        return 0.0f;
    }

    return Normalize_Angle(JY901S_GetYaw() - q2_yaw_start);
}

static void Q2_Begin_State(Q2_State_t next)
{
    q2_state = next;
    q2_state_time = g_ms;

    Encoder_Clear();

    q2_line_on_count = 0;
    q2_line_lost_count = 0;
    q2_last_line_error = 0;
    q2_arc_correction_last = 0;

    JY901S_Update();

    if (JY901S_HasYaw())
    {
        q2_yaw_start = JY901S_GetYaw();
    }

    debug_state = (uint8_t)q2_state;
}

static uint8_t Q2_Is_Line_Detected(void)
{
    uint8_t gray;

    gray = Gray_Read_All();

    debug_gray = gray;
    debug_line_hit_count = Count_Gray_Hit(gray);

    return (debug_line_hit_count >= 1);
}

static uint8_t Q2_Line_On_Stable(void)
{
    if (Q2_Is_Line_Detected())
    {
        if (q2_line_on_count < Q2_LINE_ON_CONFIRM)
        {
            q2_line_on_count++;
        }
    }
    else
    {
        q2_line_on_count = 0;
    }

    return (q2_line_on_count >= Q2_LINE_ON_CONFIRM);
}

static uint8_t Q2_Line_Lost_Stable(void)
{
    if (!Q2_Is_Line_Detected())
    {
        if (q2_line_lost_count < Q2_LINE_LOST_CONFIRM)
        {
            q2_line_lost_count++;
        }
    }
    else
    {
        q2_line_lost_count = 0;
    }

    return (q2_line_lost_count >= Q2_LINE_LOST_CONFIRM);
}

static uint8_t Q2_Straight_To_Arc_End_OK(uint16_t backup_cm)
{
    if (Q2_Distance_OK(Q2_STRAIGHT_TO_ARC_MIN_CM) && Q2_Line_On_Stable())
    {
        return 1;
    }

    if (Q2_Distance_OK(backup_cm))
    {
        return 1;
    }

    return 0;
}

static void Q2_Straight_Run_No_Line(uint16_t base_speed)
{
    int32_t left_count;
    int32_t right_count;
    int32_t encoder_error;
    int32_t encoder_correction;
    int32_t yaw_correction;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;
    float yaw_error;

    JY901S_Update();

    left_count = Abs32(Encoder_GetLeft());
    right_count = Abs32(Encoder_GetRight());

    encoder_error = left_count - right_count;
    encoder_correction = encoder_error * Q2_ENCODER_KP;

    encoder_correction = Limit_Int16(
        encoder_correction,
        -Q2_MAX_ENCODER_CORRECTION,
        Q2_MAX_ENCODER_CORRECTION
    );

    yaw_correction = 0;

    if (JY901S_HasYaw())
    {
        yaw_error = Q2_Get_Relative_Yaw();
        yaw_correction = (int32_t)(yaw_error * Q2_YAW_KP);

        yaw_correction = Limit_Int16(
            yaw_correction,
            -Q2_MAX_YAW_CORRECTION,
            Q2_MAX_YAW_CORRECTION
        );
    }

    correction = encoder_correction + yaw_correction;

    correction = Limit_Int16(
        correction,
        -Q2_MAX_STRAIGHT_CORRECTION,
        Q2_MAX_STRAIGHT_CORRECTION
    );

    left_speed = base_speed - correction;
    right_speed = base_speed + correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static void Q2_Line_Follow_RightArc(uint16_t base_speed)
{
    uint8_t gray;
    uint8_t hit_count;
    int8_t line_error;
    int8_t d_error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    gray = Gray_Read_All();
    hit_count = Count_Gray_Hit(gray);

    debug_gray = gray;
    debug_line_hit_count = hit_count;

    /*
     * 更稳的弧线灰度循迹：
     * 1. KP 从 60 降到 45，避免左右猛摆；
     * 2. 加 KD，抑制快速抖动；
     * 3. 加 correction 滤波；
     * 4. 瞬间丢线时沿用上一次误差，不让车头突然回正冲出去。
     */
    if (hit_count == 0)
    {
        line_error = q2_last_line_error;
    }
    else
    {
        line_error = Get_Line_Error(gray);
    }

    d_error = line_error - q2_last_line_error;

    correction = (int32_t)line_error * Q2_LINE_KP_ARC +
                 (int32_t)d_error * Q2_LINE_KD_ARC;

    q2_last_line_error = line_error;

    correction = Limit_Int16(
        correction,
        -Q2_MAX_LINE_CORRECTION_ARC,
        Q2_MAX_LINE_CORRECTION_ARC
    );

    /*
     * 低通滤波，减少灰度循迹抖动，让到 C 点的位置更稳定。
     */
    correction = (q2_arc_correction_last * 4 + correction) / 5;
    q2_arc_correction_last = correction;

    left_speed = base_speed + correction;
    right_speed = base_speed - correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_line_error = line_error;
    debug_line_d_error = d_error;
    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static uint8_t Q2_Arc_End_By_LineLost_OK(uint16_t min_distance_cm)
{
    if (Q2_Distance_OK(min_distance_cm) && Q2_Line_Lost_Stable())
    {
        return 1;
    }

    return 0;
}

static void Q2_Save_D_Line_Error(void)
{
    uint8_t gray;
    uint8_t hit_count;

    gray = Gray_Read_All();
    hit_count = Count_Gray_Hit(gray);

    debug_gray = gray;
    debug_line_hit_count = hit_count;

    /*
     * 记录到达 D 点瞬间黑线在车身哪一侧。
     *
     * line_error < 0：黑线偏左，后面小角度向左偏；
     * line_error > 0：黑线偏右，后面小角度向右偏；
     * line_error = 0：黑线基本在中间，短暂直走。
     */
    if (hit_count == 0)
    {
        q2_d_entry_line_error = q2_last_line_error;
    }
    else
    {
        q2_d_entry_line_error = Get_Line_Error(gray);
    }

    debug_line_error = q2_d_entry_line_error;
}

static void Q2_Bias_Towards_D_Line(void)
{
    int16_t left_speed;
    int16_t right_speed;

    /*
     * 到达 D 点后，不马上进入 D->A 循迹。
     * 先根据刚碰到 D 点时的灰度方向，让车头朝黑线方向轻微偏一点。
     */
    if (q2_d_entry_line_error < 0)
    {
        /*
         * 黑线在左边：左轮慢，右轮快，车头向左偏。
         */
        left_speed = Q2_D_BIAS_SPEED - Q2_D_BIAS_TURN;
        right_speed = Q2_D_BIAS_SPEED + Q2_D_BIAS_TURN;
    }
    else if (q2_d_entry_line_error > 0)
    {
        /*
         * 黑线在右边：左轮快，右轮慢，车头向右偏。
         */
        left_speed = Q2_D_BIAS_SPEED + Q2_D_BIAS_TURN;
        right_speed = Q2_D_BIAS_SPEED - Q2_D_BIAS_TURN;
    }
    else
    {
        left_speed = Q2_D_BIAS_SPEED;
        right_speed = Q2_D_BIAS_SPEED;
    }

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static void Q2_Set_C_Align_Target(void)
{
    /*
     * C 点出弯后，不再根据“当前灰度循迹出来的相对角度”决定目标。
     * 目标固定为：第二问刚开始 A->B 的绝对航向 + 180 度。
     *
     * 这样每次出 C 的角度就不会被灰度循迹误差带偏，
     * C->D 会始终瞄同一个绝对方向。
     */
    q2_c_align_target = Normalize_Angle(
        q2_start_abs_yaw + 180.0f + Q2_C_OUT_BIAS_DEG
    );
}

static uint8_t Q2_Align_To_Arc_180(void)
{
    float yaw;
    float target;
    float error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    yaw = Q2_Get_Relative_Yaw();
    target = q2_c_align_target;

    error = Normalize_Angle(target - yaw);
    debug_align_error = error;

    if (AbsFloat(error) <= Q2_ALIGN_OK_DEG)
    {
        Motor_Stop();
        return 1;
    }

    if ((g_ms - q2_state_time) >= Q2_ALIGN_MAX_TIME_MS)
    {
        Motor_Stop();
        return 1;
    }

    correction = (int32_t)(error * Q2_ALIGN_KP);

    correction = Limit_Int16(
        correction,
        -Q2_ALIGN_MAX_CORRECTION,
        Q2_ALIGN_MAX_CORRECTION
    );

#if Q2_ALIGN_REVERSE
    left_speed  = Q2_ALIGN_FORWARD_SPEED - correction;
    right_speed = Q2_ALIGN_FORWARD_SPEED + correction;
#else
    left_speed  = Q2_ALIGN_FORWARD_SPEED + correction;
    right_speed = Q2_ALIGN_FORWARD_SPEED - correction;
#endif

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    return 0;
}

static uint8_t Q2_Align_C_To_D_Straight(void)
{
    float yaw;
    float target;
    float error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    /*
     * 用绝对 yaw 对准 C->D。
     * 不能再用 Q2_Get_Relative_Yaw()，否则会受 B->C 灰度循迹结束角度影响。
     */
    if (JY901S_HasYaw())
    {
        yaw = Normalize_Angle(JY901S_GetYaw());
    }
    else
    {
        yaw = 0.0f;
    }

    target = q2_c_align_target;
    error = Normalize_Angle(target - yaw);

    debug_align_error = error;

    if (AbsFloat(error) <= Q2_C_ALIGN_OK_DEG)
    {
        Motor_Stop();
        return 1;
    }

    if ((g_ms - q2_state_time) >= Q2_C_ALIGN_MAX_TIME_MS)
    {
        Motor_Stop();
        return 1;
    }

    correction = (int32_t)(error * Q2_ALIGN_KP);

    if ((correction > 0) && (correction < Q2_C_ALIGN_MIN_CORRECTION))
    {
        correction = Q2_C_ALIGN_MIN_CORRECTION;
    }
    else if ((correction < 0) && (correction > -Q2_C_ALIGN_MIN_CORRECTION))
    {
        correction = -Q2_C_ALIGN_MIN_CORRECTION;
    }

    correction = Limit_Int16(
        correction,
        -Q2_C_ALIGN_MAX_CORRECTION,
        Q2_C_ALIGN_MAX_CORRECTION
    );

#if Q2_ALIGN_REVERSE
    left_speed  = Q2_C_ALIGN_FORWARD_SPEED - correction;
    right_speed = Q2_C_ALIGN_FORWARD_SPEED + correction;
#else
    left_speed  = Q2_C_ALIGN_FORWARD_SPEED + correction;
    right_speed = Q2_C_ALIGN_FORWARD_SPEED - correction;
#endif

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    return 0;
}


static void Q2_Debug_Update(void)
{
    JY901S_Update();

    debug_state = (uint8_t)q2_state;

    debug_left_encoder = Encoder_GetLeft();
    debug_right_encoder = Encoder_GetRight();
    debug_avg_encoder = Get_Avg_Encoder();

    if (JY901S_HasYaw())
    {
        debug_yaw = JY901S_GetYaw();
        debug_relative_yaw = Q2_Get_Relative_Yaw();
    }
}

static void Run_Question2(void)
{
    uint16_t i;

    Encoder_Clear();

    for (i = 0; i < 100; i++)
    {
        JY901S_Update();
        Delay_ms(5);
    }

    if (JY901S_HasYaw())
    {
        q2_yaw_start = JY901S_GetYaw();
        q2_start_abs_yaw = q2_yaw_start;
    }
    else
    {
        q2_yaw_start = 0.0f;
        q2_start_abs_yaw = 0.0f;
    }

    q2_start_time = g_ms;
    q2_state_time = g_ms;
    q2_state = Q2_STATE_A_TO_B;
    q2_line_on_count = 0;
    q2_line_lost_count = 0;
    q2_last_line_error = 0;
    q2_arc_correction_last = 0;

    debug_state = (uint8_t)q2_state;

    while (1)
    {
        JY901S_Update();
        Q2_Debug_Update();

        if ((q2_state != Q2_STATE_STOP) &&
            ((g_ms - q2_start_time) >= Q2_MAX_TIME_MS))
        {
            Motor_Stop();
            Alert_Once();
            q2_state = Q2_STATE_STOP;
        }

        switch (q2_state)
        {
            case Q2_STATE_A_TO_B:
                Q2_Straight_Run_No_Line(Q2_BASE_SPEED);

                if (Q2_Straight_To_Arc_End_OK(Q2_AB_BACKUP_DISTANCE_CM))
                {
                    Alert_Once();
                    Q2_Begin_State(Q2_STATE_B_TO_C);
                }
                break;

            case Q2_STATE_B_TO_C:
                Q2_Line_Follow_RightArc(Q2_ARC_SPEED);

                if (Q2_Arc_End_By_LineLost_OK(Q2_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();

                    Q2_Set_C_Align_Target();

                    q2_state = Q2_STATE_C_ALIGN_TO_D;
                    q2_state_time = g_ms;
                    debug_state = (uint8_t)q2_state;
                }
                break;

            case Q2_STATE_C_ALIGN_TO_D:
                if (Q2_Align_C_To_D_Straight())
                {
                    Q2_Begin_State(Q2_STATE_C_TO_D);
                }
                break;

            case Q2_STATE_C_TO_D:
                Q2_Straight_Run_No_Line(Q2_BASE_SPEED);

                /*
                 * C->D 过程中，只要稳定碰到 D 点黑线，
                 * 就先声光提示，然后朝黑线方向小偏一点角度，
                 * 最后再进入 D->A 灰度循迹。
                 */
                if (Q2_Line_On_Stable())
                {
                    Q2_Save_D_Line_Error();
                    Alert_Once();

                    q2_state = Q2_STATE_D_BIAS_TO_LINE;
                    q2_state_time = g_ms;
                    debug_state = (uint8_t)q2_state;
                }
                break;

            case Q2_STATE_D_BIAS_TO_LINE:
                Q2_Bias_Towards_D_Line();

                if ((g_ms - q2_state_time) >= Q2_D_BIAS_TIME_MS)
                {
                    Q2_Begin_State(Q2_STATE_D_TO_A);
                }
                break;

            case Q2_STATE_D_TO_A:
                Q2_Line_Follow_RightArc(Q2_ARC_SPEED);

                if (Q2_Arc_End_By_LineLost_OK(Q2_DA_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();
                    q2_state = Q2_STATE_STOP;
                    debug_state = (uint8_t)q2_state;
                }
                break;

            case Q2_STATE_STOP:
            default:
                Motor_Stop();
                break;
        }

        Delay_ms(10);
    }
}

/* =========================================================
 * 第三问：PA2
 * A->C，C->B，B->D，D->A，一圈
 * ========================================================= */

#define Q3_BASE_SPEED                  300
#define Q3_ARC_SPEED                   230
#define Q3_MAX_TIME_MS                 40000U

#define Q3_COUNTS_PER_CM               20
#define Q3_DIAGONAL_TO_ARC_MIN_CM      75
#define Q3_AC_BACKUP_DISTANCE_CM       92
#define Q3_BD_BACKUP_DISTANCE_CM       93
#define Q3_CB_ARC_MIN_DISTANCE_CM      90
#define Q3_DA_ARC_MIN_DISTANCE_CM      88

#define Q3_A_TO_C_TARGET_DEG           -38.0f
#define Q3_C_TO_B_TARGET_DEG           60.0f
#define Q3_C_OUT_BIAS_DEG              0.0f
#define Q3_B_OUT_BIAS_DEG              38.0f
#define Q3_D_TO_A_TARGET_DEG           -51.0f

#define Q3_ENCODER_KP                  1
#define Q3_YAW_KP                      2
#define Q3_MAX_ENCODER_CORRECTION      60
#define Q3_MAX_YAW_CORRECTION          50
#define Q3_MAX_STRAIGHT_CORRECTION     100

#define Q3_LINE_KP_ARC                 60
#define Q3_MAX_LINE_CORRECTION_ARC     180
#define Q3_LINE_ON_CONFIRM             2
#define Q3_LINE_LOST_CONFIRM           5U
#define Q3_CB_LINE_REVERSE             0
#define Q3_DA_LINE_REVERSE             0

#define Q3_ALIGN_FORWARD_SPEED         130
#define Q3_ALIGN_KP                    4
#define Q3_ALIGN_MAX_CORRECTION        60
#define Q3_ALIGN_OK_DEG                5.0f
#define Q3_ALIGN_MAX_TIME_MS           1050
#define Q3_ALIGN_REVERSE               1

#define Q3_B_ALIGN_MAX_TIME_MS         1000
#define Q3_B_ALIGN_MAX_CORRECTION      60

typedef enum
{
    Q3_STATE_A_ALIGN_TO_C = 0,
    Q3_STATE_A_TO_C,
    Q3_STATE_C_ALIGN_TO_B,
    Q3_STATE_C_TO_B,
    Q3_STATE_B_ALIGN_TO_D,
    Q3_STATE_B_TO_D,
    Q3_STATE_D_ALIGN_TO_A,
    Q3_STATE_D_TO_A,
    Q3_STATE_STOP
} Q3_State_t;

static Q3_State_t q3_state = Q3_STATE_A_ALIGN_TO_C;

static uint32_t q3_start_time = 0;
static uint32_t q3_state_time = 0;

static float q3_yaw_start = 0.0f;
static float q3_align_target = 0.0f;
static float q3_c_entry_abs_yaw = 0.0f;
static float q3_c_align_target_abs = 0.0f;

static uint8_t q3_line_on_count = 0;
static uint8_t q3_line_lost_count = 0;
static int8_t q3_last_line_error = 0;

static uint8_t Q3_Distance_OK(uint16_t cm)
{
    int32_t target_count;

    target_count = (int32_t)cm * Q3_COUNTS_PER_CM;

    return (Get_Avg_Encoder() >= target_count);
}

static float Q3_Get_Relative_Yaw(void)
{
    JY901S_Update();

    if (!JY901S_HasYaw())
    {
        return 0.0f;
    }

    return Normalize_Angle(JY901S_GetYaw() - q3_yaw_start);
}

static void Q3_Begin_State(Q3_State_t next)
{
    q3_state = next;
    q3_state_time = g_ms;

    Encoder_Clear();

    q3_line_on_count = 0;
    q3_line_lost_count = 0;
    q3_last_line_error = 0;

    JY901S_Update();

    if (JY901S_HasYaw())
    {
        q3_yaw_start = JY901S_GetYaw();
    }

    debug_state = (uint8_t)q3_state;
}

static uint8_t Q3_Is_Line_Detected(void)
{
    uint8_t gray;

    gray = Gray_Read_All();

    debug_gray = gray;
    debug_line_hit_count = Count_Gray_Hit(gray);

    return (debug_line_hit_count >= 1);
}

static uint8_t Q3_Line_On_Stable(void)
{
    if (Q3_Is_Line_Detected())
    {
        if (q3_line_on_count < Q3_LINE_ON_CONFIRM)
        {
            q3_line_on_count++;
        }
    }
    else
    {
        q3_line_on_count = 0;
    }

    return (q3_line_on_count >= Q3_LINE_ON_CONFIRM);
}

static uint8_t Q3_Line_Lost_Stable(void)
{
    if (!Q3_Is_Line_Detected())
    {
        if (q3_line_lost_count < Q3_LINE_LOST_CONFIRM)
        {
            q3_line_lost_count++;
        }
    }
    else
    {
        q3_line_lost_count = 0;
    }

    return (q3_line_lost_count >= Q3_LINE_LOST_CONFIRM);
}

static uint8_t Q3_Diagonal_To_Arc_End_OK(uint16_t backup_cm)
{
    (void)backup_cm;

    if (Q3_Distance_OK(Q3_DIAGONAL_TO_ARC_MIN_CM) && Q3_Line_On_Stable())
    {
        return 1;
    }

    return 0;
}

static uint8_t Q3_Arc_End_By_LineLost_OK(uint16_t min_cm)
{
    if (Q3_Distance_OK(min_cm) && Q3_Line_Lost_Stable())
    {
        return 1;
    }

    return 0;
}


static void Q3_Save_C_Entry_Yaw(void)
{
    /*
     * 第三问 A->C 快进入 C 点黑线时记录当前绝对 yaw。
     * 后面 C 点出弯不再完全依赖相对 yaw，
     * 而是按这个进入 C 前的方向计算固定出口方向。
     */
    JY901S_Update();

    if (JY901S_HasYaw())
    {
        q3_c_entry_abs_yaw = Normalize_Angle(JY901S_GetYaw());
    }
}

static void Q3_Set_C_Align_Target(void)
{
    /*
     * 第三问 C 点使用第二问同款思路：
     *
     * 进入 C 点黑线前的绝对方向
     * + C->B 需要转过去的角度
     * + 微调角度
     *
     * 这样出 C 点时不会被当前灰度循迹误差带偏。
     */
    q3_c_align_target_abs = Normalize_Angle(
        q3_c_entry_abs_yaw +
        Q3_C_TO_B_TARGET_DEG +
        Q3_C_OUT_BIAS_DEG
    );
}

static uint8_t Q3_Align_C_To_B_Absolute(void)
{
    float yaw;
    float error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    if (JY901S_HasYaw())
    {
        yaw = Normalize_Angle(JY901S_GetYaw());
    }
    else
    {
        yaw = 0.0f;
    }

    error = Normalize_Angle(q3_c_align_target_abs - yaw);
    debug_align_error = error;

    if (AbsFloat(error) <= Q3_ALIGN_OK_DEG)
    {
        Motor_Stop();
        return 1;
    }

    if ((g_ms - q3_state_time) >= Q3_ALIGN_MAX_TIME_MS)
    {
        Motor_Stop();
        return 1;
    }

    correction = (int32_t)(error * Q3_ALIGN_KP);

    correction = Limit_Int16(
        correction,
        -Q3_ALIGN_MAX_CORRECTION,
        Q3_ALIGN_MAX_CORRECTION
    );

#if Q3_ALIGN_REVERSE
    left_speed  = Q3_ALIGN_FORWARD_SPEED - correction;
    right_speed = Q3_ALIGN_FORWARD_SPEED + correction;
#else
    left_speed  = Q3_ALIGN_FORWARD_SPEED + correction;
    right_speed = Q3_ALIGN_FORWARD_SPEED - correction;
#endif

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    return 0;
}

static void Q3_Straight_Run_No_Line(uint16_t base_speed)
{
    int32_t left_count;
    int32_t right_count;
    int32_t encoder_error;
    int32_t encoder_correction;
    int32_t yaw_correction;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;
    float yaw_error;

    /*
     * 第三问 A->C、B->D 直线恢复为：
     * 编码器纠偏 + 陀螺仪 yaw 轻微保角。
     *
     * 原因：
     * 第三问的 C 点出口角度现在会记录进入 C 前的 yaw。
     * 如果直线完全不用 yaw，A->C 路上车头角度容易漂，
     * 进入 C 前记录到的角度也会变，后面的出 C 目标角就跟着变。
     */
    JY901S_Update();

    left_count = Abs32(Encoder_GetLeft());
    right_count = Abs32(Encoder_GetRight());

    encoder_error = left_count - right_count;
    encoder_correction = encoder_error * Q3_ENCODER_KP;

    encoder_correction = Limit_Int16(
        encoder_correction,
        -Q3_MAX_ENCODER_CORRECTION,
        Q3_MAX_ENCODER_CORRECTION
    );

    yaw_correction = 0;

    if (JY901S_HasYaw())
    {
        yaw_error = Q3_Get_Relative_Yaw();
        yaw_correction = (int32_t)(yaw_error * Q3_YAW_KP);

        yaw_correction = Limit_Int16(
            yaw_correction,
            -Q3_MAX_YAW_CORRECTION,
            Q3_MAX_YAW_CORRECTION
        );
    }

    correction = encoder_correction + yaw_correction;

    correction = Limit_Int16(
        correction,
        -Q3_MAX_STRAIGHT_CORRECTION,
        Q3_MAX_STRAIGHT_CORRECTION
    );

    left_speed = base_speed - correction;
    right_speed = base_speed + correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static void Q3_Line_Follow_Arc(uint16_t base_speed, uint8_t reverse)
{
    uint8_t gray;
    int8_t line_error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    gray = Gray_Read_All();

    debug_gray = gray;
    debug_line_hit_count = Count_Gray_Hit(gray);

    line_error = Get_Line_Error(gray);
    q3_last_line_error = line_error;

    correction = (int32_t)line_error * Q3_LINE_KP_ARC;

    correction = Limit_Int16(
        correction,
        -Q3_MAX_LINE_CORRECTION_ARC,
        Q3_MAX_LINE_CORRECTION_ARC
    );

    if (reverse)
    {
        left_speed = base_speed - correction;
        right_speed = base_speed + correction;
    }
    else
    {
        left_speed = base_speed + correction;
        right_speed = base_speed - correction;
    }

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_line_error = line_error;
    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static void Q3_Set_B_Align_Target(void)
{
    float yaw;

    yaw = Q3_Get_Relative_Yaw();

    if (yaw >= 0.0f)
    {
        q3_align_target = Normalize_Angle(180.0f + Q3_B_OUT_BIAS_DEG);
    }
    else
    {
        q3_align_target = Normalize_Angle(-180.0f - Q3_B_OUT_BIAS_DEG);
    }
}

static uint8_t Q3_Align_To_Target_Custom(uint32_t max_time_ms, int16_t max_correction)
{
    float yaw;
    float error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    yaw = Q3_Get_Relative_Yaw();

    error = Normalize_Angle(q3_align_target - yaw);
    debug_align_error = error;

    if (AbsFloat(error) <= Q3_ALIGN_OK_DEG)
    {
        Motor_Stop();
        return 1;
    }

    if ((g_ms - q3_state_time) >= max_time_ms)
    {
        Motor_Stop();
        return 1;
    }

    correction = (int32_t)(error * Q3_ALIGN_KP);

    correction = Limit_Int16(
        correction,
        -max_correction,
        max_correction
    );

#if Q3_ALIGN_REVERSE
    left_speed  = Q3_ALIGN_FORWARD_SPEED - correction;
    right_speed = Q3_ALIGN_FORWARD_SPEED + correction;
#else
    left_speed  = Q3_ALIGN_FORWARD_SPEED + correction;
    right_speed = Q3_ALIGN_FORWARD_SPEED - correction;
#endif

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    return 0;
}

static uint8_t Q3_Align_To_Target(void)
{
    return Q3_Align_To_Target_Custom(Q3_ALIGN_MAX_TIME_MS, Q3_ALIGN_MAX_CORRECTION);
}

static uint8_t Q3_Align_To_Target_B(void)
{
    return Q3_Align_To_Target_Custom(Q3_B_ALIGN_MAX_TIME_MS, Q3_B_ALIGN_MAX_CORRECTION);
}

static void Q3_Debug_Update(void)
{
    JY901S_Update();

    debug_state = (uint8_t)q3_state;

    debug_left_encoder = Encoder_GetLeft();
    debug_right_encoder = Encoder_GetRight();
    debug_avg_encoder = Get_Avg_Encoder();

    if (JY901S_HasYaw())
    {
        debug_yaw = JY901S_GetYaw();
        debug_relative_yaw = Q3_Get_Relative_Yaw();
    }
}

static void Run_Question3(void)
{
    uint16_t i;

    Encoder_Clear();

    for (i = 0; i < 100; i++)
    {
        JY901S_Update();
        Delay_ms(5);
    }

    if (JY901S_HasYaw())
    {
        q3_yaw_start = JY901S_GetYaw();
    }
    else
    {
        q3_yaw_start = 0.0f;
    }

    q3_start_time = g_ms;
    q3_state_time = g_ms;

    q3_state = Q3_STATE_A_ALIGN_TO_C;
    q3_line_on_count = 0;
    q3_line_lost_count = 0;
    q3_last_line_error = 0;

    debug_state = (uint8_t)q3_state;

    while (1)
    {
        JY901S_Update();
        Q3_Debug_Update();

        if ((q3_state != Q3_STATE_STOP) &&
            ((g_ms - q3_start_time) >= Q3_MAX_TIME_MS))
        {
            Motor_Stop();
            Alert_Once();
            q3_state = Q3_STATE_STOP;
            debug_state = (uint8_t)q3_state;
        }

        switch (q3_state)
        {
            case Q3_STATE_A_ALIGN_TO_C:
                q3_align_target = Q3_A_TO_C_TARGET_DEG;

                if (Q3_Align_To_Target())
                {
                    Q3_Begin_State(Q3_STATE_A_TO_C);
                }
                break;

            case Q3_STATE_A_TO_C:
                Q3_Straight_Run_No_Line(Q3_BASE_SPEED);

                if (Q3_Diagonal_To_Arc_End_OK(Q3_AC_BACKUP_DISTANCE_CM))
                {
                    Q3_Save_C_Entry_Yaw();
                    Alert_Once();
                    Q3_Begin_State(Q3_STATE_C_ALIGN_TO_B);
                    Q3_Set_C_Align_Target();
                }
                break;

            case Q3_STATE_C_ALIGN_TO_B:
                if (Q3_Align_C_To_B_Absolute())
                {
                    Q3_Begin_State(Q3_STATE_C_TO_B);
                }
                break;

            case Q3_STATE_C_TO_B:
                Q3_Line_Follow_Arc(Q3_ARC_SPEED, Q3_CB_LINE_REVERSE);

                if (Q3_Arc_End_By_LineLost_OK(Q3_CB_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();

                    Q3_Set_B_Align_Target();

                    q3_state = Q3_STATE_B_ALIGN_TO_D;
                    q3_state_time = g_ms;
                    debug_state = (uint8_t)q3_state;
                }
                break;

            case Q3_STATE_B_ALIGN_TO_D:
                if (Q3_Align_To_Target_B())
                {
                    Q3_Begin_State(Q3_STATE_B_TO_D);
                }
                break;

            case Q3_STATE_B_TO_D:
                Q3_Straight_Run_No_Line(Q3_BASE_SPEED);

                if (Q3_Diagonal_To_Arc_End_OK(Q3_BD_BACKUP_DISTANCE_CM))
                {
                    Alert_Once();
                    Q3_Begin_State(Q3_STATE_D_ALIGN_TO_A);
                }
                break;

            case Q3_STATE_D_ALIGN_TO_A:
                q3_align_target = Q3_D_TO_A_TARGET_DEG;

                if (Q3_Align_To_Target())
                {
                    Q3_Begin_State(Q3_STATE_D_TO_A);
                }
                break;

            case Q3_STATE_D_TO_A:
                Q3_Line_Follow_Arc(Q3_ARC_SPEED, Q3_DA_LINE_REVERSE);

                if (Q3_Arc_End_By_LineLost_OK(Q3_DA_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();
                    q3_state = Q3_STATE_STOP;
                    debug_state = (uint8_t)q3_state;
                }
                break;

            case Q3_STATE_STOP:
            default:
                Motor_Stop();
                break;
        }

        Delay_ms(10);
    }
}

/* =========================================================
 * 第四问：PA2
 * 固定初始 yaw + 捕线纠偏逻辑
 *
 * 一圈流程：
 * 1. 启动第四问时记录固定初始 yaw；
 * 2. A->C：先按 初始yaw-60° 走一段；
 * 3. A->C：再按 初始yaw+0° 扫 C 点黑线；
 * 4. 任意灰度扫到线后，不再继续直走，进入低速捕线纠偏；
 * 5. 线被拉到中间后，进入 C->B 正常灰度循迹；
 * 6. B->D：先按 初始yaw-120° 走一段；
 * 7. B->D：再按 初始yaw-180° 扫 D 点黑线；
 * 8. 任意灰度扫到线后，进入低速捕线纠偏；
 * 9. 线被拉到中间后，进入 D->A 正常灰度循迹；
 * 10. 循环 4 圈。
 *
 * 关键改动：
 * 扫到左边/右边外侧灰度后，不会继续按固定角度往前冲，
 * 而是立刻进入捕线纠偏，把黑线拉回中间再开始圆弧循迹。
 * ========================================================= */

#define Q4_BASE_SPEED                  300
#define Q4_ARC_SPEED                   180
#define Q4_MAX_TIME_MS                 180000U
#define Q4_TOTAL_LAPS                  4

#define Q4_COUNTS_PER_CM               20

#define Q4_AC_FIRST_DISTANCE_CM        65
#define Q4_BD_FIRST_DISTANCE_CM        68

#define Q4_CB_ARC_MIN_DISTANCE_CM      95
#define Q4_DA_ARC_MIN_DISTANCE_CM      95

#define Q4_A_TO_C_FIRST_DEG            -60.0f
#define Q4_A_TO_C_SCAN_DEG             0.0f
#define Q4_B_TO_D_FIRST_DEG            -120.0f
#define Q4_B_TO_D_SCAN_DEG             -180.0f

#define Q4_ENCODER_KP                  1
#define Q4_YAW_KP                      2
#define Q4_MAX_ENCODER_CORRECTION      60
#define Q4_MAX_YAW_CORRECTION          50
#define Q4_MAX_STRAIGHT_CORRECTION     100

/*
 * 斜线走完以后，摆正后去扫 C/D 灰线的直线段专用参数。
 * 这段不再完全只靠编码器，也不再强行追固定绝对角度；
 * 而是以“摆正完成那一刻的 yaw”为目标，做轻微陀螺仪保角。
 */
#define Q4_SCAN_SPEED                  260
#define Q4_SCAN_YAW_KP                 1
#define Q4_SCAN_MAX_YAW_CORRECTION     35
#define Q4_SCAN_MAX_CORRECTION         80

/*
 * 第四问正常灰度循迹：
 * 使用加权求和误差，参考你发来的 Trace_pid_test() 思路。
 */
#define Q4_LINE_KP_ARC                 8
#define Q4_LINE_KD_ARC                 2
#define Q4_MAX_LINE_CORRECTION_ARC     90
#define Q4_LINE_ON_CONFIRM             2
#define Q4_LINE_LOST_CONFIRM           8U
#define Q4_CB_LINE_REVERSE             0
#define Q4_DA_LINE_REVERSE             0

/*
 * 捕线纠偏：
 * 任意灰度扫到黑线后，不继续固定角度直走；
 * 先低速按灰度误差修正，把线拉到 L1/M/R1 附近。
 */
#define Q4_CAPTURE_SPEED               135
#define Q4_CAPTURE_KP                  7
#define Q4_CAPTURE_KD                  1
#define Q4_CAPTURE_MAX_CORRECTION      85
#define Q4_CAPTURE_CENTER_CONFIRM      2U
#define Q4_CAPTURE_MAX_TIME_MS         650U

#define Q4_ALIGN_FORWARD_SPEED         130
#define Q4_ALIGN_KP                    4
#define Q4_ALIGN_OK_DEG                5.0f
#define Q4_ALIGN_REVERSE               1
#define Q4_ALIGN_MAX_TIME_MS           1200
#define Q4_ALIGN_MAX_CORRECTION        90

typedef enum
{
    Q4_STATE_A_ALIGN_FIRST = 0,
    Q4_STATE_A_TO_C_FIRST,
    Q4_STATE_A_ALIGN_SCAN,
    Q4_STATE_A_TO_C_SCAN,
    Q4_STATE_C_CAPTURE_LINE,
    Q4_STATE_C_TO_B,
    Q4_STATE_B_ALIGN_FIRST,
    Q4_STATE_B_TO_D_FIRST,
    Q4_STATE_B_ALIGN_SCAN,
    Q4_STATE_B_TO_D_SCAN,
    Q4_STATE_D_CAPTURE_LINE,
    Q4_STATE_D_TO_A,
    Q4_STATE_STOP
} Q4_State_t;

static Q4_State_t q4_state = Q4_STATE_A_ALIGN_FIRST;

static uint32_t q4_start_time = 0;
static uint32_t q4_state_time = 0;

static float q4_yaw_start = 0.0f;
static float q4_fixed_start_yaw = 0.0f;
static float q4_target_abs = 0.0f;

static uint8_t q4_line_on_count = 0;
static uint8_t q4_line_lost_count = 0;
static uint8_t q4_capture_center_count = 0;
static int8_t q4_last_line_error = 0;
static int32_t q4_arc_correction_last = 0;
static uint8_t q4_lap_count = 0;

static uint8_t Q4_Distance_OK(uint16_t cm)
{
    int32_t target_count;

    target_count = (int32_t)cm * Q4_COUNTS_PER_CM;

    return (Get_Avg_Encoder() >= target_count);
}

static float Q4_Get_Relative_Yaw(void)
{
    JY901S_Update();

    if (!JY901S_HasYaw())
    {
        return 0.0f;
    }

    return Normalize_Angle(JY901S_GetYaw() - q4_yaw_start);
}

static float Q4_Make_Fixed_Target(float offset_deg)
{
    return Normalize_Angle(q4_fixed_start_yaw + offset_deg);
}

static void Q4_Begin_State(Q4_State_t next)
{
    q4_state = next;
    q4_state_time = g_ms;

    Encoder_Clear();

    q4_line_on_count = 0;
    q4_line_lost_count = 0;
    q4_capture_center_count = 0;
    q4_last_line_error = 0;
    q4_arc_correction_last = 0;

    JY901S_Update();

    if (JY901S_HasYaw())
    {
        q4_yaw_start = JY901S_GetYaw();
    }

    debug_state = (uint8_t)q4_state;
}

static uint8_t Q4_Is_Line_Detected(void)
{
    uint8_t gray;

    gray = Gray_Read_All();

    debug_gray = gray;
    debug_line_hit_count = Count_Gray_Hit(gray);

    return (debug_line_hit_count >= 1);
}

static uint8_t Q4_Line_On_Stable(void)
{
    if (Q4_Is_Line_Detected())
    {
        if (q4_line_on_count < Q4_LINE_ON_CONFIRM)
        {
            q4_line_on_count++;
        }
    }
    else
    {
        q4_line_on_count = 0;
    }

    return (q4_line_on_count >= Q4_LINE_ON_CONFIRM);
}

static uint8_t Q4_Line_Lost_Stable(void)
{
    if (!Q4_Is_Line_Detected())
    {
        if (q4_line_lost_count < Q4_LINE_LOST_CONFIRM)
        {
            q4_line_lost_count++;
        }
    }
    else
    {
        q4_line_lost_count = 0;
    }

    return (q4_line_lost_count >= Q4_LINE_LOST_CONFIRM);
}

static uint8_t Q4_Arc_End_By_LineLost_OK(uint16_t min_cm)
{
    if (Q4_Distance_OK(min_cm) && Q4_Line_Lost_Stable())
    {
        return 1;
    }

    return 0;
}

static uint8_t Q4_Line_Is_Centered(uint8_t gray)
{
    /*
     * bit4 = L1
     * bit3 = M
     * bit2 = R1
     *
     * 只要中间三个传感器之一压到线，
     * 就认为已经从“外侧扫到线”拉回到可正常循迹的位置。
     */
    if (gray & ((1 << 4) | (1 << 3) | (1 << 2)))
    {
        return 1;
    }

    return 0;
}

static uint8_t Q4_Align_To_Absolute_Target(float target_abs)
{
    float yaw;
    float error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    if (JY901S_HasYaw())
    {
        yaw = Normalize_Angle(JY901S_GetYaw());
    }
    else
    {
        yaw = 0.0f;
    }

    error = Normalize_Angle(target_abs - yaw);
    debug_align_error = error;
    q4_target_abs = target_abs;

    if (AbsFloat(error) <= Q4_ALIGN_OK_DEG)
    {
        Motor_Stop();
        return 1;
    }

    if ((g_ms - q4_state_time) >= Q4_ALIGN_MAX_TIME_MS)
    {
        Motor_Stop();
        return 1;
    }

    correction = (int32_t)(error * Q4_ALIGN_KP);

    correction = Limit_Int16(
        correction,
        -Q4_ALIGN_MAX_CORRECTION,
        Q4_ALIGN_MAX_CORRECTION
    );

#if Q4_ALIGN_REVERSE
    left_speed  = Q4_ALIGN_FORWARD_SPEED - correction;
    right_speed = Q4_ALIGN_FORWARD_SPEED + correction;
#else
    left_speed  = Q4_ALIGN_FORWARD_SPEED + correction;
    right_speed = Q4_ALIGN_FORWARD_SPEED - correction;
#endif

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    return 0;
}

static void Q4_Straight_Run_To_Absolute_Target(uint16_t base_speed, float target_abs)
{
    int32_t left_count;
    int32_t right_count;
    int32_t encoder_error;
    int32_t encoder_correction;
    int32_t yaw_correction;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;
    float yaw;
    float yaw_error;

    JY901S_Update();

    left_count = Abs32(Encoder_GetLeft());
    right_count = Abs32(Encoder_GetRight());

    encoder_error = left_count - right_count;
    encoder_correction = encoder_error * Q4_ENCODER_KP;

    encoder_correction = Limit_Int16(
        encoder_correction,
        -Q4_MAX_ENCODER_CORRECTION,
        Q4_MAX_ENCODER_CORRECTION
    );

    yaw_correction = 0;

    if (JY901S_HasYaw())
    {
        yaw = Normalize_Angle(JY901S_GetYaw());

        yaw_error = Normalize_Angle(yaw - target_abs);
        yaw_correction = (int32_t)(yaw_error * Q4_YAW_KP);

        yaw_correction = Limit_Int16(
            yaw_correction,
            -Q4_MAX_YAW_CORRECTION,
            Q4_MAX_YAW_CORRECTION
        );

        debug_align_error = Normalize_Angle(target_abs - yaw);
    }

    correction = encoder_correction + yaw_correction;

    correction = Limit_Int16(
        correction,
        -Q4_MAX_STRAIGHT_CORRECTION,
        Q4_MAX_STRAIGHT_CORRECTION
    );

    left_speed = base_speed - correction;
    right_speed = base_speed + correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
    q4_target_abs = target_abs;
}


static void Q4_Straight_Run_Scan_Stable(uint16_t base_speed)
{
    int32_t left_count;
    int32_t right_count;
    int32_t encoder_error;
    int32_t encoder_correction;
    int32_t yaw_correction;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;
    float yaw_error;

    /*
     * 第四问扫线直线段专用：
     *
     * 用在：
     * A->C 第二段扫 C 点黑线
     * B->D 第二段扫 D 点黑线
     *
     * 逻辑：
     * 1. Q4_Begin_State() 进入扫线状态时，会把 q4_yaw_start 记录为当前 yaw；
     * 2. 这段直走时，不追固定绝对角度；
     * 3. 只保持“进入扫线状态那一刻”的车头方向；
     * 4. 编码器负责左右轮走直；
     * 5. 陀螺仪只做轻微保角，防止慢慢偏出去。
     */
    JY901S_Update();

    left_count = Abs32(Encoder_GetLeft());
    right_count = Abs32(Encoder_GetRight());

    encoder_error = left_count - right_count;
    encoder_correction = encoder_error * Q4_ENCODER_KP;

    encoder_correction = Limit_Int16(
        encoder_correction,
        -Q4_MAX_ENCODER_CORRECTION,
        Q4_MAX_ENCODER_CORRECTION
    );

    yaw_correction = 0;

    if (JY901S_HasYaw())
    {
        /*
         * 这里的相对 yaw 是：
         * 当前 yaw - 进入扫线状态那一刻的 yaw。
         *
         * 所以它不是强行追全局 0° / -180°，
         * 而是让车保持刚摆正后的方向继续直走。
         */
        yaw_error = Q4_Get_Relative_Yaw();

        yaw_correction = (int32_t)(yaw_error * Q4_SCAN_YAW_KP);

        yaw_correction = Limit_Int16(
            yaw_correction,
            -Q4_SCAN_MAX_YAW_CORRECTION,
            Q4_SCAN_MAX_YAW_CORRECTION
        );

        debug_align_error = -yaw_error;
    }

    correction = encoder_correction + yaw_correction;

    correction = Limit_Int16(
        correction,
        -Q4_SCAN_MAX_CORRECTION,
        Q4_SCAN_MAX_CORRECTION
    );

    left_speed = base_speed - correction;
    right_speed = base_speed + correction;

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static int8_t Q4_Get_Weighted_Line_Error(uint8_t gray)
{
    int16_t error;

    /*
     * 7 路灰度加权误差：
     *
     * bit6 bit5 bit4 bit3 bit2 bit1 bit0
     * L3   L2   L1   M    R1   R2   R3
     *
     * 黑线 = 1
     */
    error = 0;

    if (gray & (1 << 6)) { error += -10; }  /* L3 */
    if (gray & (1 << 5)) { error += -6;  }  /* L2 */
    if (gray & (1 << 4)) { error += -3;  }  /* L1 */
    if (gray & (1 << 3)) { error +=  0;  }  /* M  */
    if (gray & (1 << 2)) { error +=  3;  }  /* R1 */
    if (gray & (1 << 1)) { error +=  6;  }  /* R2 */
    if (gray & (1 << 0)) { error +=  10; }  /* R3 */

    return (int8_t)error;
}

static uint8_t Q4_Capture_Line_To_Center(uint8_t reverse)
{
    uint8_t gray;
    uint8_t hit_count;
    int8_t line_error;
    int8_t d_error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    gray = Gray_Read_All();
    hit_count = Count_Gray_Hit(gray);

    debug_gray = gray;
    debug_line_hit_count = hit_count;

    /*
     * 捕线阶段：
     * 只要外侧传感器碰到线，就立刻按灰度误差修正，
     * 不再继续按固定角度往前冲。
     */
    if (hit_count == 0)
    {
        line_error = q4_last_line_error;
    }
    else
    {
        line_error = Q4_Get_Weighted_Line_Error(gray);
    }

    d_error = line_error - q4_last_line_error;

    correction = (int32_t)line_error * Q4_CAPTURE_KP +
                 (int32_t)d_error * Q4_CAPTURE_KD;

    q4_last_line_error = line_error;

    correction = Limit_Int16(
        correction,
        -Q4_CAPTURE_MAX_CORRECTION,
        Q4_CAPTURE_MAX_CORRECTION
    );

    correction = (q4_arc_correction_last * 2 + correction) / 3;
    q4_arc_correction_last = correction;

    if (reverse)
    {
        left_speed = Q4_CAPTURE_SPEED - correction;
        right_speed = Q4_CAPTURE_SPEED + correction;
    }
    else
    {
        left_speed = Q4_CAPTURE_SPEED + correction;
        right_speed = Q4_CAPTURE_SPEED - correction;
    }

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_line_error = line_error;
    debug_line_d_error = d_error;
    debug_left_speed = left_speed;
    debug_right_speed = right_speed;

    if (Q4_Line_Is_Centered(gray))
    {
        if (q4_capture_center_count < Q4_CAPTURE_CENTER_CONFIRM)
        {
            q4_capture_center_count++;
        }
    }
    else
    {
        q4_capture_center_count = 0;
    }

    if (q4_capture_center_count >= Q4_CAPTURE_CENTER_CONFIRM)
    {
        return 1;
    }

    /*
     * 兜底：如果已经捕线一段时间还没完全居中，
     * 也进入正常循迹，避免卡死。
     */
    if ((g_ms - q4_state_time) >= Q4_CAPTURE_MAX_TIME_MS)
    {
        return 1;
    }

    return 0;
}

static void Q4_Line_Follow_Arc(uint16_t base_speed, uint8_t reverse)
{
    uint8_t gray;
    uint8_t hit_count;
    int8_t line_error;
    int8_t d_error;
    int32_t correction;
    int16_t left_speed;
    int16_t right_speed;

    JY901S_Update();

    gray = Gray_Read_All();
    hit_count = Count_Gray_Hit(gray);

    debug_gray = gray;
    debug_line_hit_count = hit_count;

    if (hit_count == 0)
    {
        line_error = q4_last_line_error;
    }
    else
    {
        line_error = Q4_Get_Weighted_Line_Error(gray);
    }

    d_error = line_error - q4_last_line_error;

    correction = (int32_t)line_error * Q4_LINE_KP_ARC +
                 (int32_t)d_error * Q4_LINE_KD_ARC;

    q4_last_line_error = line_error;

    correction = Limit_Int16(
        correction,
        -Q4_MAX_LINE_CORRECTION_ARC,
        Q4_MAX_LINE_CORRECTION_ARC
    );

    correction = (q4_arc_correction_last * 2 + correction) / 3;
    q4_arc_correction_last = correction;

    if (reverse)
    {
        left_speed = base_speed - correction;
        right_speed = base_speed + correction;
    }
    else
    {
        left_speed = base_speed + correction;
        right_speed = base_speed - correction;
    }

    left_speed = Limit_Int16(left_speed, 0, 1000);
    right_speed = Limit_Int16(right_speed, 0, 1000);

    Motor_SetSpeed(left_speed, right_speed);

    debug_line_error = line_error;
    debug_line_d_error = d_error;
    debug_left_speed = left_speed;
    debug_right_speed = right_speed;
}

static void Q4_Debug_Update(void)
{
    JY901S_Update();

    debug_state = (uint8_t)q4_state;
    debug_lap_count = q4_lap_count;

    debug_left_encoder = Encoder_GetLeft();
    debug_right_encoder = Encoder_GetRight();
    debug_avg_encoder = Get_Avg_Encoder();

    if (JY901S_HasYaw())
    {
        debug_yaw = JY901S_GetYaw();
        debug_relative_yaw = Q4_Get_Relative_Yaw();
    }
}

static void Run_Question4(void)
{
    uint16_t i;

    Encoder_Clear();

    for (i = 0; i < 100; i++)
    {
        JY901S_Update();
        Delay_ms(5);
    }

    if (JY901S_HasYaw())
    {
        q4_yaw_start = JY901S_GetYaw();
        q4_fixed_start_yaw = Normalize_Angle(q4_yaw_start);
    }
    else
    {
        q4_yaw_start = 0.0f;
        q4_fixed_start_yaw = 0.0f;
    }

    q4_start_time = g_ms;
    q4_state_time = g_ms;

    q4_lap_count = 0;
    debug_lap_count = 0;

    q4_state = Q4_STATE_A_ALIGN_FIRST;
    debug_state = (uint8_t)q4_state;

    q4_line_on_count = 0;
    q4_line_lost_count = 0;
    q4_capture_center_count = 0;
    q4_last_line_error = 0;
    q4_arc_correction_last = 0;
    q4_target_abs = Q4_Make_Fixed_Target(Q4_A_TO_C_FIRST_DEG);

    while (1)
    {
        JY901S_Update();
        Q4_Debug_Update();

        if ((q4_state != Q4_STATE_STOP) &&
            ((g_ms - q4_start_time) >= Q4_MAX_TIME_MS))
        {
            Motor_Stop();
            Alert_Once();
            q4_state = Q4_STATE_STOP;
            debug_state = (uint8_t)q4_state;
        }

        switch (q4_state)
        {
            case Q4_STATE_A_ALIGN_FIRST:
                if (Q4_Align_To_Absolute_Target(Q4_Make_Fixed_Target(Q4_A_TO_C_FIRST_DEG)))
                {
                    Q4_Begin_State(Q4_STATE_A_TO_C_FIRST);
                }
                break;

            case Q4_STATE_A_TO_C_FIRST:
                Q4_Straight_Run_To_Absolute_Target(
                    Q4_BASE_SPEED,
                    Q4_Make_Fixed_Target(Q4_A_TO_C_FIRST_DEG)
                );

                if (Q4_Distance_OK(Q4_AC_FIRST_DISTANCE_CM))
                {
                    Q4_Begin_State(Q4_STATE_A_ALIGN_SCAN);
                }
                break;

            case Q4_STATE_A_ALIGN_SCAN:
                if (Q4_Align_To_Absolute_Target(Q4_Make_Fixed_Target(Q4_A_TO_C_SCAN_DEG)))
                {
                    Q4_Begin_State(Q4_STATE_A_TO_C_SCAN);
                }
                break;

            case Q4_STATE_A_TO_C_SCAN:
                /*
                 * 扫 C 点黑线这段改成更稳的直走：
                 * 编码器走直 + 轻微 yaw 保角。
                 */
                Q4_Straight_Run_Scan_Stable(Q4_SCAN_SPEED);

                /*
                 * 任意灰度扫到 C 点线后，立刻进入捕线纠偏。
                 * 不再继续按 0° 往前直冲。
                 */
                if (Q4_Line_On_Stable())
                {
                    Q4_Begin_State(Q4_STATE_C_CAPTURE_LINE);
                }
                break;

            case Q4_STATE_C_CAPTURE_LINE:
                if (Q4_Capture_Line_To_Center(Q4_CB_LINE_REVERSE))
                {
                    Alert_Once();
                    Q4_Begin_State(Q4_STATE_C_TO_B);
                }
                break;

            case Q4_STATE_C_TO_B:
                Q4_Line_Follow_Arc(Q4_ARC_SPEED, Q4_CB_LINE_REVERSE);

                if (Q4_Arc_End_By_LineLost_OK(Q4_CB_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();
                    Q4_Begin_State(Q4_STATE_B_ALIGN_FIRST);
                }
                break;

            case Q4_STATE_B_ALIGN_FIRST:
                if (Q4_Align_To_Absolute_Target(Q4_Make_Fixed_Target(Q4_B_TO_D_FIRST_DEG)))
                {
                    Q4_Begin_State(Q4_STATE_B_TO_D_FIRST);
                }
                break;

            case Q4_STATE_B_TO_D_FIRST:
                Q4_Straight_Run_To_Absolute_Target(
                    Q4_BASE_SPEED,
                    Q4_Make_Fixed_Target(Q4_B_TO_D_FIRST_DEG)
                );

                if (Q4_Distance_OK(Q4_BD_FIRST_DISTANCE_CM))
                {
                    Q4_Begin_State(Q4_STATE_B_ALIGN_SCAN);
                }
                break;

            case Q4_STATE_B_ALIGN_SCAN:
                if (Q4_Align_To_Absolute_Target(Q4_Make_Fixed_Target(Q4_B_TO_D_SCAN_DEG)))
                {
                    Q4_Begin_State(Q4_STATE_B_TO_D_SCAN);
                }
                break;

            case Q4_STATE_B_TO_D_SCAN:
                /*
                 * 扫 D 点黑线这段改成更稳的直走：
                 * 编码器走直 + 轻微 yaw 保角。
                 */
                Q4_Straight_Run_Scan_Stable(Q4_SCAN_SPEED);

                /*
                 * 任意灰度扫到 D 点线后，立刻进入捕线纠偏。
                 */
                if (Q4_Line_On_Stable())
                {
                    Q4_Begin_State(Q4_STATE_D_CAPTURE_LINE);
                }
                break;

            case Q4_STATE_D_CAPTURE_LINE:
                if (Q4_Capture_Line_To_Center(Q4_DA_LINE_REVERSE))
                {
                    Alert_Once();
                    Q4_Begin_State(Q4_STATE_D_TO_A);
                }
                break;

            case Q4_STATE_D_TO_A:
                Q4_Line_Follow_Arc(Q4_ARC_SPEED, Q4_DA_LINE_REVERSE);

                if (Q4_Arc_End_By_LineLost_OK(Q4_DA_ARC_MIN_DISTANCE_CM))
                {
                    Alert_Once();

                    q4_lap_count++;
                    debug_lap_count = q4_lap_count;

                    if (q4_lap_count >= Q4_TOTAL_LAPS)
                    {
                        q4_state = Q4_STATE_STOP;
                        debug_state = (uint8_t)q4_state;
                    }
                    else
                    {
                        Q4_Begin_State(Q4_STATE_A_ALIGN_FIRST);
                    }
                }
                break;

            case Q4_STATE_STOP:
            default:
                Motor_Stop();
                break;
        }

        Delay_ms(10);
    }
}




/* ===================== 第三问 PA24 按键 ===================== */

/*
 * 第三问选择按键改到 PA24。
 *
 * 默认按键接法按 PA21/PA25 一样处理：
 * PA24 配置为上拉输入，按下接 GND，低电平为按下。
 */
#define Q3_SELECT_KEY_PIN              DL_GPIO_PIN_24

static void Q3_Select_Key_PA24_Init(void)
{
    /*
     * PA24 请在 SysConfig 里配置为 GPIO Input Pull-Up。
     * 这里不手动调用 DL_GPIO_initDigitalInputFeatures，
     * 避免不同 SDK 版本 PinCM 参数不一致导致编译报错。
     */
}

static uint8_t Q3_Select_Key_PA24_Read(void)
{
    if ((DL_GPIO_readPins(GPIOA, Q3_SELECT_KEY_PIN) & Q3_SELECT_KEY_PIN) == 0)
    {
        return KEY_PRESSED;
    }

    return KEY_RELEASED;
}


/* ===================== 上电选择模式 ===================== */

static uint8_t Select_Mode_On_Power(void)
{
    uint32_t start_time;

    /*
     * 默认第一问。
     *
     * 上电后给 3 秒选择时间：
     * PA21 -> 第二问
     * PA24 -> 第三问
     * PA2  -> 第四问
     * PA28 不使用
     *
     * 如果 3 秒内没有按键，就自动进入第一问。
     */

    start_time = g_ms;

    while ((g_ms - start_time) < 3000U)
    {
        Motor_Stop();

        if (Key1_Read() == KEY_PRESSED)
        {
            Delay_ms(30);

            if (Key1_Read() == KEY_PRESSED)
            {
                return KEY_MODE_2;
            }
        }

        if (Q3_Select_Key_PA24_Read() == KEY_PRESSED)
        {
            Delay_ms(30);

            if (Q3_Select_Key_PA24_Read() == KEY_PRESSED)
            {
                return KEY_MODE_3;
            }
        }

        if (Key3_Read() == KEY_PRESSED)
        {
            Delay_ms(30);

            if (Key3_Read() == KEY_PRESSED)
            {
                return KEY_MODE_4;
            }
        }

        Delay_ms(10);
    }

    return KEY_MODE_1;
}

/* ===================== 总 main ===================== */

int main(void)
{
    uint8_t mode;

    SYSCFG_DL_init();

    SysTick_Config(CPUCLK_FREQ / 1000);

    Gray_Init();
    Motor_Init();
    LED_Init();
    Buzzer_Init();
    Encoder_Init();
    JY901S_Init();
    Key_Init();
    Q3_Select_Key_PA24_Init();

    Encoder_Clear();
    Motor_Stop();
    /*
     * 上电后 3 秒内选择：
     * PA21 -> 第二问
     * PA24 -> 第三问
     * PA2  -> 第四问
     *
     * 3 秒内不按任何按键，自动进入第一问。
     */
    mode = Select_Mode_On_Power();

    if (mode == KEY_MODE_1)
    {
        Run_Question1();
    }
    else if (mode == KEY_MODE_2)
    {
        Run_Question2();
    }
    else if (mode == KEY_MODE_3)
    {
        Run_Question3();
    }
    else
    {
        Run_Question4();
    }

    while (1)
    {
        Motor_Stop();
    }
}
