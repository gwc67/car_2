/*
 * 树形菜单系统 — 核心实现
 *
 * 功能:
 *   1. 树导航: UP/DOWN 循环, ENTER 进入, BACK 返回 (带光标恢复)
 *   2. 滚动列表: 超过 MENU_VISIBLE_ROWS 项时自动滚动
 *   3. 编辑模式: ENTER 进入 → UP/DOWN 改值 → 双击 ENTER 切步进 → BACK 退出
 *   4. int32 / float 双类型支持
 */

#include "menu.h"
#include "OLED.h"
#include "stdio.h"
#if USE_OLED_MENU

/* 步进值表: 0.1, 1, 10, 100 — 双击 ENTER 循环 */
const float s_menu_steps[MENU_STEP_COUNT] = { 0.01,0.1f,1.0f, 10.0f, 100.0f};

static struct menu_node_t *s_current_pst = NULL;
static volatile bool s_dirty_b = false;
static struct keyfunc_subscriber_t s_menu_sub;
static struct menu_node_t *s_nav_stack[8];
static int s_nav_top = -1;
static bool s_editing = false;

/* 按键映射 */
#define KEY_MENU_UP     KEY_1_em
#define KEY_MENU_DOWN   KEY_2_em
#define KEY_MENU_ENTER  KEY_3_em
#define KEY_MENU_BACK   KEY_4_em

/* 前向声明 */
static void s_menu_folder_draw(struct menu_node_t *node);

/* ================================================================
 * 辅助函数
 * ================================================================ */

/* 当前节点是否有可编辑数据 (int 或 float) */
static bool s_has_data(struct menu_node_t *n)
{
    return (n->data_type == DATA_INT && n->data != NULL)
        || (n->data_type == DATA_FLOAT && n->data_f != NULL);
}

/* 获取当前步进值 */
static float s_get_step(struct menu_node_t *n)
{
    return s_menu_steps[n->step_idx % MENU_STEP_COUNT];
}

/* 增加值 (UP) */
static void s_adjust_plus(struct menu_node_t *n)
{
    float step = s_get_step(n);
    if (n->data_type == DATA_INT && n->data) {
        int32_t v = (int32_t)step;
        if (v < 1) v = 1;  /* int 模式最小步进 1 */
        *n->data += v;
        if (n->data_max != 0 && *n->data > n->data_max)
            *n->data = n->data_max;
    } else if (n->data_type == DATA_FLOAT && n->data_f) {
        *n->data_f += step;
        if (n->data_max != 0 && *n->data_f > (float)n->data_max)
            *n->data_f = (float)n->data_max;
    }
}

/* 减小值 (DOWN) */
static void s_adjust_minus(struct menu_node_t *n)
{
    float step = s_get_step(n);
    if (n->data_type == DATA_INT && n->data) {
        int32_t v = (int32_t)step;
        if (v < 1) v = 1;
        *n->data -= v;
        if (n->data_min != 0 && *n->data < n->data_min)
            *n->data = n->data_min;
    } else if (n->data_type == DATA_FLOAT && n->data_f) {
        *n->data_f -= step;
        if (n->data_min != 0 && *n->data_f < (float)n->data_min)
            *n->data_f = (float)n->data_min;
    }
}

/* 格式化值为字符串 (int 或 float) */
static void s_format_value(struct menu_node_t *n, char *buf, uint8_t bufsize)
{
    if (n->data_type == DATA_INT && n->data) {
        snprintf(buf, bufsize, "%d", (int)*n->data);
    } else if (n->data_type == DATA_FLOAT && n->data_f) {
        float v = *n->data_f;
        if (v == (int)v)
            snprintf(buf, bufsize, "%d", (int)v);
        else
            snprintf(buf, bufsize, "%.2f", (double)v);
    } else {
        buf[0] = '-'; buf[1] = '-'; buf[2] = '\0';
    }
}

/* 子节点计数 */
static uint8_t s_count_children(struct menu_node_t *node)
{
    uint8_t n = 0;
    struct menu_node_t *c = node->first_child;
    while (c) { n++; c = c->next; }
    return n;
}

/* ================================================================
 * 树构建
 * ================================================================ */

struct menu_base_t *menu_create_item(struct menu_node_t *parent,
                                     struct menu_node_t *me,
                                     const char *name,
                                     enum menu_kind_e kind,
                                     void (*draw)(struct menu_node_t *))
{
    me->base.name = name;
    me->kind = kind;
    me->draw = draw;
    me->parent = parent;
    me->first_child = NULL;
    me->next = NULL;
    me->cursor = 0;
    me->view_offset = 0;

    /* 可编辑数据默认值 */
    me->data = NULL;
    me->data_f = NULL;
    me->data_type = DATA_INT;
    me->step_idx = 1;           /* 默认步进 s_menu_steps[1] = 1.0 */
    me->data_min = 0;
    me->data_max = 0;

    INIT_LIST_HEAD(&me->list);

    /* 挂到 parent 子链表末尾 */
    if (parent) {
        if (!parent->first_child) {
            parent->first_child = me;
        } else {
            struct menu_node_t *p = parent->first_child;
            while (p->next) p = p->next;
            p->next = me;
        }
    }

    /* 第一个创建的节点作为首页 */
    if (!s_current_pst)
        s_current_pst = me;

    return &me->base;
}

