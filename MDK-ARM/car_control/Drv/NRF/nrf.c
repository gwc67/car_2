#include "nrf.h"
#include "nrf_define.h"
#include "spi.h"
#include "driver_registry.h"

/*
 * 适配 CE 硬接 VCC 的接法 (6 线: SCK/MOSI/MISO/CSN/VCC/GND)
 *
 * 原理:
 *   - CE 常高 → 芯片始终不在 Standby-I
 *   - 模式切换全靠 CONFIG 的 PRIM_RX 位
 *   - TX: PRIM_RX=0 + 写 TX FIFO → 芯片自动进入发射 (CE 已为高)
 *   - RX: PRIM_RX=1 + PWR_UP=1 → 芯片持续接收
 */

/* ================================================================
 * 全局数组
 * ================================================================ */
uint8_t NRF_TxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
uint8_t NRF_TxPacket[NRF_TX_PACKET_WIDTH];

uint8_t NRF_RxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
uint8_t NRF_RxPacket[NRF_RX_PACKET_WIDTH];

/* ================================================================
 * GPIO: 仅 CSN (CE 已接 VCC, 不需要控制)
 * ================================================================ */
#define CSN_LOW()  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_RESET)
#define CSN_HIGH() HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET)

/* ================================================================
 * 硬件 SPI 字节交换
 * ================================================================ */
static uint8_t spi_swap(uint8_t tx)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
    return rx;
}

/* ================================================================
 * 底层 SPI 指令 (CSN 控制片选)
 * ================================================================ */

static uint8_t nrf_read_reg(uint8_t reg)
{
    uint8_t data;
    CSN_LOW();
    spi_swap(NRF_R_REGISTER | reg);
    data = spi_swap(NRF_NOP);
    CSN_HIGH();
    return data;
}

static void nrf_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    spi_swap(NRF_R_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_swap(NRF_NOP);
    }
    CSN_HIGH();
}

static void nrf_write_reg(uint8_t reg, uint8_t data)
{
    CSN_LOW();
    spi_swap(NRF_W_REGISTER | reg);
    spi_swap(data);
    CSN_HIGH();
}

static void nrf_write_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    spi_swap(NRF_W_REGISTER | reg);
    for (uint8_t i = 0; i < len; i++) {
        spi_swap(buf[i]);
    }
    CSN_HIGH();
}

static uint8_t nrf_read_status(void)
{
    uint8_t st;
    CSN_LOW();
    st = spi_swap(NRF_NOP);
    CSN_HIGH();
    return st;
}

static void nrf_flush_tx(void)
{
    CSN_LOW();
    spi_swap(NRF_FLUSH_TX);
    CSN_HIGH();
}

static void nrf_flush_rx(void)
{
    CSN_LOW();
    spi_swap(NRF_FLUSH_RX);
    CSN_HIGH();
}

static void nrf_write_tx_payload(uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    spi_swap(NRF_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) {
        spi_swap(buf[i]);
    }
    CSN_HIGH();
}

static void nrf_read_rx_payload(uint8_t *buf, uint8_t len)
{
    CSN_LOW();
    spi_swap(NRF_R_RX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = spi_swap(NRF_NOP);
    }
    CSN_HIGH();
}

/* ================================================================
 * 模式切换 (仅通过 CONFIG 寄存器, 不再操作 CE)
 * ================================================================ */

void NRF_PowerDown(void)
{
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg &= ~0x02;                          // PWR_UP = 0
    nrf_write_reg(NRF_CONFIG, cfg);
}

void NRF_RxMode(void)
{
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg |= 0x03;                           // PWR_UP=1, PRIM_RX=1 → RX
    nrf_write_reg(NRF_CONFIG, cfg);
    /* CE 已接 VCC, 写完后芯片立即进入 RX */
}

void NRF_TxMode(void)
{
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg |= 0x02;                           // PWR_UP=1
    cfg &= ~0x01;                          // PRIM_RX=0 → Standby-II
    nrf_write_reg(NRF_CONFIG, cfg);
    /* CE=1 + PRIM_RX=0 → 等待 TX FIFO 写入即自动发射 */
}

/* ================================================================
 * 初始化
 * ================================================================ */

