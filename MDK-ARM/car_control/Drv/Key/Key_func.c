#include "Key_func.h"
#include "driver_registry.h"


#define KEYFUNC_DOUBLE_MS 300       //双击
#define KEYFUNC_LONG_MS   2000      //长按阈值
#define KEYFUNC_REPEAT_MS  200      //长按自动连发

//手势状态机
enum kf_state {
    ST_IDLE_em,
    ST_WAIT_LONG_em,               //等待长按
    ST_WAIT_DOUBLE_em,             //等待双击，否则就是一直REPEAT
    ST_REPEAT_em,
};


struct kf_runtime_t {
    enum kf_state state_em;
    uint32_t      tick_ms_ul;    //进入当前状态的时间搓
    bool          pressed_prev_b;//上一次扫描的按下状态 
};

static struct kf_runtime_t s_kf_pst[KEY_NUM_em];
static LIST_HEAD(s_sub_list);   /* 订阅者链表头 */

//这个函数会执行操作下对应事件下的所有函数
static void s_emit(enum Key_Id_e key_em, enum KeyFunc_Event_e ev_em)
{
    struct keyfunc_subscriber_t *pos;
    list_for_each_entry(pos, &s_sub_list, list, struct keyfunc_subscriber_t) {
        if (pos->callback_pst)
            pos->callback_pst(key_em, ev_em);
    }
}

void keyfunc_subscriber_add_v(struct keyfunc_subscriber_t *sub_pst)
{
    list_add_tail(&sub_pst->list, &s_sub_list);
}


void keyfunc_scan_v(uint32_t now_ms_ul)
{
    struct Key_State_t keys_st[KEY_NUM_em];
    key_state_copy_v(keys_st, KEY_NUM_em);

    for (int i = 0; i < KEY_NUM_em; i++)
    {
        struct kf_runtime_t *kf_pst = &s_kf_pst[i];
        bool pressed_b = keys_st[i].pressed_b;                        //当前按键按下的状态 如果没有它，程序只知道“现在按没按”，无法区分“一直按着”和“刚刚按下”。
        bool just_pressed_b = pressed_b && !kf_pst->pressed_prev_b;   //just_pressed指上升沿
        bool just_released_b = !pressed_b && kf_pst->pressed_prev_b;  //just_released 指下降沿 pressed 按键硬件没有按下，再检测kf_pst 下的前面是否按下
        kf_pst->pressed_prev_b = pressed_b;

        //开始默认状态都是0,会自动触发空闲
        switch (kf_pst->state_em)
        {
        case ST_IDLE_em:
            if (just_pressed_b)         
            {
                kf_pst->state_em = ST_WAIT_LONG_em;
                kf_pst->tick_ms_ul = now_ms_ul;
            }
            break;
        // 等待是否长按
        case ST_WAIT_LONG_em:
            if (just_released_b)
            {
                // 短按 , 进入双击窗口
                kf_pst->state_em = ST_WAIT_DOUBLE_em;
                kf_pst->tick_ms_ul = now_ms_ul;
            }
            // 处于一直按下，且按下时间 大于 double 双击时间
            else if (pressed_b && now_ms_ul - kf_pst->tick_ms_ul >= KEYFUNC_LONG_MS)
            {
                // 长按确认 ，进入重复窗口 //会先有一次长按的状态，才会进入repeat间隔
                s_emit((enum Key_Id_e)i, KEYFUNC_LONG_em);
                kf_pst->state_em = ST_REPEAT_em;
                kf_pst->tick_ms_ul = now_ms_ul;
            }
            break;
        //真正判断按键事件的操作
        case ST_WAIT_DOUBLE_em: {
            if (just_pressed_b)
            {
                s_emit((enum Key_Id_e)i, KEYFUNC_DOUBLE_em);
                kf_pst->state_em = ST_IDLE_em;
            }
            else if (now_ms_ul - kf_pst->tick_ms_ul >= KEYFUNC_DOUBLE_MS)
            {
                // 双击窗口超时 -> 确认为单击
                s_emit((enum Key_Id_e)i, KEYFUNC_SINGLE_em);
                kf_pst->state_em = ST_IDLE_em;
            }
            break;
        }
        case ST_REPEAT_em:
            if (just_released_b)
            {
                kf_pst->state_em = ST_IDLE_em;
            }
            else if (pressed_b && now_ms_ul - kf_pst->tick_ms_ul >= KEYFUNC_REPEAT_MS)
            {
                s_emit((enum Key_Id_e)i, KEYFUNC_REPEAT_em);
                kf_pst->tick_ms_ul = now_ms_ul;
            }
            break;
        }
    }
}

/* 默认订阅者 (LED + 舵机 + 飞控) */


//default 下的按键回调
static void my_key_handler(enum Key_Id_e key_em,enum KeyFunc_Event_e ev_em)
{
#if PID_RESET
    if (key_em == KEY_1_em)
    {
        if (ev_em == KEYFUNC_LONG_em)
        {

            pid_reset_v(pid_get_loc_xyz(X_em));
            pid_reset_v(pid_get_loc_xyz(Y_em));
            PID_Init();
            fly_task_reset_v();
            led_start_blinking_color(LED_COLOR_RED,100,5,LED_PRIORITY_NORMAL);
        }
    }
#endif
    // else if (key_em == KEY_2_em)
    // {
    //     if (ev_em == KEYFUNC_SINGLE_em)
    //     {
    //         // led_start_blinking_color(LED_COLOR_GREEN,100,5,LED_PRIORITY_NORMAL);
    //     }
    //     else if (ev_em == KEYFUNC_DOUBLE_em)
    //     {
    //         // led_start_blinking_color(LED_COLOR_ORANGE,100,5,LED_PRIORITY_NORMAL);
    //     }
    // }
    
}
static struct keyfunc_subscriber_t s_default_sub = {
    .callback_pst = my_key_handler,
};

/* 兼容旧 API — 内部转发到 subscriber 链表 */
void keyfunc_event_register_v(keyfunc_callback_t callback_pst)
{
    s_default_sub.callback_pst = callback_pst;
}
void keyfunc_init_v(void)
{
    for (int i = 0; i < KEY_NUM_em; i++)
    {
        s_kf_pst[i].state_em  = ST_IDLE_em;
        s_kf_pst[i].pressed_prev_b = false;
    }
    /* 注册默认订阅者到链表 */
    INIT_LIST_HEAD(&s_default_sub.list);
    list_add_tail(&s_default_sub.list, &s_sub_list);
}


DRIVER_INIT(keyfunc_init_v);
