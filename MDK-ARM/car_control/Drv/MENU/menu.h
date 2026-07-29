#ifndef __MENU_H
#define __MENU_H

#include "main.h"
#include "Key_func.h"

#define USE_OLED_MENU 1

#if USE_OLED_MENU

/*
 * 树形菜单系统 — Linux menuconfig 风格
 *
 * 功能:
 *   - 文件夹/叶子树结构, 框架自动显示子项列表 + 光标
 *   - 滚动列表 (超过 MENU_VISIBLE_ROWS 项自动滚动)
 *   - int32 / float 可编辑叶子
 *   - 步进值循环切换 (编辑时双击 ENTER 切换 step)
 *
 * 操作:
 *   单击 KEY1/KEY2 — UP/DOWN 导航或编辑 ±step
 *   单击 KEY3      — ENTER 进入子项 / 切换编辑模式
 *   双击 KEY3      — 编辑模式下循环切换步进值
 *   单击 KEY4      — BACK 退出编辑 / 返回上级
 */

/* 显示行数 (64px 屏: 16px 标题 + 3×14px 子项) */
#define MENU_VISIBLE_ROWS  3

/* 步进值表 — 双击 ENTER 在这几个值之间循环 */
#define MENU_STEP_COUNT    5
extern const float s_menu_steps[MENU_STEP_COUNT];

/* 节点类型 */
enum menu_kind_e {
    MENU_FOLDER,
    MENU_LEAF,
};

/* 数据类型 */
enum menu_data_type_e {
    DATA_INT,       /* data 指向 int32_t */
    DATA_FLOAT,     /* data_f 指向 float */
};

struct menu_base_t {
    const char* name;
};
/* 菜单节点 */
struct menu_node_t {
    struct menu_base_t base;
    enum menu_kind_e kind;
    void (*draw)(struct menu_node_t *self);

    /* 树结构 */
    struct menu_node_t *parent;
    struct menu_node_t *first_child;
    struct menu_node_t *next;

    uint8_t cursor;        /* 当前高亮子项索引 */
    uint8_t view_offset;   /* 滚动偏移 (第一行显示哪个子项) */

    /* 可编辑数据 */
    int32_t *data;                /* int 模式: 指向 int32_t 变量 */
    float   *data_f;              /* float 模式: 指向 float 变量 */
    enum menu_data_type_e data_type;
    uint8_t step_idx;             /* 当前步进索引 (循环切换) */
    int32_t data_min;             /* 下限 */
    int32_t data_max;             /* 上限 */
    int32_t data_default;         /* int 默认值 (长按 ENTER 恢复) */
    float   data_f_default;       /* float 默认值 (长按 ENTER 恢复) */

    struct list_head list;
};

/* container_of 下转型: menu_base_t* → menu_node_t*
 * 等价于 Linux 内核的 to_my_device(dev) 模式 */
#define to_menu_node(base_ptr) \
    container_of((base_ptr), struct menu_node_t, base)

/* ================================================================
 * 构建 API
 * ================================================================ */

struct menu_base_t *menu_create_item(struct menu_node_t *parent,
                                     struct menu_node_t *me,
                                     const char *name,
                                     enum menu_kind_e kind,
                                     void (*draw)(struct menu_node_t *));

/* 注意: me 传结构体本身 (不是指针), 宏内部取地址 */
#define Create_Menu_Folder(parent, me, name) \
    menu_create_item((parent), &(me), (name), MENU_FOLDER, NULL)

#define Create_Menu_Leaf(parent, me, name, draw_fn) \
    menu_create_item((parent), &(me), (name), MENU_LEAF, (draw_fn))

/* int32 可编辑叶子 */
#define Create_Menu_Leaf_Int(parent, me, name, draw_fn, ptr) \
    do { \
        menu_create_item((parent), &(me), (name), MENU_LEAF, (draw_fn)); \
        (me).data = (ptr); \
        (me).data_type = DATA_INT; \
    } while (0)

/* int32 可编辑叶子 + 范围限制 (步进通过双击 ENTER 循环切换) */
#define Create_Menu_Leaf_Range(parent, me, name, draw_fn, ptr, min_v, max_v) \
    do { \
        menu_create_item((parent), &(me), (name), MENU_LEAF, (draw_fn)); \
        (me).data = (ptr); \
        (me).data_type = DATA_INT; \
        (me).data_min = (min_v); \
        (me).data_max = (max_v); \
    } while (0)

/* float 可编辑叶子 */
#define Create_Menu_Leaf_Float(parent, me, name, draw_fn, ptr) \
    do { \
        menu_create_item((parent), &(me), (name), MENU_LEAF, (draw_fn)); \
        (me).data_f = (ptr); \
        (me).data_type = DATA_FLOAT; \
    } while (0)

/* float 可编辑叶子 + 范围限制 */
#define Create_Menu_Leaf_Float_Range(parent, me, name, draw_fn, ptr, min_v, max_v) \
    do { \
        menu_create_item((parent), &(me), (name), MENU_LEAF, (draw_fn)); \
        (me).data_f = (ptr); \
        (me).data_type = DATA_FLOAT; \
        (me).data_min = (min_v); \
        (me).data_max = (max_v); \
    } while (0)

/* ================================================================
 * 框架 API
 * ================================================================ */

void menu_init_v(void);
void menu_navigate_v(struct menu_node_t *target);
void menu_task_v(void);
struct menu_node_t *menu_current_get_pst(void);
bool menu_leaf_is_editing(void);

/* 设置默认值 (长按 ENTER 恢复到此值) */
static inline void menu_set_default_int(struct menu_node_t *n, int32_t val)
{
    n->data_default = val;
}

static inline void menu_set_default_float(struct menu_node_t *n, float val)
{
    n->data_f_default = val;
}

/* 恢复默认值 (长按 ENTER 触发) */
void menu_reset_to_default(struct menu_node_t *n);

/* 强制下一帧刷新（用于周期性任务，如 5Hz 数据更新） */
void menu_force_refresh(void);

/* 外部数据源请求刷新: 只有当 node 正在显示时才触发重绘
 * 用法: 在数据更新后调用 menu_request_refresh(&g_bat_st);
 *       不同数据源传不同 node, 互不干扰                       */
void menu_request_refresh(struct menu_base_t *base);

#endif


#endif /* __MENU_H */
