#ifndef OLED_MENU_H
#define OLED_MENU_H

#include "menu.h"
#include "ano_scheduler.h"

#define PHASE_DEBUG 0
/*
 * OLED 菜单页面定义
 *
 * 菜单树结构 (在 OLED_Menu.c 中构建):
 *
 *   g_root (Main)          ← 文件夹: 自动显示子项列表
 *   └─ g_menu_select       ← 文件夹: 自动显示子项列表
 *      ├─ s_encode_st      ← 叶子: draw 显示禁飞区数据
 *      └─ g_item_que2      ← 叶子: draw 显示禁飞区数据
 *
 * 用户只需:
 *   1. Create_Menu_Folder/Leaf 声明树
 *   2. 为叶子写 draw 函数
 *   UP/DOWN/ENTER/BACK 由框架自动处理, 无需手动连线
 */

/* 根节点 (外部可见, 用于 menu_navigate_v 等) */
extern struct menu_base_t* g_encode_pst;

/* 初始化: 构建菜单树 + 注册按键订阅 */

#endif /* OLED_MENU_H */
