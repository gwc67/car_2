#include "led_base.h"
#include "led_gpio.h"
#include "main.h"
#include "AutoInit\driver_registry.h"
LedBase *g_left_dir_gpio_pst;
LedBase *g_right_dir_gpio_pst;

DelayBase *pst_os_delay;  //句柄delay放后面，函数delay放前面
DelayBase *pst_hal_delay;  //句柄delay放后面，函数delay放前面
static LedGpio s_gpio_left_dir_st;
static LedGpio s_gpio_right_dir_st;


static stDelay st_os_delay;
static stDelay st_hal_delay;
void board_init(void)
{
    led_gpio_init(&s_gpio_left_dir_st,"gpio_left",LEFT_DIR_GPIO_Port,LEFT_DIR_Pin,1);
    led_gpio_init(&s_gpio_right_dir_st,"gpio_right",RIGHT_DIR_GPIO_Port,RIGHT_DIR_Pin,1);

    delay_os_init(&st_os_delay);
    delay_hal_init(&st_hal_delay);

    g_left_dir_gpio_pst = &s_gpio_left_dir_st.base;
    g_right_dir_gpio_pst = &s_gpio_right_dir_st.base;

    pst_os_delay = &st_os_delay.base;
    pst_hal_delay = &st_hal_delay.base;
}
DRIVER_INIT_1(board_init);
