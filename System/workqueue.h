#ifndef __WORKQUEUE_H__
#define __WORKQUEUE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>   /* NULL */

/*
 * ============================================================================
 *  迷你 workqueue（延迟任务调度器）
 * ============================================================================
 *
 *  一句话理解它解决什么问题：
 *     以前的写法是主循环里"每圈都问一遍每个任务到点没有"（轮询）；
 *     现在的写法是"任务自己说：我 500ms 之后要再跑一次"，然后主循环只管
 *     问调度器"有没有人到点了？有就叫他跑"。
 *
 *  专业解释：
 *     workqueue 是「延迟任务队列」——每个任务是一个 work_struct 节点，调用
 *     schedule_delayed_work() 时把自己挂进一个链表，并记录"到点时刻(due)"。
 *     主循环调用 run_workqueue() 遍历链表，发现某个节点的 due 已经到达，
 *     就摘下来执行它的 func。任务跑完后可以再次 schedule_delayed_work()
 *     把自己挂回队列，形成"自调度"——这就是事件驱动里最核心的一块拼图。
 *
 *     对比一下：
 *       轮询（systick_timeout）:  主循环每圈调用 Task()，Task 内部判断"到点没"
 *       事件（workqueue）:         任务只在需要时"挂起"，主循环不关心它，到点才唤醒
 *     轮询是"我去问你"，事件是"你到点喊我"。任务越多，后者的优势越明显。
 */

/* work 节点的状态 */
enum {
    WORK_IDLE = 0,      /* 空闲：没在队列里，也没在跑 */
    WORK_PENDING,       /* 挂起：已经进队列，正在等待到点 */
    WORK_RUNNING,       /* 运行中：正被执行（用来识别"自调度"） */
};

/*
 * work 节点（这是完整定义，不是不透明指针——为什么？）
 *  因为上层模块要用 `static struct work_struct my_work;` 静态分配它，
 *  编译器必须知道它多大。所以它和 GPIO_Pin 不一样，得把字段摊开给所有人看。
 *  但约定：这些字段只由 workqueue.c 去读，上层不要手改。
 */
struct work_struct {
    const char *name;       /* 任务名（调试用，可以传空字符串） */
    void (*func)(void *arg);/* 到点后要执行的函数 */
    void *arg;              /* 传给 func 的参数 */

    uint32_t delay_ms;      /* 用户填的延迟时长 */
    uint32_t due_ms;        /* 到点时刻 = 调度时刻 + delay_ms */

    uint8_t state;          /* 当前状态：IDLE / PENDING / RUNNING */
    struct work_struct *next; /* 链表指针（队列内部用） */
};

/*
 * 初始化一个 work 节点（有点像 C++ 里的"构造"）。
 * 用法：INIT_WORK(&my_work, "my_work", my_func, my_arg);
 * 专业解释：宏展开成一段 do{}while(0)，把结构体字段全部置为安全初值。
 * 用宏而不是函数，是为了让调用点看起来像"声明即初始化"，读代码更直观。
 */
#define INIT_WORK(_work, _name, _func, _arg) \
    do {                                     \
        (_work)->name     = (_name);         \
        (_work)->func     = (_func);         \
        (_work)->arg      = (_arg);          \
        (_work)->delay_ms = 0;               \
        (_work)->due_ms   = 0;               \
        (_work)->state    = WORK_IDLE;       \
        (_work)->next     = NULL;            \
    } while (0)

/*
 * 把一个任务延迟 delay_ms 毫秒后执行。
 * 如果这个任务已经在队列里（或正在跑），本次调用会被忽略——不会重复排队。
 * 专业解释：这是幂等保护。自调度任务在回调里会再 schedule 自己一次，
 * 如果允许重复排队，链表里就会出现同一个节点两次，run 的时候会跑两遍甚至死循环。
 */
void schedule_delayed_work(struct work_struct *work, uint32_t delay_ms);

/*
 * 取消一个还没到点的任务（从队列里摘出来，回到 IDLE）。
 * 返回 true 表示确实取消了一个挂起的任务，false 表示它本来就没在排队。
 * 专业解释：在协作式单线程模型里没有"正在跑还要等它跑完"的场景，
 * 所以这里只提供 cancel_work（不阻塞），不提供 cancel_work_sync。
 */
bool cancel_work(struct work_struct *work);

/*
 * 主循环里唯一要调用的函数：检查队列，把所有到点的任务都跑一遍。
 * 专业解释：这是整个事件驱动架构的"心脏"。它遍历链表，把 due 已到的节点
 * 摘下来执行。之所以要先"摘下来再执行"，是因为任务在 func 里很可能又会
 * schedule 自己（自调度）——如果还挂在链表上，重排时就会破坏链表结构。
 */
void run_workqueue(void);

/* 队列是否为空（调试/低功耗判断用） */
bool workqueue_empty(void);

#endif /* __WORKQUEUE_H__ */
