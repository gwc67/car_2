#ifndef __NRF_H
#define __NRF_H

#include "main.h"
#include <stdbool.h>

/* ---- 数据包宽度 (可调, 1~32) ---- */
#define NRF_TX_PACKET_WIDTH  8
#define NRF_RX_PACKET_WIDTH  8

/* ---- 全局数组: 上层直接读写 ---- */
extern uint8_t NRF_TxAddress[5];
extern uint8_t NRF_TxPacket[NRF_TX_PACKET_WIDTH];

extern uint8_t NRF_RxAddress[5];
extern uint8_t NRF_RxPacket[NRF_RX_PACKET_WIDTH];

/* ---- 发送状态码 ---- */
enum nrf_send_result_e {
    NRF_SEND_OK       = 1,   // 发送成功
    NRF_SEND_MAX_RT   = 2,   // 重发达上限, 未收到 ACK
    NRF_SEND_ERR      = 3,   // 状态寄存器异常
    NRF_SEND_TIMEOUT  = 4,   // 超时
};

/* ---- 接收状态码 ---- */
enum nrf_recv_result_e {
    NRF_RECV_NONE     = 0,   // 无数据
    NRF_RECV_OK       = 1,   // 收到一个包
    NRF_RECV_ERR      = 2,   // 状态寄存器异常
    NRF_RECV_PWRDN    = 3,   // 芯片处于掉电模式
};

/* ---- API ---- */
void NRF_Init(void);
uint8_t NRF_Send(void);
uint8_t NRF_Receive(void);
void NRF_UpdateRxAddress(void);

/* 模式切换 (一般不需要手动调用, Send/Receive 内部已处理) */
void NRF_PowerDown(void);
void NRF_RxMode(void);
void NRF_TxMode(void);

/* 调试: 读单个寄存器 */
uint8_t NRF_ReadReg(uint8_t reg);

/* 调试: 批量读取关键寄存器到 buf[8] — 方便 OLED 打印
 * buf[0]=CONFIG, [1]=EN_AA, [2]=EN_RXADDR, [3]=SETUP_RETR,
 * buf[4]=RF_CH, [5]=RF_SETUP, [6]=STATUS, [7]=FIFO_STATUS */
void NRF_DumpRegs(uint8_t buf[8]);

#endif
