#include "encode.h"
#include "tim.h"
#include "driver_registry.h"
#include "FreeRTOS.h"
#include "task.h"

/* ═══════════════════════════════════════════════════════════
 *  sample / get 分离架构
 *
 *  sample:  10ms 调度任务独占调用 → 读硬件 CNT + 算 delta + 更新内部状态
 *  get:     OLED / debug / 任何人调用 → 只读快照，不碰硬件，不改状态
 *
 *  这样 OLED 随时刷新，不会污染 10ms 控制回路的 delta 计算。
 * ═══════════════════════════════════════════════════════════ */

struct encoder_runtime {
    TIM_HandleTypeDef *htim;
    uint16_t prev_count;
    int32_t  total;
    uint32_t last_ms;
    bool     started;

    /* 缓存最近一次 sample 结果，供 get 快照 */
    EncoderState_t cached;
};

static struct encoder_runtime s_enc[ENCODER_NUM] = {
    [ENCODER_LEFT]  = { .htim = &htim3 },
    [ENCODER_RIGHT] = { .htim = &htim4 },
};

/* --- 周期采样（改状态） --- */
void encoder_sample(EncoderId_e id)
{
    if (id >= ENCODER_NUM) return;

    struct encoder_runtime *rt = &s_enc[id];

    if (!rt->started) {
        HAL_TIM_Encoder_Start(rt->htim, TIM_CHANNEL_ALL);
        rt->prev_count = __HAL_TIM_GET_COUNTER(rt->htim);
        rt->total      = 0;
        rt->last_ms    = xTaskGetTickCount();
        rt->started    = true;

        rt->cached.raw_count    = rt->prev_count;
        rt->cached.delta        = 0;
        rt->cached.total        = 0;
        rt->cached.last_tick_ms = rt->last_ms;
        rt->cached.speed_pps    = 0;
        return;
    }

    uint16_t cur = __HAL_TIM_GET_COUNTER(rt->htim);
    uint32_t now = xTaskGetTickCount();

    int16_t delta  = (int16_t)((uint16_t)(cur - rt->prev_count));
    rt->total     += delta;

    uint32_t dt = now - rt->last_ms;
    int16_t speed;
    if (dt > 0) {
        speed = (int16_t)((int32_t)delta * 1000 / (int32_t)dt);
    } else {
        speed = rt->cached.speed_pps;  // 保持上次值
    }

    rt->prev_count = cur;
    rt->last_ms    = now;

    /* 写入缓存，供 get 读取 */
    rt->cached.raw_count    = cur;
    rt->cached.delta        = delta;
    rt->cached.total        = rt->total;
    rt->cached.last_tick_ms = now;
    rt->cached.speed_pps    = speed;
}

/* --- 只读快照（不改状态） --- */
void encoder_get_state(EncoderId_e id, EncoderState_t *out)
{
    if (id >= ENCODER_NUM || !out) return;
    *out = s_enc[id].cached;
}

/* --- 批量 --- */
void encoder_sample_all(void)
{
    for (int i = 0; i < ENCODER_NUM; i++) {
        encoder_sample((EncoderId_e)i);
    }
}

void encoder_get_all(EncoderState_t out[ENCODER_NUM])
{
    for (int i = 0; i < ENCODER_NUM; i++) {
        encoder_get_state((EncoderId_e)i, &out[i]);
    }
}

/* --- 自动初始化 --- */
static void s_encoder_init(void)
{
    for (int i = 0; i < ENCODER_NUM; i++) {
        s_enc[i].started    = false;
        s_enc[i].prev_count = 0;
        s_enc[i].total      = 0;
        s_enc[i].last_ms    = 0;
    }
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_Base_Start(&htim4);
    
}
DRIVER_INIT(s_encoder_init);
