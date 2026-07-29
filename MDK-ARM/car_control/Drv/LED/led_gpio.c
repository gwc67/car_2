#include "stddef.h"
#include "led_base.h"
#include "led_gpio.h"
#include "cmsis_os2.h"
// 现在的需求，开始封装delay函数，现在没有什么思路
static const uint8_t MaxBrightness_led = 100;
#define container_of(ptr, type, member) ((type*)((char*)(ptr) - offsetof(type, member)))

static void gpio_on(LedBase *base)
{
    LedGpio* me = container_of(base, LedGpio, base);
    HAL_GPIO_WritePin(me->gpio, me->pin, (GPIO_PinState)me->on_level);
}

static void gpio_off(LedBase *base)
{
    LedGpio* me = container_of(base, LedGpio, base);  // 这些函数是用来返回一开始的头地址的，始终只有操作LED_BASE
    HAL_GPIO_WritePin(me->gpio, me->pin, (GPIO_PinState)!me->on_level);
}

//用于给gpio口来设置亮度的 我说白了吧 其实只有3种颜色 绿 黄 白
static void gpio_set_brightness(LedBase *base, uint8_t val)
{
    LedGpio* me = container_of(base,LedGpio,base);
    if (val >= MaxBrightness_led)
    {
        val = MaxBrightness_led;
    }
    me->brightness = val; 
}

static uint8_t gpio_get_brightness(LedBase* base)
{
    LedGpio* me = container_of(base,LedGpio,base);
    return me->brightness;
}

static void delay_os_ms(DelayBase* me,uint32_t val)
{
    //这个delayBase不用回到开头
    osDelay(val);
}
static void delay_hal_ms(DelayBase* me,uint32_t val)
{
    //这个delayBase不用回到开头
    HAL_Delay(val);
}

static const LedOps gpio_ops = {.on = gpio_on, .off = gpio_off,.set_brightness = gpio_set_brightness,.get_brightness = gpio_get_brightness};
static const DelayOps delay_os_ops = {.ms = delay_os_ms,};  // .us =NULL
static const DelayOps delay_hal_ops = {.ms = delay_hal_ms, };
    
static void led_base_init(LedBase* me, const char *name)
{
    me->name = name;
    me->state = 0;
}

void led_gpio_init(LedGpio *me, const char *name, GPIO_TypeDef* gpio, uint16_t pin, uint8_t on_level)
{
    led_base_init(&me->base, name);

    me->gpio = gpio;
    me->pin = pin;
    me->on_level = on_level;
    me->base.ops = &gpio_ops;
}

void delay_os_init(stDelay* me)
{
    me->base.ops = &delay_os_ops; //给虚函数表绑定操作函数
}
void delay_hal_init(stDelay* me)
{
    me->base.ops = &delay_hal_ops;
}
