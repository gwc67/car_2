#ifndef __XVJI_H
#define __XVJI_H

#include "main.h"
#include <stdbool.h>

/* 5 路循迹模块状态 */
struct xvji_t {
    bool S1_b;
    bool S2_b;
    bool S3_b;
    bool S4_b;
};

/* 轮询采样: 读 GPIO → 更新内部缓存, 调用频率自定 (如 1ms / 10ms) */
void xvji_sample(void);

/* 获取快照: 只读, 不碰硬件, 安全用于 OLED/日志/控制回路 */
void xvji_copy(struct xvji_t *out);

#endif
