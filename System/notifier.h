#ifndef __NOTIFIER_H__
#define __NOTIFIER_H__

#include <stdint.h>
#include <stdbool.h>

/*
 * ============================================================================
 *  迷你 notifier（发布 / 订阅）
 * ============================================================================
 *
 *  一句话理解它解决什么问题：
 *     "键盘被按了"这件事，怎么让"灯""蜂鸣器""屏幕"三个互不相识的模块都知道？
 *     办法：键盘只负责把事件"广播"出去，谁想知道谁提前"订阅"。广播的人
 *     不认识订阅的人，订阅的人也不认识广播的人——这就是发布/订阅（观察者模式）。
 *
 *  专业解释：
 *     notifier 的核心是一个「通知链」：一串 notifier_block 节点，每个节点里
 *     挂着一个回调函数 call。发布者调用 notifier_chain_notify() 时，会沿着链
 *     逐个调用 call，把事件编号(event)和数据(data)传下去。订阅者 = 把自己的
 *     回调节点注册进链。这样生产者和消费者彻底解耦：
 *       - 加一个新的订阅者 → 只加一个 notifier_block，不动生产者和其它订阅者
 *       - 换掉事件来源（键盘换成触摸）→ 只改发布方，所有订阅者无感
 *     这正是 sc2016x_template 里 key_register_notifier() 背后的东西。
 */

struct notifier_block;

/* 回调函数原型：返回 0 继续传播，返回非 0 停止传播 */
typedef int (*notifier_call_t)(struct notifier_block *nb,
                               unsigned long event, void *data);

/* 回调的返回值约定 */
#define NOTIFY_DONE  0   /* 我处理完了，继续通知下一个订阅者 */
#define NOTIFY_STOP  1   /* 我处理完了，别往下传了 */

/* 订阅者节点 */
struct notifier_block {
    notifier_call_t call;      /* 事件来了要调的回调 */
    struct notifier_block *next; /* 链指针 */
};

/* ============================================================================
 * 第一部分：通用通知链（想给哪个模块建事件链都行）
 * ========================================================================== */

/* 初始化一个订阅者节点，绑定回调 */
void notifier_block_init(struct notifier_block *nb, notifier_call_t call);

/* 初始化一个链头（一个空的订阅列表） */
void notifier_chain_init(struct notifier_block **head);

/* 订阅：把 nb 挂进 head 链（允许重复订阅） */
int notifier_chain_register(struct notifier_block **head, struct notifier_block *nb);

/* 退订：把 nb 从 head 链摘掉 */
int notifier_chain_unregister(struct notifier_block **head, struct notifier_block *nb);

/* 广播：沿链逐个调用回调，把 event 和 data 传下去 */
int notifier_chain_notify(struct notifier_block **head, unsigned long event, void *data);

/* ============================================================================
 * 第二部分：系统事件总线（内置一个"每秒广播一次"的心跳事件源）
 * ============================================================================
 *  这里演示一个真实用途：系统需要给所有模块一个"1 秒节拍"，但谁也不想让
 *  main 循环每圈都问"到 1 秒了吗"。于是用一个 workqueue 任务每秒广播一次
 *  SYS_EVT_SECOND，需要"每秒做一次"的模块只要订阅它就行。
 *  专业解释：事件源（1s 定时）和事件分发（通知链）在这里组合，展示
 *  workqueue + notifier 两个机制如何配合——这是事件驱动架构的完整最小闭环。
 */

/* 系统级事件编号 */
enum {
    SYS_EVT_SECOND = 0,   /* 每秒广播一次（内部 workqueue 触发） */
    /* 未来加：SYS_EVT_KEY_PRESS, SYS_EVT_LOW_BATTERY ... */
};

/* 启动系统事件总线：内部用 workqueue 每秒广播 SYS_EVT_SECOND */
int sys_event_init(void);

/* 订阅系统事件 */
int sys_event_register(struct notifier_block *nb);
int sys_event_unregister(struct notifier_block *nb);

/* 主动广播一个系统事件（供未来按键模块、保护模块等调用） */
void sys_event_post(unsigned long event, void *data);

#endif /* __NOTIFIER_H__ */
