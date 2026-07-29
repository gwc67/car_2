#ifndef __XVJI_H
#define __XVJI_H

#include "main.h"
#include <stdbool.h>

/* 4 路循迹传感器 (左→右: S1 S2 S3 S4) */
struct xvji_t {
    bool S1_b;
    bool S2_b;
    bool S3_b;
    bool S4_b;
};

/* 循迹状态 */
enum xvji_state_e {
    STRAIGHT_em,       // 直行
    LEFT_em,           // 左偏
    RIGHT_em,          // 右偏
    LEFT_HIGH_em,      // 大左转
    RIGHT_HIGH_em,     // 大右转
};

/* 轮询采样: 读 GPIO → 更新内部缓存 + 计算状态 */
void xvji_sample(void);

/* 获取快照: 4 路原始值 */
void xvji_copy(struct xvji_t *out);

/* 获取当前计算出的状态 */
enum xvji_state_e xvji_get_state(void);

#endif
