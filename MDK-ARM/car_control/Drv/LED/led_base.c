#include "led_base.h"


//这里是写虚函数操作的 ,本质也是给app.c调用的
//base层绑定具体函数操作
void delay_ms(DelayBase* me,uint32_t val)
{
    assert_param(me->ops->ms); // 断言
    me->ops->ms(me,val);
}

void delay_us(DelayBase* me,uint32_t val)
{
    assert_param(me->ops->us);
    me->ops->us(me,val);
}

void led_on(LedBase* me)
{
    assert_param(me->ops->on);
    me->ops->on(me);
}

void led_off(LedBase* me)
{
    assert_param(me->ops->off);
    me->ops->off(me);
}

uint8_t led_get_brightness(LedBase *me)
{
    if (me->ops->get_brightness)
    {
        return me->ops->get_brightness(me);
    }
    return 0 ;
}

void led_set_brightness(LedBase* me,uint8_t val)
{
    if (me->ops->set_brightness)
    {   
        me->ops->set_brightness(me,val);
    }
    
}

