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
#include "oled_gfx.h"
#include "driver_registry.h"
#include "stdio.h"
#include "servos.h"
#include "My_ANO_LX.h"
#include "ano_device_jesnano.h"
#include "fly_task.h"
#include "Freq_Dectector.h"
#include "ano.h"

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
static void s_draw_question_1(struct menu_node_t *self)
{
    OLED_ShowString(0, 0, self->base.name, OLED_8X16);
    fly_task_set_question_mode(QUESTION_MODE_1_em);
    OLED_ShowString(0,20,"QUE1 SUCCESS!",OLED_8X16);
}
static void s_draw_question2_v(struct menu_node_t *self)
{

    OLED_ShowString(0,0,self->base.name,OLED_8X16);
    fly_task_set_question_mode(QUESTION_MODE_2_em);
    OLED_ShowString(8,20,"QUE2 SUCCESS!",OLED_8X16);

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

/* servo_angle: 同步舵机 */
static void s_draw_angle_v(struct menu_node_t *self)
{
    OLED_ShowString(0, 0, self->base.name, OLED_8X16);
    if (self->data)
        servo_set_angle(cammer_servo_pst, (uint8_t)*self->data);
}



static void s_draw_battery_v(struct menu_node_t *self)
{


    OLED_ShowString(0, 0, self->base.name, OLED_8X16);
    struct fc_bat_t battery_st;
    fc_bat_copy(&battery_st);
    char buf[16];
    sprintf(buf,"vol:%.2f",battery_st.voltage_100 / 100.0f);
    OLED_ShowString(0,20,buf,OLED_8X16);
    sprintf(buf,"cur:%.2f",battery_st.current_100 / 100.0f);
    OLED_ShowString(0,40,buf,OLED_8X16);
    sprintf(buf,"freq:%.2f",FreqDetector_GetFreq_db(freq_detector_pst[DATA_STREAM_BAT_CUR_em]));
    OLED_ShowString(70,50,buf,OLED_6X8);


}

/* ================================================================
 * 可编辑数据
 * ================================================================ */

static int32_t s_servo_angle = 0;      /* 舵机角度 int32 */


/* ================================================================
 * 静态节点实例
 * ================================================================ */

struct menu_node_t g_root;
//select下的文件
static struct menu_node_t g_menu_select;
static struct menu_node_t g_item_que1;
static struct menu_node_t g_item_que2;

#if PHASE_DEBUG
static struct menu_node_t g_phase_st;
#endif  
static struct menu_node_t g_servo_angle_st;
static struct menu_node_t g_battery_st;

//jesnano data 
static struct menu_node_t g_jesnano_data_st;
static struct menu_node_t g_jesnano_qua_st;
static struct menu_node_t g_jesnano_pos_st;
static struct menu_node_t g_jesnano_yaw_st;
static struct menu_node_t g_jesnano_speed_st;
static struct menu_node_t g_jesnano_stop_st;
static struct menu_node_t g_radar_st;
static struct menu_node_t g_radar_start_st;
static struct menu_node_t g_radar_stop_st;
static struct menu_node_t g_cam_start_st;

static struct menu_node_t g_delivery_st;

struct menu_base_t * g_jesnano_pos_oled_pst;
struct menu_base_t * g_jesnano_qua_oled_pst;
struct menu_base_t * g_jesnano_yaw_oled_pst;
struct menu_base_t * g_jesnano_speed_oled_pst;
struct menu_base_t * g_delivery_oled_pst;


static void s_draw_jesnano_cam_start(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    
    uint8_t cmd_byte_puc[1] = {0x01};
    vano_cmd_send_v(pstAnobase_Jesnano,0xff,0x33,cmd_byte_puc,sizeof(cmd_byte_puc)/sizeof(cmd_byte_puc[0]));
    OLED_ShowString(20,20,"cam_start!!!",OLED_8X16);
}



static void s_draw_jesnano_stop_v(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    
    uint8_t cmd_byte_puc[3] = {0xA5,0x5A,0x01};
    vano_cmd_send_v(pstAnobase_Jesnano,0xff,0x36,cmd_byte_puc,sizeof(cmd_byte_puc)/sizeof(cmd_byte_puc[0]));
    OLED_ShowString(20,20,"Jesnano Stop!!!",OLED_8X16);
}


static void s_draw_jesnano_qua_v(struct menu_node_t* me )
{
    OLED_ShowString(0, 0, me->base.name, OLED_8X16);
    struct Radar_Qua_t radar_qua_st;
    jesnano_radar_qua_copy(&radar_qua_st);

    char buf[16];
    sprintf(buf,"qx:%.2f",radar_qua_st.qX_f);
    OLED_ShowString(0,20,buf,OLED_8X16);
    sprintf(buf,"qy:%.2f",radar_qua_st.qY_f);
    OLED_ShowString(60,20,buf,OLED_8X16);
    sprintf(buf,"qz:%.2f",radar_qua_st.qZ_f);
    OLED_ShowString(0,40,buf,OLED_8X16);
    sprintf(buf,"qw:%.2f",radar_qua_st.qW_f);
    OLED_ShowString(60,40,buf,OLED_8X16);
}


static void s_draw_jesnano_pos_v(struct menu_node_t* me)
{
    OLED_ShowString(0, 0, me->base.name, OLED_8X16);
    struct Radar_Pos_t radar_pos_st;
    jesnano_radar_pos_copy(&radar_pos_st);

    char buf[16];
    sprintf(buf,"x:%d",radar_pos_st.x_s);
    OLED_ShowString(0,20,buf,OLED_8X16);
    sprintf(buf,"y:%d",radar_pos_st.y_s);
    OLED_ShowString(60,20,buf,OLED_8X16);
    sprintf(buf,"z:%d",radar_pos_st.z_s);
    OLED_ShowString(0,40,buf,OLED_8X16);

    double freq_db = FreqDetector_GetFreq_db(freq_detector_pst[DATA_STREAM_RADAR_POS_em]);
    sprintf(buf,"freq:%.2f",freq_db);
    OLED_ShowString(60,40,buf,OLED_6X8);
}

static void s_draw_jesnano_yaw_v(struct menu_node_t* me )
{
    OLED_ShowString(0, 0, me->base.name, OLED_8X16);
    struct Radar_Qua_t radar_qua_st;
    jesnano_radar_qua_copy(&radar_qua_st);

    const float qX_f = radar_qua_st.qX_f;
    const float qY_f = radar_qua_st.qY_f;
    const float qZ_f = radar_qua_st.qZ_f;
    const float qW_f = radar_qua_st.qW_f;

    float s_radar_current_yaw_f = -atan2(2 * qX_f * qY_f + 2 * qZ_f * qW_f, -2 * qY_f * qY_f - 2 * qZ_f * qZ_f + 1) * 57.2957795; // 四元数转换欧拉角yaw 旋转矩阵 //IMU给出来的也是负的yaw角
    
    double freq_db = FreqDetector_GetFreq_db(freq_detector_pst[Data_stream_Radar_qua_em]);
    char buf[16];
    sprintf(buf,"yaw:%.2f",s_radar_current_yaw_f);
    OLED_ShowString(0,20,buf,OLED_8X16);
    sprintf(buf,"freq:%.2f",freq_db);
    OLED_ShowString(0,40,buf,OLED_8X16);
}

static void s_draw_jesnano_speed_v(struct menu_node_t* me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    struct Radar_Speed_t radar_speed_st;
    jesnano_radar_speed_copy(&radar_speed_st);

    char buf[16];
    sprintf(buf,"spdx:%d",radar_speed_st.speed_x_s);
    OLED_ShowString(0,20,buf,OLED_8X16);
    sprintf(buf,"spdy:%d",radar_speed_st.speed_y_s);
    OLED_ShowString(60,20,buf,OLED_8X16);
    sprintf(buf,"spdz:%d",radar_speed_st.speed_z_s);
    OLED_ShowString(0,40,buf,OLED_8X16);

    double freq_db = FreqDetector_GetFreq_db(freq_detector_pst[DATA_STREAM_RADAR_SPEED_em]);
    sprintf(buf,"freq:%.2f",freq_db);
    OLED_ShowString(60,40,buf,OLED_6X8);
}

static void s_draw_jesnano_radar_start_v(struct menu_node_t *me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    OLED_ShowString(20,16,"RADAR_START !!!",OLED_6X8);
    HAL_GPIO_WritePin(Radar_Ctrl_GPIO_Port,Radar_Ctrl_Pin,GPIO_PIN_SET);
}

static void s_draw_jesnano_radar_stop_v(struct menu_node_t *me)
{
    OLED_ShowString(0,0,me->base.name,OLED_8X16);
    OLED_ShowString(20,16,"RADAR_STOP !!!",OLED_6X8);
    HAL_GPIO_WritePin(Radar_Ctrl_GPIO_Port,Radar_Ctrl_Pin,GPIO_PIN_RESET);
}

static void s_draw_jesnano_delivery_v(struct menu_node_t *me)
{
    OLED_ShowString(0,0,me->base.name,OLED_6X8);
    
    struct Jesnano_cam_raw_t jesnano_raw_st;
    jesnano_cam_raw_copy(&jesnano_raw_st);
    uint8_t type_uc ;
    jesnano_copy_type(&type_uc);

    char buf[16];
    sprintf(buf,"errorx:%d",jesnano_raw_st.cam_error_x_s);
    OLED_ShowString(0,10,buf,OLED_6X8);
    sprintf(buf,"errory:%d",jesnano_raw_st.cam_error_y_s);
    OLED_ShowString(64,10,buf,OLED_6X8);

    sprintf(buf,"type:%d",type_uc);
    OLED_ShowString(FIRST_X,20,buf,OLED_6X8);
    
}


/* 演示滚动: More 文件夹 + 5 个子项 */

struct menu_base_t* g_battery_oled_pst;
/* ================================================================
 * 构建菜单树
 * ================================================================ */

static void s_build_menu_tree(void)
{
    /* 根节点 (me 传结构体本身, 不传指针) */
    Create_Menu_Folder(NULL, g_root, "Main");

    /* Select 文件夹 */
    Create_Menu_Folder(&g_root, g_menu_select, "Select");
    Create_Menu_Leaf(&g_menu_select, g_item_que1, "Question_1", s_draw_question_1);
    Create_Menu_Leaf(&g_menu_select, g_item_que2, "Question_2", s_draw_question2_v);
    g_battery_oled_pst = Create_Menu_Leaf(&g_menu_select, g_battery_st, "battary", s_draw_battery_v);

    Create_Menu_Folder(&g_root,g_jesnano_data_st,"jesnano");
    g_jesnano_qua_oled_pst = Create_Menu_Leaf(&g_jesnano_data_st,g_jesnano_qua_st,"qua",s_draw_jesnano_qua_v);
    g_jesnano_pos_oled_pst = Create_Menu_Leaf(&g_jesnano_data_st,g_jesnano_pos_st,"pos",s_draw_jesnano_pos_v);
    g_jesnano_yaw_oled_pst = Create_Menu_Leaf(&g_jesnano_data_st,g_jesnano_yaw_st,"yaw",s_draw_jesnano_yaw_v);
    g_jesnano_speed_oled_pst = Create_Menu_Leaf(&g_jesnano_data_st,g_jesnano_speed_st,"speed",s_draw_jesnano_speed_v);
    g_delivery_oled_pst      = Create_Menu_Leaf(&g_jesnano_data_st,g_delivery_st,"delivery",s_draw_jesnano_delivery_v);
    Create_Menu_Leaf(&g_jesnano_data_st,g_jesnano_stop_st,"STOP JESNANO",s_draw_jesnano_stop_v);
    Create_Menu_Leaf(&g_jesnano_data_st,g_cam_start_st,"cam_start",s_draw_jesnano_cam_start);

    Create_Menu_Folder(&g_jesnano_data_st,g_radar_st,"radar");
    Create_Menu_Leaf(&g_radar_st,g_radar_start_st,"radar_start",s_draw_jesnano_radar_start_v);
    Create_Menu_Leaf(&g_radar_st,g_radar_stop_st,"radar_stop",s_draw_jesnano_radar_stop_v);

#if PHASE_DEBUG
    Create_Menu_Leaf_Int(&g_menu_select,g_phase_st,"fly_phase",s_draw_phase,(int32_t*)&s_phase_em);
    menu_set_default_int(&g_phase_st,0);

    Create_Menu_Leaf_Int(&g_menu_select,s_vel_z_st,"cmd_vel_z",s_draw_vel_z,(int32_t*)&vel_z_s);
    menu_set_default_int(&s_vel_z_st,0);
#endif

    /* int32 可编辑: 舵机角度 0~180 */
    Create_Menu_Leaf_Range(&g_menu_select, g_servo_angle_st,
                           "servo_angle", s_draw_angle_v,
                           &s_servo_angle, 0, 180);

    menu_set_default_int(&g_servo_angle_st,90);
    /* float 可编辑: 演示 float 编辑 + 步进切换 */
#if UNDERSTAND_RADAR_VEL_CHANGE
    Create_Menu_Folder(&g_root,s_pid_folder_st,"pid");
    
    Create_Menu_Leaf_Float(&s_pid_folder_st, s_kp_st,
                           "kp", s_draw_kp,
                           &s_kp_f);
    menu_set_default_float(&s_kp_st,0);


    Create_Menu_Leaf_Float(&s_pid_folder_st, s_ki_st,
                           "ki", s_draw_ki,
                           &s_ki_f);
    menu_set_default_float(&s_ki_st,0);

    Create_Menu_Leaf_Float(&s_pid_folder_st, s_kd_st,
                           "kd", s_draw_kd,
                           &s_kd_f);
    menu_set_default_float(&s_kd_st,0);
    
#endif 
    
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