/* ================================================================
 * 按键事件处理
 * ================================================================ */

static void s_menu_key_handler(enum Key_Id_e key_em, enum KeyFunc_Event_e ev_em)
{
    if (!s_current_pst) return;

    struct menu_node_t *target;

    /* ---- 双击: 编辑模式下切换步进值 ---- */
    if (ev_em == KEYFUNC_DOUBLE_em) {
        if (key_em == KEY_MENU_ENTER && s_editing && s_has_data(s_current_pst)) {
            s_current_pst->step_idx =
                (s_current_pst->step_idx + 1) % MENU_STEP_COUNT;
            s_dirty_b = true;
        }
        return;
    }
    else if (ev_em == KEYFUNC_LONG_em)
    {
        /* 长按 ENTER: 恢复当前叶子的默认值 */
        if (key_em == KEY_MENU_ENTER && s_has_data(s_current_pst)) {
            menu_reset_to_default(s_current_pst);
            s_dirty_b = true;
        }
    }
    

    /* 只处理单击 */
    if (ev_em != KEYFUNC_SINGLE_em) return;

    switch (key_em) {

    case KEY_MENU_DOWN:
        if (s_editing && s_has_data(s_current_pst)) {
            s_adjust_minus(s_current_pst);
            s_dirty_b = true;
        } else if (s_current_pst->first_child) {
            uint8_t count = s_count_children(s_current_pst);
            if (count > 0) {
                s_current_pst->cursor = (s_current_pst->cursor + 1) % count;
                /* 滚动: 光标超出可视区域下界 */
                if (s_current_pst->cursor >= s_current_pst->view_offset + MENU_VISIBLE_ROWS)
                    s_current_pst->view_offset = s_current_pst->cursor - MENU_VISIBLE_ROWS + 1;
                /* wrap-around 时也可能超出上界, 再检查一次 */
                if (s_current_pst->cursor < s_current_pst->view_offset)
                    s_current_pst->view_offset = s_current_pst->cursor;
                s_dirty_b = true;
            }
        }
        break;

    case KEY_MENU_UP:
        if (s_editing && s_has_data(s_current_pst)) {
            s_adjust_plus(s_current_pst);
            s_dirty_b = true;
        } else if (s_current_pst->first_child) {
            uint8_t count = s_count_children(s_current_pst);
            if (count > 0) {
                s_current_pst->cursor = (s_current_pst->cursor == 0)
                    ? count - 1
                    : s_current_pst->cursor - 1;
                /* 滚动: 光标超出可视区域上界 */
                if (s_current_pst->cursor < s_current_pst->view_offset)
                    s_current_pst->view_offset = s_current_pst->cursor;
                /* wrap-around 时也可能超出下界, 再检查一次 */
                if (s_current_pst->cursor >= s_current_pst->view_offset + MENU_VISIBLE_ROWS)
                    s_current_pst->view_offset = s_current_pst->cursor - MENU_VISIBLE_ROWS + 1;
                s_dirty_b = true;
            }
        }
        break;

    case KEY_MENU_ENTER:
        if (s_current_pst->first_child) {
            /* 文件夹: 进入高亮子项 */
            target = s_current_pst->first_child;
            {
                uint8_t i;
                for (i = 0; i < s_current_pst->cursor && target; i++)
                    target = target->next;
            }
            if (target) {
                if (s_nav_top < 7)
                    s_nav_stack[++s_nav_top] = s_current_pst;
                s_current_pst = target;
                s_editing = false;
                s_dirty_b = true;
            }
        } else if (s_has_data(s_current_pst)) {
            /* 叶子有数据: 切换编辑模式 */
            s_editing = !s_editing;
            s_dirty_b = true;
        }
        break;

    case KEY_MENU_BACK:
        if (s_editing) {
            s_editing = false;
            s_dirty_b = true;
        } else if (s_nav_top >= 0) {
            s_current_pst = s_nav_stack[s_nav_top--];
            s_dirty_b = true;
        } else if (s_current_pst->parent) {
            s_current_pst = s_current_pst->parent;
            s_dirty_b = true;
        }
        break;

    default:
        break;
    }
}

/* ================================================================
 * 初始化 + API
 * ================================================================ */

void menu_init_v(void)
{
    INIT_LIST_HEAD(&s_menu_sub.list);
    s_menu_sub.callback_pst = s_menu_key_handler;
    keyfunc_subscriber_add_v(&s_menu_sub);
    s_dirty_b = true;
}

