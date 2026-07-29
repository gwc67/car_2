#include "encode.h"
#include "tim.h"
#include "driver_registry.h"
#include "FreeRTOS.h"
#include "task.h"
/* ═══════════════════════════════════════════════════════════
 *  数据驱动 + 溢出安全算法
 *
 *  TIM3/TIM4 均为 16-bit 编码器模式，Period = 65535。
 *  硬件自动处理方向：CNT 递增 = 正转，CNT 递减 = 反转。
 *
 *  delta = (int16_t)((cur - prev) & 0xFFFF)
 *  利用 16-bit 无符号截断实现溢出安全：
 *    65535 → 0 (正转)  → delta = +1
 *    0 → 65535 (反转)  → delta = -1
 *  只要两次读取间脉冲变化 ≤ 32767 就不会误判方向。
 *  轮子转一圈所对应脉冲数跟电机有关，上层自行换算。
 * ═══════════════════════════════════════════════════════════ */

/* --- per-encoder runtime --- */
struct encoder_runtime {
    TIM_HandleTypeDef *htim;
    uint16_t prev_count;
    int32_t  total;
    uint32_t last_ms;
    bool     started;
};

static struct encoder_runtime s_enc[ENCODER_NUM] = {
    [ENCODER_LEFT]  = { .htim = &htim3 },
    [ENCODER_RIGHT] = { .htim = &htim4 },
};

/* --- 单次读取 --- */
void encoder_read(EncoderId_e id, EncoderState_t *out)
{
    if (id >= ENCODER_NUM || !out) return;

    struct encoder_runtime *rt = &s_enc[id];

    /* 首次读取：仅记初值，delta/speed 为 0 */
    if (!rt->started) {
        HAL_TIM_Encoder_Start(rt->htim, TIM_CHANNEL_ALL);
        rt->prev_count = __HAL_TIM_GET_COUNTER(rt->htim);
        rt->total     = 0;
        rt->last_ms   = xTaskGetTickCount();
        rt->started   = true;

        out->raw_count    = rt->prev_count;
        out->delta        = 0;
        out->total        = 0;
        out->last_tick_ms = rt->last_ms;
        out->speed_pps    = 0;
        return;
    }

    uint16_t cur  = __HAL_TIM_GET_COUNTER(rt->htim);
    uint32_t now  = xTaskGetTickCount();

    /* 16-bit 溢出安全的差值（有符号） */
    int16_t delta = (int16_t)((uint16_t)(cur - rt->prev_count));
    rt->total    += delta;

    /* 速度估算 (pps)，避免除零 */
    uint32_t dt = now - rt->last_ms;
    int16_t speed;
    if (dt > 0) {
        speed = (int16_t)((int32_t)delta * 1000 / (int32_t)dt);
    } else {
        speed = rt->total ? (int16_t)(rt->total > 0 ? 32767 : -32768) : 0;
    }

    /* 写回 */
    rt->prev_count = cur;
    rt->last_ms    = now;

    out->raw_count    = cur;
    out->delta        = delta;
    out->total        = rt->total;
    out->last_tick_ms = now;
    out->speed_pps    = speed;
}

/* --- 批量读取 --- */
void encoder_read_all(EncoderState_t out[ENCODER_NUM])
{
    for (int i = 0; i < ENCODER_NUM; i++) {
        encoder_read((EncoderId_e)i, &out[i]);
    }
}

/* --- 自动初始化 --- */
static void s_encoder_init(void)
{
    for (int i = 0; i < ENCODER_NUM; i++) {
        s_enc[i].started     = false;
        s_enc[i].prev_count  = 0;
        s_enc[i].total       = 0;
        s_enc[i].last_ms     = 0;
    }
}
DRIVER_INIT(s_encoder_init);
