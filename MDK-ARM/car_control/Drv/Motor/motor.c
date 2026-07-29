#include "motor.h"
#include "driver_registry.h"

void motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

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
