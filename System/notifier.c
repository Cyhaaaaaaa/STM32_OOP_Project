#include "notifier.h"
#include "workqueue.h"   /* 系统事件总线用 workqueue 做"1 秒定时" */
#include "delay.h"

/*
 * ============================================================================
 *  第一部分：通用通知链实现
 * ============================================================================
 *  本质就是一个单向链表，链表节点是订阅者(notifier_block)。
 *  广播 = 从链头走到链尾，逐个调用每个订阅者的 call。
 */

void notifier_block_init(struct notifier_block *nb, notifier_call_t call)
{
    if (!nb) {
        return;
    }
    nb->call = call;
    nb->next = NULL;
}

void notifier_chain_init(struct notifier_block **head)
{
    if (head) {
        *head = NULL;
    }
}

int notifier_chain_register(struct notifier_block **head, struct notifier_block *nb)
{
    if (!head || !nb) {
        return -1;
    }

    nb->next = NULL;

    /* 头插法：新订阅者插到最前面，O(1) 完成 */
    if (*head == NULL) {
        *head = nb;
    } else {
        nb->next = *head;
        *head = nb;
    }
    return 0;
}

int notifier_chain_unregister(struct notifier_block **head, struct notifier_block *nb)
{
    if (!head || !nb) {
        return -1;
    }

    /* 单链表删除：找前驱 */
    struct notifier_block *prev = NULL;
    struct notifier_block *w    = *head;

    while (w != NULL && w != nb) {
        prev = w;
        w = w->next;
    }

    if (w == NULL) {
        return -1; /* 没订阅过，没东西可退 */
    }

    if (prev != NULL) {
        prev->next = w->next;
    } else {
        *head = w->next;
    }
    w->next = NULL;
    return 0;
}

int notifier_chain_notify(struct notifier_block **head, unsigned long event, void *data)
{
    if (!head) {
        return 0;
    }

    /*
     * 专业解释：返回值用"或"累加（NOTIFY_STOP 的位会保留下来）。
     * 只要有一个订阅者返回非 0，最终返回值就非 0——发布者据此知道"有人拦截了"。
     */
    int ret = 0;
    struct notifier_block *nb = *head;

    while (nb != NULL) {
        ret |= nb->call(nb, event, data);
        nb = nb->next;
    }

    return ret;
}

/* ============================================================================
 *  第二部分：系统事件总线（内置 1 秒心跳）
 * ============================================================================
 */

/* 系统事件链头 */
static struct notifier_block *sys_event_head = NULL;

/* 心跳任务节点：每秒广播一次 SYS_EVT_SECOND */
static struct work_struct sys_event_work;

#define SYS_EVT_TICK_MS  1000U   /* 1 秒心跳间隔 */

/*
 * 心跳回调：广播一次"过了一秒"，然后把自己再挂进队列，形成自调度。
 * 口语化解释：这个函数像一只"报时鸟"，每过一秒叫一声，叫完自己再定下一秒的闹钟。
 */
static void sys_event_tick_cb(void *arg)
{
    (void)arg;

    notifier_chain_notify(&sys_event_head, SYS_EVT_SECOND, NULL);

    /* 自调度：再挂 1 秒。这就是 workqueue 替代轮询的典型用法。 */
    schedule_delayed_work(&sys_event_work, SYS_EVT_TICK_MS);
}

int sys_event_init(void)
{
    INIT_WORK(&sys_event_work, "sys_event", sys_event_tick_cb, NULL);
    schedule_delayed_work(&sys_event_work, SYS_EVT_TICK_MS);
    return 0;
}

int sys_event_register(struct notifier_block *nb)
{
    return notifier_chain_register(&sys_event_head, nb);
}

int sys_event_unregister(struct notifier_block *nb)
{
    return notifier_chain_unregister(&sys_event_head, nb);
}

void sys_event_post(unsigned long event, void *data)
{
    notifier_chain_notify(&sys_event_head, event, data);
}
