#ifndef __LED_GPIO_H
#define __LED_GPIO_H

#include "led_base.h"

typedef struct 
{
    LedBase base;
    uint16_t pin;
    GPIO_TypeDef* gpio;
    uint8_t on_level;
    uint8_t brightness;

    // int brightness; 
}LedGpio;

typedef struct 
{
    DelayBase base;
}stDelay;

void led_gpio_init(LedGpio *me,const char *name,GPIO_TypeDef* gpio,uint16_t pin,uint8_t on_level);
void delay_os_init(stDelay* me);
void delay_hal_init(stDelay* me);

#endif
