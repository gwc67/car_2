#ifndef __ENCODE_H
#define __ENCODE_H

#include "main.h"

typedef enum {
    ENCODER_LEFT   = 0,   // TIM3 — PA6/PA7, filter=15 on CH1
    ENCODER_RIGHT,        // TIM4 — PB6/PB7
    ENCODER_NUM
} EncoderId_e;

typedef struct {
    int32_t  raw_count;        // 当前硬件计数值 (0~65535)
    int16_t  delta;            // 距上次读取的差值（带方向）
    int32_t  total;            // 累计总脉冲（不受溢出影响）
    uint32_t last_tick_ms;     // 上次读取的时刻 ms
    int16_t  speed_pps;        // 每秒脉冲数 (pulse per second)
} EncoderState_t;

void encoder_read(EncoderId_e id, EncoderState_t *out);
void encoder_read_all(EncoderState_t out[ENCODER_NUM]);

#endif
