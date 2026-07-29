#ifndef __KEY_FUNC_H
#define __KEY_FUNC_H

#include "main.h"
#include "Drv_Key.h"

enum KeyFunc_Event_e {
    KEYFUNC_NONE_em = 0,
    KEYFUNC_SINGLE_em,      // 单击 300ms 确认
    KEYFUNC_DOUBLE_em,      // 双击 (第二次按下瞬间触发)
    KEYFUNC_LONG_em,
    KEYFUNC_REPEAT_em,
};

typedef void (*keyfunc_callback_t)(enum Key_Id_e, enum KeyFunc_Event_e ev);

/* ================================================================
 * Subscriber 链表 — 多订阅者事件分发
 * ================================================================ */

/* Linux 内核风格双向链表头 */
struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    head->prev->next = node;            //head 前一个的下面 == node ？
    node->prev = head->prev;            //node prev 下的head->prev
    node->next = head;
    head->prev = node;
}

#ifndef container_of
#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))
#endif

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/* list_for_each_entry — 需要 AC5 兼容, 不用 __typeof__ */
#define list_for_each_entry(pos, head, member, type) \
    for (pos = list_entry((head)->next, type, member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, type, member))

/* 按键事件订阅者 */
struct keyfunc_subscriber_t {
    struct list_head list;
    keyfunc_callback_t callback_pst;
};

/* API */
void keyfunc_scan_v(uint32_t now_ms_ul);
void keyfunc_subscriber_add_v(struct keyfunc_subscriber_t *sub_pst);

#endif
