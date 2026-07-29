# Drv_LED — C 虚函数表（vtable）驱动架构

## 概述

用 C 语言模拟面向对象多态，通过操作表（vtable）实现 LED 驱动的硬件无关化。
同一套 `led_on()` / `led_off()` 接口可操作 GPIO LED、PWM LED、I2C LED，**行为由初始化时绑定的 vtable 决定**。

同时扩展了 Delay 抽象层，让 `delay_ms()` 也可通过 vtable 分发（指向 `osDelay` 或其他实现）。

## 四层架构

```
App 层          app.c
                 ↕ LedBase* / DelayBase* 句柄
Board 层        board_init.c, leds.h
                 ↕ 硬件绑定
Subclass 层     led_gpio.c/h, led_pwm.c, led_i2c.c
                 ↕ container_of 向下转型
Base 层         led_base.c/h
```

---

### Base 层 — `led_base.h / led_base.c`

定义基类和操作表。

```c
struct LedBase {
    const char* name;          // LED 名称
    int state;                 // 状态
    int brightness;            // 当前亮度值（软件 PWM 用）
    const LedOps *ops;         // vtable 指针
};

typedef struct {
    void (* on)(LedBase *me);                            // 必须实现
    void (* off)(LedBase *me);                           // 必须实现
    void (* set_brightness)(LedBase *me, int8_t val);    // 可选（NULL 判空跳过）
} LedOps;
```

Delay 基类：

```c
struct DelayBase {
    const DelayOps* ops;
};

typedef struct {
    void (* ms)(DelayBase* me, uint32_t val);   // 必须
    void (* us)(DelayBase* me, uint32_t val);   // 可选
} DelayOps;
```

**分发函数：**

```c
void led_on(LedBase* me) {
    assert_param(me->ops->on);
    me->ops->on(me);
}

void led_set_brightness(LedBase* me, int8_t val) {
    if (me->ops->set_brightness)          // NULL 判空
        me->ops->set_brightness(me, val);
}

void delay_ms(DelayBase* me, uint32_t val) {
    assert_param(me->ops->ms);
    me->ops->ms(me, val);                 // 注意: ops 是指针，用 ->
}
```

**⚠️ 常见错误**: `ops` 是**指针**，访问成员必须用 `->`（不是 `.`），字段名全小写（不是 `Ops`）。

---

### Subclass 层 — `led_gpio.c/h`

继承 `LedBase`，扩展私有数据。

```c
typedef struct {
    LedBase base;           // 基类 — 必须是第一个成员
    uint16_t pin;
    GPIO_TypeDef* gpio;
    uint8_t on_level;
} LedGpio;

typedef struct {
    DelayBase base;
} Delay_os;                  // Delay 子类（无需额外数据）
```

实现方式：

```c
static void gpio_on(LedBase *base)         // 参数必须用 LedBase*
{
    LedGpio* me = container_of(base, LedGpio, base);
    HAL_GPIO_WritePin(me->gpio, me->pin, (GPIO_PinState)me->on_level);
}

static void gpio_set_brightness(LedBase *base, int8_t val)
{
    base->brightness = val;   // GPIO 做不了硬件调光，只存值
}

static const LedOps gpio_ops = {
    .on = gpio_on,
    .off = gpio_off,
    .set_brightness = gpio_set_brightness,
};

static void delay_ms(DelayBase* me, uint32_t val)
{
    osDelay(val);             // delay 子类实现
}

static const DelayOps delay_os_ops = {.ms = delay_ms};

void led_gpio_init(LedGpio *me, ...)
{
    me->base.ops = &gpio_ops;          // 绑定 vtable
}

void delay_os_init(Delay_os* me)
{
    me->base.ops = &delay_os_ops;
}
```

---

### Board 层 — `leds.h / board_init.c`

唯一知道具体硬件的地方。创建静态实例并暴露基类指针。

```c
// leds.h
extern LedBase *g_led_red;
extern LedBase *g_led_green;
extern LedBase *g_led_blue;
extern LedBase *g_led_ano;
extern DelayBase *delay;

// board_init.c
static LedGpio gpio_red;
static Delay_os delay_os;

void board_init(void)
{
    led_gpio_init(&gpio_red, "red", LED_R_GPIO_Port, LED_R_Pin, 1);
    g_led_red = &gpio_red.base;          // 向上转型
    delay_os_init(&delay_os);
    delay = &delay_os.base;
}
```