static void NRF_InitInternal(void)
{
    CSN_HIGH();                            // CSN 默认高, 不选中

    /* 先进入掉电模式, 确保已知状态 */
    nrf_write_reg(NRF_CONFIG, 0x00);       // PWR_UP=0

    /* 写入配置 (PWR_UP 仍为 0, 安全写入) */
    nrf_write_reg(NRF_CONFIG,      0x00);   // 稍后统一使能
    nrf_write_reg(NRF_EN_AA,       0x3F);   // 通道 0~5 自动应答
    nrf_write_reg(NRF_EN_RXADDR,   0x01);   // 只使能通道 0
    nrf_write_reg(NRF_SETUP_AW,    0x03);   // 地址 5 字节
    nrf_write_reg(NRF_SETUP_RETR,  0x03);   // 250µs 间隔, 重传 3 次
    nrf_write_reg(NRF_RF_CH,       0x02);   // 2.402GHz
    nrf_write_reg(NRF_RF_SETUP,    0x0E);   // 2Mbps, 0dBm

    nrf_write_reg(NRF_RX_PW_P0, NRF_RX_PACKET_WIDTH);
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);

    nrf_flush_tx();
    nrf_flush_rx();

    /* 清中断标志 */
    nrf_write_reg(NRF_STATUS, 0x70);

    /* 进入接收模式: PWR_UP=1, PRIM_RX=1 (CE 已硬接 VCC) */
    NRF_RxMode();
}

void NRF_Init(void)
{
    NRF_InitInternal();
}
DRIVER_INIT(NRF_Init);

/* ================================================================
 * 发送 (CE 常高, 写 FIFO 自动触发 TX)
 * ================================================================ */

uint8_t NRF_Send(void)
{
    /* 先切到 Standby-II: PRIM_RX=0, PWR_UP=1 */
    NRF_TxMode();

    /* 设置发送地址 */
    nrf_write_regs(NRF_TX_ADDR,    NRF_TxAddress, 5);
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_TxAddress, 5);

    /* 写入载荷 → 因 CE=1 + PRIM_RX=0, 写入后自动开始发射 */
    nrf_write_tx_payload(NRF_TxPacket, NRF_TX_PACKET_WIDTH);

    /* 等待完成 */
    uint32_t timeout = 100000;
    uint8_t result;
    while (1) {
        uint8_t st = nrf_read_status();
        timeout--;

        if (timeout == 0) {
            result = NRF_SEND_TIMEOUT;
            NRF_InitInternal();
            break;
        }
        if ((st & 0x30) == 0x30) {
            result = NRF_SEND_ERR;
            NRF_InitInternal();
            break;
        }
        if (st & NRF_STATUS_MAX_RT) {
            result = NRF_SEND_MAX_RT;
            NRF_InitInternal();
            break;
        }
        if (st & NRF_STATUS_TX_DS) {
            result = NRF_SEND_OK;
            break;
        }
    }

    /* 清 TX 标志 + 清空 FIFO */
    nrf_write_reg(NRF_STATUS, 0x30);
    nrf_flush_tx();

    /* 恢复接收通道 + 切回 RX */
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);
    NRF_RxMode();

    return result;
}

/* ================================================================
 * 接收
 * ================================================================ */

uint8_t NRF_Receive(void)
{
    uint8_t st  = nrf_read_status();
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);

    if ((cfg & 0x02) == 0x00) {
        NRF_InitInternal();
        return NRF_RECV_PWRDN;
    }
    if ((st & 0x30) == 0x30) {
        NRF_InitInternal();
        return NRF_RECV_ERR;
    }
    if (st & NRF_STATUS_RX_DR) {
        nrf_read_rx_payload(NRF_RxPacket, NRF_RX_PACKET_WIDTH);
        nrf_write_reg(NRF_STATUS, 0x40);
        nrf_flush_rx();
        return NRF_RECV_OK;
    }

    return NRF_RECV_NONE;
}

/* ================================================================
 * 更新接收地址
 * ================================================================ */

void NRF_UpdateRxAddress(void)
{
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);
}

/* ================================================================
 * 调试
 * ================================================================ */

uint8_t NRF_ReadReg(uint8_t reg)
{
    return nrf_read_reg(reg);
}

void NRF_DumpRegs(uint8_t buf[8])
{
    buf[0] = nrf_read_reg(NRF_CONFIG);
    buf[1] = nrf_read_reg(NRF_EN_AA);
    buf[2] = nrf_read_reg(NRF_EN_RXADDR);
    buf[3] = nrf_read_reg(NRF_SETUP_RETR);
    buf[4] = nrf_read_reg(NRF_RF_CH);
    buf[5] = nrf_read_reg(NRF_RF_SETUP);
    buf[6] = nrf_read_reg(NRF_STATUS);
    buf[7] = nrf_read_reg(NRF_FIFO_STATUS);
}
