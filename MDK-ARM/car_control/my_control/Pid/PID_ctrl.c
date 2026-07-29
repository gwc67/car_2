#include "PID_ctrl.h"
#include "driver_registry.h"

#include "FreeRTOS.h"
#include "task.h"

#define ABS(x)  ((x) > 0 ? (x) : -(x))
#define _MIN(a, b) ((a) < (b) ? (a) : (b))
#define _MAX(a, b) ((a) > (b) ? (a) : (b))

/* ================================================================
 * 全局 PID 实例
 * ================================================================ */

/* 速度环 — 用编码器反馈保持恒定车速 */
struct AdaptivePID_t g_speed_pid = {
    .Kp_f            = 0.5f,
    .Ki_f            = 0.1f,
    .Kd_f            = 0.0f,
    .output_max_l    = 4000,
    .output_min_l    = -4000,
    .integral_max_l  = 500,
    .I_Band_l        = 100,
    .d_filter_alpha_f = 0.8f,
};

/* 转向环 — 预留，后续可用编码器差值或陀螺仪做角度闭环 */
struct AdaptivePID_t g_turn_pid = {
    .Kp_f            = 1.0f,
    .Ki_f            = 0.0f,
    .Kd_f            = 0.0f,
    .output_max_l    = 1500,
    .output_min_l    = -1500,
    .integral_max_l  = 300,
    .I_Band_l        = 50,
    .d_filter_alpha_f = 0.8f,
};

/* ================================================================
 * 初始化 (清零运行时状态)
 * ================================================================ */
void PID_Init(void)
{
    pid_reset_v(&g_speed_pid);
    pid_reset_v(&g_turn_pid);
}
DRIVER_INIT(PID_Init);

/* ================================================================
 * PID 更新
 * ================================================================ */
int32_t PID_Update_l(struct AdaptivePID_t *pid_pst, float setpoint_f, float measurement_f)
{
    float error_f = setpoint_f - measurement_f;

    pid_pst->error_f = error_f;
    uint32_t now_time_ul = xTaskGetTickCount();

    pid_pst->delat_time_ul = now_time_ul - pid_pst->last_time_ul;
    pid_pst->last_time_ul  = now_time_ul;

    if (pid_pst->delat_time_ul == 0) {
        pid_pst->delat_time_ul = 1;
    }

    /* 积分分离 */
    if (ABS(error_f) <= pid_pst->I_Band_l) {
        pid_pst->integral_f += error_f * pid_pst->delat_time_ul;
        pid_pst->integral_f = _MIN(_MAX(pid_pst->integral_f,
                                        -(float)pid_pst->integral_max_l),
                                   (float)pid_pst->integral_max_l);
    } else {
        pid_pst->integral_f = 0;
    }

    /* 微分先行 + 滤波 */
    float d_raw_f = (measurement_f - pid_pst->prev_measurement_f) / pid_pst->delat_time_ul;
    float d_filtered_f = pid_pst->d_filter_alpha_f * d_raw_f
                       + (1.0f - pid_pst->d_filter_alpha_f) * pid_pst->prev_d_filtered_f;

    /* PID 输出 */
    float out_put_f = pid_pst->Kp_f * error_f
                    + pid_pst->Ki_f * pid_pst->integral_f
                    - pid_pst->Kd_f * d_filtered_f;

    /* 输出限幅 + 抗积分饱和 */
    if (out_put_f > pid_pst->output_max_l) {
        out_put_f = pid_pst->output_max_l;
        pid_pst->integral_f = (out_put_f - pid_pst->Kp_f * error_f
                               + pid_pst->Kd_f * d_filtered_f) / pid_pst->Ki_f;
    } else if (out_put_f < pid_pst->output_min_l) {
        out_put_f = pid_pst->output_min_l;
        pid_pst->integral_f = (out_put_f - pid_pst->Kp_f * error_f
                               + pid_pst->Kd_f * d_filtered_f) / pid_pst->Ki_f;
    }

    pid_pst->prev_measurement_f = measurement_f;
    pid_pst->prev_d_filtered_f  = d_filtered_f;

    return (int32_t)out_put_f;
}

/* ================================================================
 * 辅助函数
 * ================================================================ */

void pid_reset_v(struct AdaptivePID_t *pid_pst)
{
    pid_pst->integral_f         = 0;
    pid_pst->prev_measurement_f = 0;
    pid_pst->prev_d_filtered_f  = 0;
    pid_pst->last_time_ul       = 0;
    pid_pst->error_f            = 0;
}

float Get_PID_Error_f(struct AdaptivePID_t *pid_pst)
{
    return pid_pst->error_f;
}

/* ---- 热调参 (OLED 菜单绑定) ---- */
void pid_speed_set_kp(float val) { g_speed_pid.Kp_f = val; }
void pid_speed_set_ki(float val) { g_speed_pid.Ki_f = val; }
void pid_speed_set_kd(float val) { g_speed_pid.Kd_f = val; }
