#include "PID_ctrl.h"
#include "driver_registry.h"

#include "Ano_Scheduler.h"
#include "FreeRTOS.h"
#include "task.h"

#define ABS(x) ((x) > 0 ? (x) : -(x))
#define _MIN(a, b) ((a) < (b) ? (a) : (b))
#define _MAX(a, b) ((a) > (b) ? (a) : (b))
struct AdaptivePID_t
{
    float Kp_f, Ki_f, Kd_f;

    float integral_f;               // 积分项
    float prev_measurement_f;       // 上一次测量值 (用于微分计算)
    float prev_d_filtered_f;        // 上一次微分滤波值
    float error_f;                  // 当前误差

    int32_t output_max_l;           // 输出上限
    int32_t output_min_l;           // 输出下限
    int32_t integral_max_l;         // 积分项最大值
    int32_t I_Band_l;               // 积分分离阈值

    float d_filter_alpha_f;         // 微分滤波系数 (0~1)

    uint32_t delat_time_ul;         // 距上次调用时间
    uint32_t last_time_ul;          // 上次调用 tick
};


void PID_Init(void)
{


    

    // // X 轴 PID
    // loc_xyz_pst[X_em].Kp_f = 0.18f;
    // loc_xyz_pst[X_em].Ki_f = 0.12f;
    // loc_xyz_pst[X_em].Kd_f = 0.7f;

    // loc_xyz_pst[X_em].output_max_l   = 23;
    // loc_xyz_pst[X_em].output_min_l   = -23;
    // loc_xyz_pst[X_em].integral_max_l = 5;
    // loc_xyz_pst[X_em].I_Band_l       = 35;

    // loc_xyz_pst[X_em].integral_f          = 0.0f;
    // loc_xyz_pst[X_em].prev_measurement_f  = 0.0f;
    // loc_xyz_pst[X_em].prev_d_filtered_f   = 0.0f;
    // loc_xyz_pst[X_em].d_filter_alpha_f    = 0.8f;

    // // Y 轴 = X 轴
    // loc_xyz_pst[Y_em] = loc_xyz_pst[X_em];

    // // Z 轴 PID
    // loc_xyz_pst[Z_em].Kp_f = 0.23f;
    // loc_xyz_pst[Z_em].Ki_f = 0.03f;
    // loc_xyz_pst[Z_em].Kd_f = 0.08f;

    // loc_xyz_pst[Z_em].output_max_l   = 20;
    // loc_xyz_pst[Z_em].output_min_l   = -20;
    // loc_xyz_pst[Z_em].integral_max_l = 5;
    // loc_xyz_pst[Z_em].I_Band_l       = 20;

    // loc_xyz_pst[Z_em].integral_f          = 0.0f;
    // loc_xyz_pst[Z_em].prev_measurement_f  = 0.0f;
    // loc_xyz_pst[Z_em].prev_d_filtered_f   = 0.0f;
    // loc_xyz_pst[Z_em].d_filter_alpha_f    = 0.8f;

    // // Yaw 轴 PID
    // loc_xyz_pst[Yaw_em].Kp_f = 0.35f;
    // loc_xyz_pst[Yaw_em].Ki_f = 0.1f;
    // loc_xyz_pst[Yaw_em].Kd_f = 0.15f;

    // loc_xyz_pst[Yaw_em].output_max_l   = 30;
    // loc_xyz_pst[Yaw_em].output_min_l   = -30;
    // loc_xyz_pst[Yaw_em].integral_max_l = 10;
    // loc_xyz_pst[Yaw_em].I_Band_l       = 20;

    // loc_xyz_pst[Yaw_em].integral_f          = 0.0f;
    // loc_xyz_pst[Yaw_em].prev_measurement_f  = 0.0f;
    // loc_xyz_pst[Yaw_em].prev_d_filtered_f   = 0.0f;
    // loc_xyz_pst[Yaw_em].d_filter_alpha_f    = 0.8f;

    // // 摄像头 X 轴 PID
    // cam_xy_pst[X_em].Kp_f = 0.40f;
    // cam_xy_pst[X_em].Ki_f = 0.16f;
    // cam_xy_pst[X_em].Kd_f = 0.33f;

    // cam_xy_pst[X_em].output_max_l   = 4;
    // cam_xy_pst[X_em].output_min_l   = -4;
    // cam_xy_pst[X_em].integral_max_l = 3;
    // cam_xy_pst[X_em].I_Band_l       = 18;

    // cam_xy_pst[X_em].integral_f          = 0.0f;
    // cam_xy_pst[X_em].prev_measurement_f  = 0.0f;
    // cam_xy_pst[X_em].prev_d_filtered_f   = 0.0f;
    // cam_xy_pst[X_em].d_filter_alpha_f    = 0.8f;

    // // 摄像头 Y 轴 = X 轴
    // cam_xy_pst[Y_em] = cam_xy_pst[X_em];
}

