#ifndef __ENCODE_H
#define __ENCODE_H

#include "main.h"

typedef enum {
    ENCODER_LEFT   = 0,   // TIM3 — PA6/PA7
    ENCODER_RIGHT,        // TIM4 — PB6/PB7
    ENCODER_NUM
} EncoderId_e;

typedef struct {
    int32_t  raw_count;        // 当前硬件计数值 (0~65535)
    int16_t  delta;            // 距上次采样的差值（带方向）
    int32_t  total;            // 累计总脉冲
    uint32_t last_tick_ms;     // 上次采样时刻 ms
    int16_t  speed_pps;        // 每秒脉冲数
} EncoderState_t;

/* 周期调用（10ms scheduler）: 读硬件 + 算delta/speed + 更新内部状态 */
void encoder_sample(EncoderId_e id);

/* 只读不打乱状态（OLED / debug）: 返回最近一次 sample 的快照 */
void encoder_get_state(EncoderId_e id, EncoderState_t *out);

/* 批量 */
void encoder_sample_all(void);
void encoder_get_all(EncoderState_t out[ENCODER_NUM]);

#endif
