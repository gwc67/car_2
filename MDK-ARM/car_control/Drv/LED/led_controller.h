#ifndef __LED_CONTROLLER_H
#define __LED_CONTROLLER_H

#include "main.h"

enum {
    LED_COLOR_RED = 0,
    LED_COLOR_GREEN,
    LED_COLOR_ORANGE,
    LED_PRIORITY_NORMAL = 0,
};

void led_start_blinking_color(uint8_t color, uint16_t period_ms, uint8_t count, uint8_t priority);

#endif
