#ifndef __PID_CTRL_H
#define __PID_CTRL_H

#include "main.h"

typedef struct AdaptivePID_t AdaptivePID_t;

struct AdaptivePID_t {
    float Kp_f, Ki_f, Kd_f;

    float integral_f;
    float prev_measurement_f;
    float prev_d_filtered_f;
    float error_f;

    int32_t output_max_l;
    int32_t output_min_l;
    int32_t integral_max_l;
    int32_t I_Band_l;

    float d_filter_alpha_f;

    uint32_t delat_time_ul;
    uint32_t last_time_ul;
};

/* ---- 全局 PID 实例 ---- */
extern struct AdaptivePID_t g_speed_pid;   // 速度环
extern struct AdaptivePID_t g_turn_pid;    // 转向环

/* ---- 初始化 ---- */
void PID_Init(void);

/* ---- PID 更新 ---- */
int32_t PID_Update_l(struct AdaptivePID_t *pid_pst, float setpoint_f, float measurement_f);

/* ---- 辅助 ---- */
void pid_reset_v(struct AdaptivePID_t *pid_pst);
float Get_PID_Error_f(struct AdaptivePID_t *pid_pst);

/* ---- 热调参 (OLED 菜单用) ---- */
void pid_speed_set_kp(float val);
void pid_speed_set_ki(float val);
void pid_speed_set_kd(float val);

#endif
