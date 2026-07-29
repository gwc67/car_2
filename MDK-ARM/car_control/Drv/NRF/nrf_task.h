#ifndef __NRF_TASK_H
#define __NRF_TASK_H

#include "nrf.h"
#include <stdbool.h>

/* 遥控指令包 (双方约定格式) */
struct nrf_cmd_t {
    int16_t speed;        // 目标速度
    int16_t turn;         // 转向量
    uint8_t flags;        // 标志位 (自定义)
    uint8_t seq;          // 包序号 (去重/丢包检测)
};

/*
 * 小车端 — 在调度任务中周期性调用:
 *   nrf_cmd_task() → 检查是否有新指令 → 写入 out
 *   返回 true 表示收到新包
 */
bool nrf_cmd_recv(struct nrf_cmd_t *out);

/*
 * 遥控器端 — 填充 cmd 后调用:
 *   nrf_cmd_send(&cmd) → 返回 NRF_SEND_OK 表示发送成功
 */
uint8_t nrf_cmd_send(struct nrf_cmd_t *cmd);

#endif
