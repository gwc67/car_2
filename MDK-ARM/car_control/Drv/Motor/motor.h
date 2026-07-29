#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"
#include "tim.h"

/* TIM2 两路 PWM — CH1 控制右电机, CH2 控制左电机 */
#define PWM_TIME htim2

/* 每个通道独立设置比较值 */
#define TIM_SetCompare_RIGHT(Speed)  __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_2, (Speed))
#define TIM_SetCompare_LEFT(Speed)   __HAL_TIM_SetCompare(&htim2, TIM_CHANNEL_1, (Speed))

/* 方向引脚: HIGH=正转, LOW=反转 */
#define LEFT_DIR(x)   HAL_GPIO_WritePin(LEFT_DIR_GPIO_Port,  LEFT_DIR_Pin,  (GPIO_PinState)(x))
#define RIGHT_DIR(x)  HAL_GPIO_WritePin(RIGHT_DIR_GPIO_Port, RIGHT_DIR_Pin, (GPIO_PinState)(x))

void motor_setspeed_right(int32_t PWM);
void motor_setspeed_left(int32_t PWM);

#endif
