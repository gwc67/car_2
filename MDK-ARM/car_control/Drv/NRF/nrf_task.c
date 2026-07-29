#include "nrf_task.h"
#include <string.h>

static struct nrf_cmd_t s_last_cmd;
static uint8_t          s_last_seq;
static bool             s_has_new;

/* ---- 小车端：接收遥控指令 ---- */
bool nrf_cmd_recv(struct nrf_cmd_t *out)
{
    s_has_new = false;

    uint8_t ret = NRF_Receive();
    if (ret != NRF_RECV_OK) return false;

    /* 校验数据长度 (至少要有数据) */
    if (NRF_RX_PACKET_WIDTH < sizeof(struct nrf_cmd_t)) return false;

    memcpy(&s_last_cmd, NRF_RxPacket, sizeof(struct nrf_cmd_t));

    /* 包序号去重 */
    if (s_last_cmd.seq == s_last_seq) return false;
    s_last_seq = s_last_cmd.seq;
    s_has_new  = true;

    if (out) *out = s_last_cmd;
    return true;
}

/* ---- 遥控器端：发送指令 ---- */
uint8_t nrf_cmd_send(struct nrf_cmd_t *cmd)
{
    if (NRF_TX_PACKET_WIDTH < sizeof(struct nrf_cmd_t)) {
        return NRF_SEND_ERR;
    }
    cmd->seq++;  // 自动递增序号
    memcpy(NRF_TxPacket, cmd, sizeof(struct nrf_cmd_t));
    return NRF_Send();
}
