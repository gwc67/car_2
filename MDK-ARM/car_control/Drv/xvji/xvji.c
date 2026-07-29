#include "xvji.h"

/* 内部缓存 — 由 xvji_sample() 周期更新 */
static struct xvji_t s_cache;

/*
 * 轮询采样: 读 5 路 GPIO → 写缓存
 *
 * P0 — GPIOB PIN_13
 * P1 — GPIOB PIN_12
 * P2 — GPIOB PIN_10
 * P3 — GPIOB PIN_11  (CubeMX 命名为 P2B11)
 * P4 — GPIOB PIN_2
 */
void xvji_sample(void)
{
    s_cache.S1_b = (HAL_GPIO_ReadPin(S1_GPIO_Port, S1_Pin) == GPIO_PIN_SET);
    s_cache.S2_b = (HAL_GPIO_ReadPin(S2_GPIO_Port, S2_Pin) == GPIO_PIN_SET);
    s_cache.S3_b = (HAL_GPIO_ReadPin(S3_GPIO_Port, S3_Pin) == GPIO_PIN_SET);
    s_cache.S4_b = (HAL_GPIO_ReadPin(S4_GPIO_Port, S4_Pin) == GPIO_PIN_SET);
}

/*
 * 获取快照: 结构体值拷贝, 不读硬件
 */
void xvji_copy(struct xvji_t *out)
{
    if (out) {
        *out = s_cache;
    }
}
