/*
 * OLED 菜单树定义
 *
 * 树结构:
 *   g_root (Main)
 *   ├─ [Select]            ← 文件夹
 *   │  ├─ Que1             ← 叶子: 禁飞区数据
 *   │  ├─ Que2             ← 叶子: 禁飞区数据
 *   │  ├─ servo_angle      ← 可编辑 int32 (0~180, 步进循环切换)
 *   │  └─ test_float       ← 可编辑 float (演示)
 *   └─ [More]              ← 文件夹 (演示滚动)
 *      ├─ item_a ~ item_e  ← 5 项, 超过 3 项自动滚动
 */

#include "OLED_Menu.h"
#include "OLED.h"
#include "encode.h"
#include "driver_registry.h"
#include "stdio.h"
#include "..\NRF\nrf.h"


#define FIRST_X    0
#define SECOND_X   64


#if UNDERSTAND_RADAR_VEL_CHANGE
#include "PID_ctrl.h"
static void s_draw_kp(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    if (me->data_f)
    {
        pid_set_xy_kp(*(me->data_f));  
    }
}
static void s_draw_ki(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    if (me->data_f)
    {
        pid_set_xy_ki(*(me->data_f));   
    }
}
static void s_draw_kd(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    if (me->data_f)
    {
        pid_set_xy_kd(*(me->data_f));    
    }
}
static struct menu_node_t s_pid_folder_st;

static struct menu_node_t s_kp_st;
static struct menu_node_t s_ki_st;
static struct menu_node_t s_kd_st;

static float s_kp_f = 0.18;
static float s_ki_f = 0.2;
static float s_kd_f = 0.5;


#endif

/* ================================================================
 * 叶子 draw 函数
 * ================================================================ */

/* Que1/Que2 共用: 显示禁飞区数据 */
static void s_draw_encode(struct menu_node_t *self)
{
    OLED_ShowString(0, 0, self->base.name, OLED_8X16);

    EncoderState_t left, right;
    encoder_get_state(ENCODER_LEFT, &left);
    encoder_get_state(ENCODER_RIGHT, &right);

    /* 128x64, 8x16 font = 16col x 4row */
    OLED_Printf(0, 16, OLED_8X16, "L:%d %d pps", left.delta, left.speed_pps);
    OLED_Printf(0, 32, OLED_8X16, "R:%d %d pps", right.delta, right.speed_pps);
    OLED_Printf(0, 48, OLED_8X16, "T:%d %d",     left.total, right.total);
}
static void s_draw_nrf(struct menu_node_t *self)
{
    OLED_ShowString(0, 0, self->base.name, OLED_8X16);

    /* 显示飞控发来的指令 (NRF_RxPacket 8 字节) */
    OLED_Printf(0, 16, OLED_6X8, "RX:%02X %02X %02X %02X",
                NRF_RxPacket[0], NRF_RxPacket[1],
                NRF_RxPacket[2], NRF_RxPacket[3]);
    OLED_Printf(0, 28, OLED_6X8, "   %02X %02X %02X %02X",
                NRF_RxPacket[4], NRF_RxPacket[5],
                NRF_RxPacket[6], NRF_RxPacket[7]);

    /* 把前2字节当 int16 解析 (speed, turn) */
    int16_t speed = (int16_t)(NRF_RxPacket[0] | (NRF_RxPacket[1] << 8));
    int16_t turn  = (int16_t)(NRF_RxPacket[2] | (NRF_RxPacket[3] << 8));
    OLED_Printf(0, 44, OLED_8X16, "S:%d T:%d", speed, turn);
}
#if PHASE_DEBUG


static struct menu_node_t s_vel_z_st;
static int16_t vel_z_s = 0;
static void s_draw_vel_z(struct menu_node_t *me)
{
    
    if (me->data)
    {
        ano_lx_set_rt_vel_z(vel_z_s);
    }
    OLED_ShowString(0,0,me->base.name, OLED_8X16);
    vano_WTS_set(pstAnobase_Lx,0x41,1);
}


static enum fly_task_phase_e s_phase_em = FLY_PHASE_IDLE_em;

static void s_draw_phase(struct menu_node_t* me)
{
    if (me->data)
    {
        fly_task_set_phase_v((enum fly_task_phase_e)*me->data);
    }
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    enum fly_task_phase_e phase_em = fly_task_get_phase_em();
    char buf[16];
    sprintf(buf,"phase:%d",phase_em);
    OLED_ShowString(8,20,buf,OLED_8X16);
    
}
#endif

/* 舵机角度 int32 */


/* ================================================================
 * 静态节点实例
 * ================================================================ */

struct menu_node_t g_root;
static struct menu_node_t s_encode_st;
static struct menu_node_t select;
struct menu_base_t* g_encode_pst;
static struct menu_node_t s_nrf_st;

static void s_build_menu_tree(void)
{
    /* 根节点 (me 传结构体本身, 不传指针) */
    Create_Menu_Folder(NULL, g_root, "Main");

    Create_Menu_Folder(&g_root, select, "select");

    g_encode_pst = Create_Menu_Leaf(&select, s_encode_st, "encode", s_draw_encode);
    Create_Menu_Leaf(&select, s_nrf_st, "nrf", s_draw_nrf);
    
    /* 演示滚动: More 文件夹有 5 个子项 (超过 3 项自动滚动) */
}

/* ================================================================
 * 初始化
 * ================================================================ */

void menu_oled_init_v(void)
{
    menu_init_v();
    s_build_menu_tree();
}
DRIVER_INIT(menu_oled_init_v);
