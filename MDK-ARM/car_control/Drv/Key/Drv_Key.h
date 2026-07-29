#ifndef __DRV_KEY_H
#define __DRV_KEY_H

#include "main.h"

enum Key_Id_e{
    KEY_1_em = 0,
    KEY_2_em,
    KEY_3_em,
    KEY_4_em,
    KEY_NUM_em,
};




enum Key_Event_e
{
    KEY_EVENT_NONE_em = 0,
    KEY_EVENT_PRESSED_em,       //按下
    KEY_EVENT_RELEASED_em,      //释放
};

struct Key_State_t {
    bool pressed_b ;
    bool just_pressed_b;
    bool just_released_b;
};

//事件回调
// typedef void (*key_event_callback_t)(enum Key_Id_e key_id_em,enum Key_Event_e event_em);

void key_scan_v(uint32_t now_ms_ul);                        //周期调用
void key_state_copy_v(struct Key_State_t *dst,uint8_t count);
// void key_event_register_v(key_event_callback_t call_back);    //注册回调

#endif
