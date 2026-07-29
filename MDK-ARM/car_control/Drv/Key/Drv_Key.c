#include "Drv_Key.h"
#include "driver_registry.h"
#include "string.h"

#define KEY_DEBOUNCE_MS 20


//   你当前只有 GPIO 按键 → 不需要 vtable，数据驱动就够了。

//   如果以后要加 ADC 按键矩阵、I2C 键盘芯片等 → 加 vtable 抽象层。

//   这和 Linux 的选择一致：gpio_keys 驱动本身没有 vtable，但它注册到 input_dev（有 vtable）。当你只有一种实现时，vtable 是过度设计；当有多种实现时，vtable 是必要的抽象。

struct key_hw_desc{
    GPIO_TypeDef *port_pst;
    uint16_t      pin_us;
    bool          active_high;
};

static const struct key_hw_desc s_key_hw_table[KEY_NUM_em] = {
    [KEY_1_em]   = {.port_pst = Key1_GPIO_Port,.pin_us = Key1_Pin,.active_high = false},
    [KEY_2_em]   = {.port_pst = Key2_GPIO_Port,.pin_us = Key2_Pin,.active_high = false},  
    [KEY_3_em]   = {.port_pst = Key3_GPIO_Port,.pin_us = Key3_Pin,.active_high = false}, 
    [KEY_4_em]   = {.port_pst = Key4_GPIO_Port,.pin_us = Key4_Pin,.active_high = false},
};

struct key_runtime {
    bool raw_prev_b;            //上次原始读值
    bool stable_b;              //消抖后的稳定状态
    uint32_t edge_tick_ms_ul;   //最近一次引脚跳变时刻
    bool edge_reported;         //当前边沿是否已经上报
};


static struct key_runtime s_keys_rt[KEY_NUM_em];
static struct Key_State_t s_key_states[KEY_NUM_em];
// static key_event_callback_t s_event_cb;

static inline bool s_key_read_pin_b(const struct key_hw_desc* hw_pst)
{
    GPIO_PinState raw = HAL_GPIO_ReadPin(hw_pst->port_pst,hw_pst->pin_us);
    return hw_pst->active_high? (raw == GPIO_PIN_SET) : (raw == GPIO_PIN_RESET);   //KEY_UP 取 GPIO_PIN_RESET
}


// 引脚跳变 后 才进行消抖吗？ 怎么引脚不跳动的时候也要消抖？
/* 
   *
   *  状态机:
   *    IDLE → 引脚跳变 → WAIT_DEBOUNCE → 稳定 → 上报边沿 → IDLE
   *
   *  调用者保证 now_ms 单调递增 (HAL_GetTick)
   * ══════════════════════════════════════════════════════════ */ 
//  引脚稳定时，两个 continue 都不触发，代码直接穿透到状态更新。 没有在"消抖"，只是在走流程。
void key_scan_v(uint32_t now_ms_ul)
{
    for (int i = 0; i < KEY_NUM_em; i++)
    {
        const struct key_hw_desc *hw_pst = &s_key_hw_table[i];   //获取按键所有信息
        struct key_runtime       *rt_pst = &s_keys_rt[i];
        struct Key_State_t       *st_pst = &s_key_states[i];

        //raw_b 始终是跳动值
        bool raw_b = s_key_read_pin_b(hw_pst);

        //引脚跳变 -> 重启消抖倒计时 如果不跳动，这部分都不执行 // 阶段①：raw == raw_prev → false → 跳过，不重置计时器
        if (raw_b != rt_pst->raw_prev_b)
        {
            rt_pst->raw_prev_b = raw_b;
            rt_pst->edge_tick_ms_ul = now_ms_ul;
            rt_pst->edge_reported = false;

            st_pst->just_pressed_b = false;
            st_pst->just_released_b = false;
            continue;                                           //本轮到此为止，等下次循环再判断是否稳定        
        }

        //消抖未完成 等待   // 阶段②：now - edge_tick = 5000ms，5000 < 20 → false → 跳过，不等待
        if (now_ms_ul - rt_pst->edge_tick_ms_ul < KEY_DEBOUNCE_MS)
        {
            continue;
        }

        //消抖完成 锁定稳定状态
        bool prev_stable_b = rt_pst->stable_b;
        rt_pst->stable_b = raw_b;
        st_pst->pressed_b = rt_pst->stable_b;

        //边沿上报 （每个边沿只报一次） 运行的时候 没有上报 并且 这次稳定后的状态和上次稳定后状态不一样的时候
        if (!rt_pst->edge_reported && rt_pst->stable_b != prev_stable_b)
        {
            rt_pst->edge_reported = true;
            
            //根据运行时候的stable_b 的状态  1：按下 0: 松开
            if (rt_pst->stable_b)
            {
                st_pst->just_pressed_b =true;           //按下
                st_pst->just_released_b = false;
            }
            else
            {
                st_pst->just_pressed_b = false;         //松开
                st_pst->just_released_b = true;
            }

            //如果有按键回调函数的话
            // if (s_event_cb)
            // {
            //     enum Key_Event_e ev_em = rt_pst->stable_b ? KEY_EVENT_PRESSED_em :KEY_EVENT_RELEASED_em;
            //     s_event_cb((enum Key_Id_e)i,ev_em);
            // }
            
            
        }
        //稳定的时候就会执行这里，前面的if不执行
        else if (rt_pst->stable_b == prev_stable_b)
        {
            //无新边沿 清掉 just_xxxx;
            st_pst->just_pressed_b = false;
            st_pst->just_released_b = false;
        }
        
        
        
        
    }
    
}

//copy所有按键的状态
void key_state_copy_v(struct Key_State_t *dst_pst,uint8_t count_uc)
{
    uint8_t n = (count_uc < KEY_NUM_em) ? count_uc : KEY_NUM_em;
    memcpy(dst_pst,s_key_states,n*sizeof(struct Key_State_t));
}

// void key_event_register_v(key_event_callback_t callback_pst)
// {
//     s_event_cb = callback_pst;
// }

static void s_key_init_v(void)
{
    memset(s_keys_rt,0,sizeof(s_keys_rt));
    memset(s_key_states,0,sizeof(s_key_states));
    // s_event_cb = NULL;
    uint32_t now_ul = HAL_GetTick();
    for (int i = 0; i < KEY_NUM_em; i++)
    {
        s_keys_rt[i].raw_prev_b = s_key_read_pin_b(&s_key_hw_table[i]);
        s_keys_rt[i].stable_b = s_keys_rt[i].raw_prev_b;
        s_keys_rt[i].edge_tick_ms_ul = now_ul;
    }
}

DRIVER_INIT(s_key_init_v);

