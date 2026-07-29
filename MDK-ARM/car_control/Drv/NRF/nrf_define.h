#ifndef __NRF_DEFINE_H
#define __NRF_DEFINE_H

/* ================================================================
 * NRF24L01 指令码
 * ================================================================ */
#define NRF_R_REGISTER          0x00    // 读寄存器 (低5位=地址)
#define NRF_W_REGISTER          0x20    // 写寄存器 (低5位=地址)
#define NRF_R_RX_PAYLOAD        0x61    // 读 Rx FIFO 有效载荷
#define NRF_W_TX_PAYLOAD        0xA0    // 写 Tx 有效载荷
#define NRF_FLUSH_TX            0xE1    // 清空 Tx FIFO
#define NRF_FLUSH_RX            0xE2    // 清空 Rx FIFO
#define NRF_REUSE_TX_PL         0xE3    // 重用上次发送载荷
#define NRF_R_RX_PL_WID         0x60    // 读 Rx 包宽 (动态包长模式)
#define NRF_W_ACK_PAYLOAD       0xA8    // 写 ACK 附带载荷
#define NRF_W_TX_PAYLOAD_NOACK  0xB0    // 写 Tx 载荷 (不要求 ACK)
#define NRF_NOP                 0xFF    // 空操作

/* ================================================================
 * NRF24L01 寄存器地址
 * ================================================================ */
#define NRF_CONFIG              0x00    // 配置寄存器
#define NRF_EN_AA               0x01    // 使能自动应答
#define NRF_EN_RXADDR           0x02    // 使能接收通道
#define NRF_SETUP_AW            0x03    // 地址宽度
#define NRF_SETUP_RETR          0x04    // 自动重传
#define NRF_RF_CH               0x05    // 射频通道
#define NRF_RF_SETUP            0x06    // 射频参数
#define NRF_STATUS              0x07    // 状态寄存器
#define NRF_OBSERVE_TX          0x08    // 发送观察
#define NRF_RPD                 0x09    // 接收功率检测
#define NRF_RX_ADDR_P0          0x0A    // 接收通道 0 地址 (5 字节)
#define NRF_RX_ADDR_P1          0x0B    // 接收通道 1 地址 (5 字节)
#define NRF_RX_ADDR_P2          0x0C    // 接收通道 2 地址 (1 字节)
#define NRF_RX_ADDR_P3          0x0D    // 接收通道 3 地址 (1 字节)
#define NRF_RX_ADDR_P4          0x0E    // 接收通道 4 地址 (1 字节)
#define NRF_RX_ADDR_P5          0x0F    // 接收通道 5 地址 (1 字节)
#define NRF_TX_ADDR             0x10    // 发送地址 (5 字节)
#define NRF_RX_PW_P0            0x11    // 接收通道 0 载荷宽度
#define NRF_RX_PW_P1            0x12    // 接收通道 1 载荷宽度
#define NRF_RX_PW_P2            0x13    // 接收通道 2 载荷宽度
#define NRF_RX_PW_P3            0x14    // 接收通道 3 载荷宽度
#define NRF_RX_PW_P4            0x15    // 接收通道 4 载荷宽度
#define NRF_RX_PW_P5            0x16    // 接收通道 5 载荷宽度
#define NRF_FIFO_STATUS         0x17    // FIFO 状态
#define NRF_DYNPD               0x1C    // 动态包长使能
#define NRF_FEATURE             0x1D    // 高级功能

/* ================================================================
 * STATUS 寄存器位
 * ================================================================ */
#define NRF_STATUS_RX_DR        0x40    // RX 数据就绪
#define NRF_STATUS_TX_DS        0x20    // TX 发送完成
#define NRF_STATUS_MAX_RT       0x10    // 达到最大重发次数

#endif
