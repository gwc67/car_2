// #ifndef __PID_CTRL_H
// #define __PID_CTRL_H

// #include "main.h"

// typedef struct AdaptivePID_t AdaptivePID_t;

// // 初始化
// void PID_Init(void);

// // 访问器函数（返回单个轴的 PID 实例指针）
// AdaptivePID_t* pid_get_loc_xyz(uint8_t axis_uc);
// AdaptivePID_t* pid_get_cam_xy(uint8_t axis_uc);

// // PID 更新
// int32_t PID_Update_l(AdaptivePID_t* pid_pst, float setpoint_f, float measurement_f);
// int32_t Yaw_PID_Update(float setpoint_f, float measurement_f);

// // 输出限幅
// void PID_SetXY_ABS_OutputMax_v(AdaptivePID_t* pid_pst, int32_t ABS_OutputMax_l);

// // 获取 PID 状态
// float Get_PID_Error_f(AdaptivePID_t* pid_pst);

// void pid_reset_v(AdaptivePID_t* pid_pst);
// void pid_reset_prev_measurement(struct AdaptivePID_t* pid_pst);

// float angleDifference_f(float target, float current);

// void pid_set_xy_kp(float kp_f);
// void pid_set_xy_ki(float ki_f);
// void pid_set_xy_kd(float kd_f);

// #endif
