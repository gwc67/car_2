//第三级 板级 与 board_init.c 绑定硬件，唯一认识硬件 // led_base *g_led_error
#ifndef __LEDS_H
#define __LEDS_H

#include "led_base.h"
extern LedBase *g_left_dir_gpio_pst;
extern LedBase *g_right_dir_gpio_pst;
extern LedBase *g_led_blue;
extern LedBase *g_led_ano;
extern DelayBase *pst_os_delay;  
extern DelayBase *pst_hal_delay; 
extern LedBase *g_buzzer_pst;
extern LedBase *g_lazzer_pst;

void board_init(void);

#endif