DRIVER_INIT(PID_Init);


// int32_t float_to_int32_l(float f)
// {
//     if (isnan(f))
//     {
//         return 0;
//     }

//     if (f >= (float)INT32_MAX + 0.5f)
//     {
//         return INT32_MAX;
//     }
//     else if (f <= (float)INT32_MIN - 0.5f)
//     {
//         return INT32_MIN;
//     }

//     return (f >= 0.0f ? (int32_t)(f + 0.5f) : (int32_t)(f - 0.5f));
// }


float angleDifference_f(float target, float current) {
    float error = target - current;
    while (error > 180.0f) error -= 360.0f;
    while (error < -180.0f) error += 360.0f;
    return error;
}


int32_t PID_Update_l(struct AdaptivePID_t* pid_pst, float setpoint_f, float measurement_f)
{
    float error_f = setpoint_f - measurement_f;

    pid_pst->error_f = error_f;
    uint32_t now_time_ul = xTaskGetTickCount();

    pid_pst->delat_time_ul = now_time_ul - pid_pst->last_time_ul;
    pid_pst->last_time_ul  = now_time_ul;

    if (pid_pst->delat_time_ul == 0)
    {
        pid_pst->delat_time_ul = 1;
    }

    // 积分分离: 只在误差较小时累积积分，避免大误差时积分饱和
    if (ABS(error_f) <= pid_pst->I_Band_l)
    {
        pid_pst->integral_f += error_f * pid_pst->delat_time_ul;
        pid_pst->integral_f = _MIN(_MAX(pid_pst->integral_f,
                                        -(float)pid_pst->integral_max_l),
                                   (float)pid_pst->integral_max_l);
    }
    else
    {
        pid_pst->integral_f = 0;
    }

    // 微分先行 + 不完全微分滤波
    float d_raw_f = (measurement_f - pid_pst->prev_measurement_f) / pid_pst->delat_time_ul;
    float d_filtered_f = pid_pst->d_filter_alpha_f * d_raw_f
                       + (1.0f - pid_pst->d_filter_alpha_f) * pid_pst->prev_d_filtered_f;

    // PID 输出: P * error + I * integral - D * d_filtered  (D 项阻尼)
    float out_put_f = pid_pst->Kp_f * error_f
                    + pid_pst->Ki_f * pid_pst->integral_f
                    - pid_pst->Kd_f * d_filtered_f;

    // 输出限幅 + 抗积分饱和
    if (out_put_f > pid_pst->output_max_l)
    {
        out_put_f = pid_pst->output_max_l;
        pid_pst->integral_f = (out_put_f - pid_pst->Kp_f * error_f
                               + pid_pst->Kd_f * d_filtered_f) / pid_pst->Ki_f;
    }
    else if (out_put_f < pid_pst->output_min_l)
    {
        out_put_f = pid_pst->output_min_l;
        pid_pst->integral_f = (out_put_f - pid_pst->Kp_f * error_f
                               + pid_pst->Kd_f * d_filtered_f) / pid_pst->Ki_f;
    }

    pid_pst->prev_measurement_f = measurement_f;
    pid_pst->prev_d_filtered_f  = d_filtered_f;

    return out_put_f;
}



float Get_PID_Error_f(struct AdaptivePID_t *pid_pst)
{
    return pid_pst->error_f;
}


void PID_SetXY_ABS_OutputMax_v(struct AdaptivePID_t *pid_pst, int32_t ABS_OutputMax_l)
{
    pid_pst->output_max_l = ABS_OutputMax_l;
    pid_pst->output_min_l = -ABS_OutputMax_l;
}




void pid_reset_v(struct AdaptivePID_t* pid_pst)
{
    pid_pst->integral_f         = 0;
    pid_pst->prev_measurement_f = 0;
    pid_pst->prev_d_filtered_f  = 0;
}


void pid_reset_prev_measurement(struct AdaptivePID_t* pid_pst)
{
    pid_pst->prev_measurement_f = 0;
}


// void pid_set_xy_kp(float kp_f)
// {
//     loc_xyz_pst[X_em].Kp_f = kp_f;
//     loc_xyz_pst[Y_em].Kp_f = kp_f;
// }


// void pid_set_xy_ki(float ki_f)
// {
//     loc_xyz_pst[X_em].Ki_f = ki_f;
//     loc_xyz_pst[Y_em].Ki_f = ki_f;
// }


// void pid_set_xy_kd(float kd_f)
// {
//     loc_xyz_pst[X_em].Kd_f = kd_f;
//     loc_xyz_pst[Y_em].Kd_f = kd_f;
// }
