#include "workqueue.h"
#include "delay.h"   /* 只要 Get_SysTick() 这个时间基准，不含任何芯片寄存器 */

/*
 * ============================================================================
 *  迷你 workqueue 实现 —— 一个带"到点时间"的单向链表
 * ============================================================================
 */

/* 队列头：指向最早挂进来的任务。空队列时为 NULL。 */
static struct work_struct *work_head = NULL;

/*
 * 把一个任务挂进队列（尾插法）。
 * 口语化解释：每个任务就像一张"便签"，上面写着"几点钟提醒我"。
 * 我们把它按顺序贴在墙上，run_workqueue() 就是那个到点就看墙的闹钟。
 */
void schedule_delayed_work(struct work_struct *work, uint32_t delay_ms)
{
    if (!work || !work->func) {
        return; /* 空指针或没绑函数，直接忽略，防止后面解引用崩掉 */
    }

    /* 已经在队列里（或正在跑），不重复排队——这是幂等保护 */
    if (work->state != WORK_IDLE) {
        return;
    }

    work->delay_ms = delay_ms;
    work->due_ms   = Get_SysTick() + delay_ms; /* 记下"到点时刻" */
    work->state    = WORK_PENDING;
    work->next     = NULL;

    /* 尾插到链表末尾 */
    if (work_head == NULL) {
        work_head = work;
    } else {
        struct work_struct *p = work_head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = work;
    }
}

/*
 * 取消一个还没到点的任务。
 * 专业解释：单链表删除节点需要前驱指针，所以先遍历找到 work 的前一个节点 prev，
 * 再让 prev->next 跳过 work。找不到（说明它没在队列里）就返回 false。
 */
bool cancel_work(struct work_struct *work)
{
    if (!work) {
        return false;
    }

    struct work_struct *prev = NULL;
    struct work_struct *w    = work_head;

    while (w != NULL && w != work) {
        prev = w;
        w = w->next;
    }

    if (w == NULL) {
        return false; /* 不在队列里，没东西可取消 */
    }

    /* 摘链 */
    if (prev != NULL) {
        prev->next = w->next;
    } else {
        work_head = w->next; /* 删的是头节点 */
    }

    w->next  = NULL;
    w->state = WORK_IDLE;
    return true;
}

/*
 * 处理所有到点的任务——这就是主循环里唯一要调用的函数。
 *
 * 关键难点：任务在执行时可能会"自调度"（又把自己挂回队列）。
 * 所以必须先把当前节点从链表上摘下来、再调用它的 func，
 * 否则 func 里再 schedule 自己时，链表结构会被搅乱。
 */
void run_workqueue(void)
{
    uint32_t now = Get_SysTick();

    struct work_struct *prev = NULL;
    struct work_struct *w    = work_head;

    while (w != NULL) {
        /* 先记下下一个节点，因为当前节点马上要被摘走/重排 */
        struct work_struct *next = w->next;

        /*
         * 到点判断：(int32_t)(now - due) >= 0 表示"现在 >= 到点时刻"。
         * 为什么用有符号减法？因为 now 和 due 都是 uint32_t，49.7 天会回绕。
         * 用 (int32_t) 强制解释差值，就能在回绕时依然正确比较先后。
         */
        if (w->state == WORK_PENDING && (int32_t)(now - w->due_ms) >= 0) {
            /* 1. 从链表上摘下来 */
            if (prev != NULL) {
                prev->next = next;
            } else {
                work_head = next;
            }

            w->next  = NULL;
            w->state = WORK_RUNNING;

            /* 2. 执行任务（任务里可能会再 schedule 自己） */
            w->func(w->arg);

            /* 3. 如果任务没有自调度，就把它放回"空闲"态，等待下次被 schedule */
            if (w->state == WORK_RUNNING) {
                w->state = WORK_IDLE;
            }

            w = next; /* 继续看下一个节点（prev 不动，因为摘掉的不是 prev 后面的了） */
        } else {
            prev = w;  /* 当前节点没到点，它成为下一个被摘节点的"前驱" */
            w    = next;
        }
    }
}

bool workqueue_empty(void)
{
    return work_head == NULL;
}
