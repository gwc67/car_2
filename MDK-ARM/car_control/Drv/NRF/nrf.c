#include "nrf.h"
#include "nrf_define.h"
#include "spi.h"
#include "driver_registry.h"

/* ================================================================
 * 全局数组
 * ================================================================ */
uint8_t NRF_TxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
uint8_t NRF_TxPacket[NRF_TX_PACKET_WIDTH];

uint8_t NRF_RxAddress[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
uint8_t NRF_RxPacket[NRF_RX_PACKET_WIDTH];

/* ================================================================
 * GPIO 宏: CE / CSN
 * ================================================================ */
#define CE_LOW()   HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_RESET)
#define CE_HIGH()  HAL_GPIO_WritePin(CE_GPIO_Port, CE_Pin, GPIO_PIN_SET)
#define CSN_LOW()  HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_RESET)
#define CSN_HIGH() HAL_GPIO_WritePin(CSN_GPIO_Port, CSN_Pin, GPIO_PIN_SET)

/* ================================================================
 * 硬件 SPI 字节交换 (替代原软件 SPI_SwapByte)
 * ================================================================ */
static uint8_t spi_swap(uint8_t tx)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
    return rx;
}

/* ================================================================
 * 底层 SPI 指令
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
 * 模式切换
 * ================================================================ */

void NRF_PowerDown(void)
{
    CE_LOW();
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg &= ~0x02;                          // PWR_UP = 0
    nrf_write_reg(NRF_CONFIG, cfg);
}

void NRF_RxMode(void)
{
    CE_LOW();
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg |= 0x03;                           // PWR_UP=1, PRIM_RX=1
    nrf_write_reg(NRF_CONFIG, cfg);
    CE_HIGH();
}

void NRF_TxMode(void)
{
    CE_LOW();
    uint8_t cfg = nrf_read_reg(NRF_CONFIG);
    if (cfg == 0xFF) return;
    cfg |= 0x02;                           // PWR_UP=1
    cfg &= ~0x01;                          // PRIM_RX=0
    nrf_write_reg(NRF_CONFIG, cfg);
    CE_HIGH();
}

/* ================================================================
 * 初始化
 * ================================================================ */

static void NRF_InitInternal(void)
{
    /* CE=0, CSN=1 初始电平 */
    CE_LOW();
    CSN_HIGH();

    nrf_write_reg(NRF_CONFIG,      0x08);   // CRC 1字节, PWR_UP=0, PRIM_RX=0
    nrf_write_reg(NRF_EN_AA,       0x3F);   // 通道0~5 自动应答全开
    nrf_write_reg(NRF_EN_RXADDR,   0x01);   // 只使能接收通道0
    nrf_write_reg(NRF_SETUP_AW,    0x03);   // 地址宽度 5 字节
    nrf_write_reg(NRF_SETUP_RETR,  0x03);   // 重传间隔 250us, 最多 3 次
    nrf_write_reg(NRF_RF_CH,       0x02);   // 2.402GHz
    nrf_write_reg(NRF_RF_SETUP,    0x0E);   // 2Mbps, 0dBm

    nrf_write_reg(NRF_RX_PW_P0, NRF_RX_PACKET_WIDTH);
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);

    nrf_flush_tx();
    nrf_flush_rx();

    /* 清中断标志 */
    nrf_write_reg(NRF_STATUS, 0x70);

    /* 默认进入接收模式 */
    NRF_RxMode();
}

DRIVER_INIT(NRF_InitInternal);

/* ================================================================
 * 发送
 * ================================================================ */

uint8_t NRF_Send(void)
{
    /* 设置发送地址 (收发地址一致, 方便应答) */
    nrf_write_regs(NRF_TX_ADDR,    NRF_TxAddress, 5);
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_TxAddress, 5);

    /* 写入载荷 */
    nrf_write_tx_payload(NRF_TxPacket, NRF_TX_PACKET_WIDTH);

    /* 进入发送模式 */
    NRF_TxMode();

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

    /* 清 TX 中断标志 + 清空 FIFO */
    nrf_write_reg(NRF_STATUS, 0x30);
    nrf_flush_tx();

    /* 恢复接收通道地址 + 回到接收模式 */
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);
    NRF_RxMode();

    return result;
}

/* ================================================================
 * 接收
 * ================================================================ */

uint8_t NRF_Receive(void)
{
    uint8_t st   = nrf_read_status();
    uint8_t cfg  = nrf_read_reg(NRF_CONFIG);

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
 * 更新接收地址 (运行中修改后调用)
 * ================================================================ */

void NRF_UpdateRxAddress(void)
{
    nrf_write_regs(NRF_RX_ADDR_P0, NRF_RxAddress, 5);
}

/* ================================================================
 * 调试函数
 * ================================================================ */

/* 读单个寄存器 (供 OLED/串口调试) */
uint8_t NRF_ReadReg(uint8_t reg)
{
    return nrf_read_reg(reg);
}

/* 批量读取关键寄存器 — 方便 OLED 一屏打印验证 */
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
