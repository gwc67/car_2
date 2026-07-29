#include "motor.h"
#include "driver_registry.h"
#include "../xvji/xvji.h"
void motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}


// PWM 范围: -5000 ~ 5000
/* 循迹速度参数 (可调) */
#define BASE_SPEED       2000    // 直行基础速度
#define TURN_SPEED_LOW   1200    // 微调弯道较慢侧
#define TURN_SPEED_HIGH  2800    // 微调弯道较快侧
#define SHARP_FWD        2500    // 大弯前进侧
#define SHARP_REV       -1000    // 大弯后退侧

DRIVER_INIT(motor_Init);

void motor_setspeed_right(int32_t PWM)
{
    if (PWM >= 0)
    {
        RIGHT_DIR(1);
        TIM_SetCompare_RIGHT(PWM);
    }
    else
    {
        RIGHT_DIR(0);
        TIM_SetCompare_RIGHT(-PWM);
    }
}

void motor_setspeed_left(int32_t PWM)
{
    if (PWM >= 0)
    {
        LEFT_DIR(1);
        TIM_SetCompare_LEFT(PWM);
    }
    else
    {
        LEFT_DIR(0);
        TIM_SetCompare_LEFT(-PWM);
    }
}


/*
 * 循迹电机控制 — 在 10ms 调度任务中调用
 *
 * 策略: 差速转向
 *   - 线偏左 → 左轮减速 / 右轮加速 → 车左转找回线
 *   - 线偏右 → 右轮减速 / 左轮加速 → 车右转找回线
 */
void motor_task_v(void)
{
    enum xvji_state_e xvji_state = xvji_get_state();

    switch (xvji_state) {

    case STRAIGHT_em:
        motor_setspeed_left(BASE_SPEED);
        motor_setspeed_right(BASE_SPEED);
        break;

    case LEFT_em:
        /* 线偏左 → 右轮快、左轮慢 → 车左转 */
        motor_setspeed_left(TURN_SPEED_LOW);
        motor_setspeed_right(TURN_SPEED_HIGH);
        break;

    case RIGHT_em:
        /* 线偏右 → 左轮快、右轮慢 → 车右转 */
        motor_setspeed_left(TURN_SPEED_HIGH);
        motor_setspeed_right(TURN_SPEED_LOW);
        break;

    case LEFT_HIGH_em:
        /* 急左转 → 左轮倒转、右轮正转 → 原地左旋 */
        motor_setspeed_left(SHARP_REV);
        motor_setspeed_right(SHARP_FWD);
        break;

    case RIGHT_HIGH_em:
        /* 急右转 → 右轮倒转、左轮正转 → 原地右旋 */
        motor_setspeed_left(SHARP_FWD);
        motor_setspeed_right(SHARP_REV);
        break;

    default:
        motor_setspeed_left(BASE_SPEED);
        motor_setspeed_right(BASE_SPEED);
        break;
    }
}



//  void motor_task_v(void)
//   {
//       // 1. 从编码器获取当前速度
//       EncoderState_t left, right;
//       encoder_get_state(ENCODER_LEFT,  &left);
//       encoder_get_state(ENCODER_RIGHT, &right);
//       float cur_speed = (left.speed_pps + right.speed_pps) / 2.0f;

//       // 2. 速度 PID: 目标速度 → PWM 基础值
//       float target = 500.0f;  // 目标速度 pps (可绑到菜单)
//       int32_t base_pwm = PID_Update_l(&g_speed_pid, target, cur_speed);

//       // 3. 叠加差速转向 (根据循迹状态)
//       enum xvji_state_e state = xvji_get_state();
//       int32_t left_pwm  = base_pwm;
//       int32_t right_pwm = base_pwm;

//       switch (state) {
//       case STRAIGHT_em:    /* 不调整 */                              break;
//       case LEFT_em:        left_pwm -= 500;  right_pwm += 500;       break;
//       case RIGHT_em:       left_pwm += 500;  right_pwm -= 500;       break;
//       case LEFT_HIGH_em:   left_pwm  = -1000; right_pwm = 2500;      break;
//       case RIGHT_HIGH_em:  left_pwm  = 2500;  right_pwm = -1000;     break;
//       }

//       motor_setspeed_left(left_pwm);
//       motor_setspeed_right(right_pwm);
//   }


