#include "xvji.h"

static struct xvji_t      s_cache;
static enum xvji_state_e  s_state = STRAIGHT_em;

/*
 * 状态判定 (S1~S4 从左到右, HIGH = 检测到黑线)
 *
 *   S1 S2 S3 S4
 *   ───────────
 *   0  1  1  0  → 直行 (中间两路在线)
 *   1  1  0  0  → 左偏
 *   0  1  0  0  → 左偏
 *   1  1  1  0  → 大左 (连续三路偏左)
 *   1  0  0  0  → 大左 (仅最左传感器)
 *   0  0  1  1  → 右偏
 *   0  0  1  0  → 右偏
 *   0  1  1  1  → 大右 (连续三路偏右)
 *   0  0  0  1  → 大右 (仅最右传感器)
 *   0  0  0  0  → 保持上次状态
 *   1  1  1  1  → 直行 (十字路口)
 *   1  0  1  0  → 直行
 *   1  0  0  1  → 保持 (异常)
 */
static enum xvji_state_e s_calc_state(struct xvji_t *raw)
{
    uint8_t mask = (raw->S1_b << 3)
                 | (raw->S2_b << 2)
                 | (raw->S3_b << 1)
                 | (raw->S4_b << 0);

    switch (mask) {
    case 0x6:  // 0110
    case 0xF:  // 1111  十字路口
    case 0xA:  // 1010
        return STRAIGHT_em;

    case 0xC:  // 1100
    case 0x4:  // 0100
        return LEFT_em;

    case 0xE:  // 1110
    case 0x8:  // 1000
        return LEFT_HIGH_em;

    case 0x3:  // 0011
    case 0x2:  // 0010
        return RIGHT_em;

    case 0x7:  // 0111
    case 0x1:  // 0001
        return RIGHT_HIGH_em;

    default:
        return s_state;  // 0x0, 0x5, 0x9, 0xD, 0xB — 异常/全灭, 保持
    }
}

void xvji_sample(void)
{
    s_cache.S1_b = (HAL_GPIO_ReadPin(S1_GPIO_Port, S1_Pin) == GPIO_PIN_SET);
    s_cache.S2_b = (HAL_GPIO_ReadPin(S2_GPIO_Port, S2_Pin) == GPIO_PIN_SET);
    s_cache.S3_b = (HAL_GPIO_ReadPin(S3_GPIO_Port, S3_Pin) == GPIO_PIN_SET);
    s_cache.S4_b = (HAL_GPIO_ReadPin(S4_GPIO_Port, S4_Pin) == GPIO_PIN_SET);

    s_state = s_calc_state(&s_cache);
}

void xvji_copy(struct xvji_t *out)
{
    if (out) {
        *out = s_cache;
    }
}

enum xvji_state_e xvji_get_state(void)
{
    return s_state;
}