---

### App 层 — `app.c`

只认 `LedBase*` / `DelayBase*` 句柄，不依赖具体硬件。

```c
void red_blink(void)
{
    led_on(g_led_red);
    delay_ms(delay, 200);      // vtable → osDelay
    led_off(g_led_red);
}

void LED_1ms_DRV(void)
{
    LedBase* Leds[LED_NUM] = {g_led_red, g_led_green, g_led_blue, g_led_ano};
    static u16 led_cnt[LED_NUM];

    for (u8 i = 0; i < LED_NUM; i++) {
        if (led_cnt[i] < Leds[i]->brightness)
            led_on(Leds[i]);           // vtable 分发
        else
            led_off(Leds[i]);
        if (++led_cnt[i] >= 20)
            led_cnt[i] = 0;
    }
    // 循环内直接 led_on/off，不用位掩码 + LED_On_Off()
}
```

## 调用链路

```
app.c: led_on(g_led_red)
  → led_base.c: me->ops->on(me)                // vtable 跳转
    → led_gpio.c: gpio_on(LedBase *base)
      → container_of(base, LedGpio, base)       // 算出 LedGpio 地址
        → HAL_GPIO_WritePin(gpio, pin, level)
```

```
app.c: delay_ms(delay, 200)
  → led_base.c: me->ops->ms(me, 200)            // vtable 跳转
    → led_gpio.c: delay_ms(DelayBase*, 200)
      → osDelay(200)
```

## container_of 宏

```c
#define container_of(ptr, type, member) \
    ((type*)((char*)(ptr) - offsetof(type, member)))
```

从结构体某个成员的地址反推结构体起始地址。参数名要和 `offsetof` 一致。

## 核心规则

1. **vtable 函数指针参数必须是基类指针**（`LedBase*` / `DelayBase*`），子类实现里用 `container_of` 转型。
2. **子类结构体第一个成员必须是基类**（`LedBase base` / `DelayBase base`）。
3. **可选函数做 NULL 判空** — `set_brightness` 或 `us` 可以不实现。
4. **`ops` 是指针，用 `->` 访问成员**，字段名全小写。
5. **board_init.c 是唯一知道具体硬件的地方**，更换引脚只改这一个文件。

## 文件清单

| 文件 | 层 | 职责 |
|------|----|------|
| `led_base.h` | Base | `LedBase`、`DelayBase` 结构体 + `LedOps`、`DelayOps` vtable 定义 |
| `led_base.c` | Base | `led_on/off/set_brightness`、`delay_ms/us` 分发函数 |
| `led_gpio.h` | Subclass | `LedGpio` 子类 + `Delay_os` 子类结构体 |
| `led_gpio.c` | Subclass | GPIO on/off、set_brightness、delay_ms 实现 |
| `led_pwm.c` | Subclass | (预留) PWM 子类实现 |
| `led_i2c.c` | Subclass | (预留) I2C 子类实现 |
| `leds.h` | Board | 声明全局 `LedBase*` 和 `DelayBase*` 句柄 |
| `board_init.c` | Board | 硬件绑定初始化 |
| `app.c` | App | 使用句柄的上层应用（闪烁 + 软件 PWM） |
| `app.h` | App | App 层函数声明 |
| `Drv_led.h` | (旧/已废弃) | 原项目 LED 头文件 — **全部注释掉，无有效代码** |
| `Drv_led.c` | (旧/已废弃) | 原项目 LED 驱动 — **全部注释掉，无有效代码** |

## 历史说明

- `Drv_led.h / Drv_led.c` 是本项目原来的 LED 驱动（宏 + 位操作 + `_led_st` 亮度数组），当前版本已全部注释，功能完全由新的 vtable 架构替代。
- `LED_1ms_DRV()` 已从旧文件迁移到 `app.c`，通过 `LedBase*` 操作，不再访问裸全局变量。
- App 层亮度数据存储在 `LedBase.brightness` 字段，由 IMU 的 0x0F 数据帧通过 `led_set_brightness()` 写入。

## 新增子类步骤

1. 定义新结构体（首成员为 `LedBase base`）
2. 实现 on/off 静态函数（参数 `LedBase*`，内部 `container_of`）
3. 创建 `static const LedOps` vtable
4. 暴露 init 函数绑定 ops