void menu_navigate_v(struct menu_node_t *target)
{
    if (!target) return;
    s_current_pst = target;
    s_dirty_b = true;
}

struct menu_node_t *menu_current_get_pst(void)
{
    return s_current_pst;
}

bool menu_leaf_is_editing(void)
{
    return s_editing;
}

void menu_reset_to_default(struct menu_node_t *n)
{
    if (!n) return;
    if (n->data_type == DATA_INT && n->data) {
        *n->data = n->data_default;
    } else if (n->data_type == DATA_FLOAT && n->data_f) {
        *n->data_f = n->data_f_default;
    }
}

void menu_request_refresh(struct menu_base_t *base)
{
    /* base → node 下转型, 只有当前正在显示这个节点时才触发重绘 */
    struct menu_node_t *me = to_menu_node(base);
    if (base && s_current_pst == me)
        s_dirty_b = true;
}

/* 强制下一帧刷新：用于周期性任务缓慢更新数据页面 (如 5Hz) */
void menu_force_refresh(void)
{
    s_dirty_b = true;
}

/* ================================================================
 * 显示
 * ================================================================ */

/*
 * 文件夹通用显示 (带滚动)
 *
 * 布局:
 *   (0, 0)   标题 (8x16, 16px)
 *   (0, 20)  子项 view_offset+0  (6x8, 14px 行距)
 *   (0, 34)  子项 view_offset+1
 *   (0, 48)  子项 view_offset+2
 *
 * 超过 MENU_VISIBLE_ROWS 项时, 底部显示 ^ v 滚动指示
 */
static void s_menu_folder_draw(struct menu_node_t *node)
{
    struct menu_node_t *child;                          //定义孩子
    uint8_t i, row;                                     //定义i，row行
    uint8_t total;                                      //子菜单数量

    /* 标题 */
    OLED_ShowString(0, 0, node->base.name, OLED_8X16);       //父菜单的名字

    /* 定位到 view_offset 对应的子项 */
    child = node->first_child;                          //父菜单第一个子菜单
    for (i = 0; i < node->view_offset && child; i++)    //view_offset 决定了当前显示第几个子项
        child = child->next;

    /* 绘制可见行 */
    total = s_count_children(node);                     //子菜单数量得到
    row = 0;
    while (child && row < MENU_VISIBLE_ROWS) {
        uint8_t y = 20 + row * 14;                      //y = 20 + row * 14 行
        uint8_t item_idx = node->view_offset + row;     //偏移量 + 显示的行数 最大显示行数只有4

        /* 光标标记 */                                  
        if (item_idx == node->cursor)                   //
            OLED_ShowString(0, y, ">>", OLED_6X8);
        else
            OLED_ShowString(0, y, "  ", OLED_6X8);

        /* 类型指示 */
        if (child->first_child)                         //文件夹下的子菜单的子菜单没有first_child
            OLED_ShowString(12, y, "[+]", OLED_6X8);    /* 文件夹 */
        else if (s_has_data(child))
            OLED_ShowString(12, y, "[#]", OLED_6X8);    /* 可编辑叶子 */
        else
            OLED_ShowString(12, y, " > ", OLED_6X8);    /* 普通叶子 */

        OLED_ShowString(30, y, child->base.name, OLED_6X8);

        child = child->next;
        row++;
    }

    /* 滚动指示器 */
    if (total > MENU_VISIBLE_ROWS) {
        if (node->view_offset > 0)
            OLED_ShowChar(120, 20, '^', OLED_6X8);
        if (node->view_offset + MENU_VISIBLE_ROWS < total)
            OLED_ShowChar(120, 48, 'v', OLED_6X8);
    }
}

void menu_task_v(void)
{
    if (!s_dirty_b || !s_current_pst) return;
    s_dirty_b = false;

    OLED_Clear();

    if (s_current_pst->first_child) {
        s_menu_folder_draw(s_current_pst);
    } else {
        if (s_current_pst->draw)
            s_current_pst->draw(s_current_pst);
        else
            OLED_ShowString(0, 24, s_current_pst->base.name, OLED_8X16);

        /* 有绑定数据 → 底部显示值 + 步进 + 模式 */
        if (s_has_data(s_current_pst)) {
            char buf[20];
            s_format_value(s_current_pst, buf, sizeof(buf));
            OLED_ShowString(0, 48, buf, OLED_8X16);

            if (s_editing) {
                char step_buf[10];
                float step = s_get_step(s_current_pst);
                if (step == (int)step)
                    snprintf(step_buf, sizeof(step_buf), "s:%d", (int)step);
                else
                    snprintf(step_buf, sizeof(step_buf), "s:%.2f", (double)step);
                OLED_ShowString(60, 48, step_buf, OLED_6X8);
                OLED_ShowString(100, 48, "EDIT", OLED_6X8);
            } else {
                OLED_ShowString(72, 48, "[ENT]", OLED_6X8);
            }
        }
    }

    OLED_Update();
}


#endif
