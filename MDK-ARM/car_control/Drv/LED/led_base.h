#ifndef __LED_BASE_H
#define __LED_BASE_H
//.c .h 第一层父类 ledBase struct ledOps 操作表 led_on 分发实现
#include "main.h"
typedef struct LedBase LedBase;  // 前置声明

typedef struct 
{
    void (* on)(LedBase *me);   //must
    void (* off)(LedBase *me);   //must
    void (* set_brightness)(LedBase *me,uint8_t val);   //choose
    uint8_t (*get_brightness)(LedBase *me); 
}LedOps;

typedef struct DelayBase DelayBase; // 一定要有Base层
typedef struct 
{
    void (* ms)(DelayBase* me, uint32_t val);   //must
    void (* us)(DelayBase* me, uint32_t val);   //must
}DelayOps;

struct DelayBase
{
    const DelayOps* ops;  // DelayBase 需要放置操作表的指针
};



struct LedBase
{
    const char* name;
    int state;
    const LedOps *ops;
};


void led_on(LedBase* me);

void led_off(LedBase* me);
 
void led_set_brightness(LedBase* me,uint8_t val);

uint8_t led_get_brightness(LedBase *me); 


void delay_ms(DelayBase* me,uint32_t val);

void delay_us(DelayBase* me,uint32_t val);

#endif
