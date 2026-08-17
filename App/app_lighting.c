#include "app_lighting.h"
#include "device_led.h"
#include "notifier.h"

/*
 * 事件驱动版的灯光业务逻辑。
 *
 * 两种机制各管一盏灯，方便你对照：
 *   - 红灯：workqueue 自驱动。App 用 Device_LED_SetBehavior 声明"100ms 闪一次"，
 *           之后设备内部一个 20ms 的任务自己去翻转，App 再也不管它。
 *   - 绿灯：notifier 事件驱动。App 订阅"每秒事件(SYS_EVT_SECOND)"，
 *           每次事件广播，回调自动翻转绿灯。App 也不轮询，只是被动收事件。
 *
 * 结论：main 循环里不再需要调用任何 App_Lighting_Task()。
 * 这就是从"轮询"到"事件驱动"的核心转变——任务自己知道何时该醒。
 */

/* 订阅者节点：App 层向系统事件总线注册的"订阅单" */
static struct notifier_block g_sec_nb;

/*
 * 事件回调：系统每秒广播一次 SYS_EVT_SECOND，就翻转一下绿灯。
 * 口语化解释：这是"报时鸟"的听众——每听见一声报时，就按一下绿灯的开关。
 * 专业解释：回调运行在 run_workqueue() 的调用链里（中断之外），
 * 但要保持短小快，别放耗时操作，否则会拖住整个 workqueue 的调度。
 */
static int on_second_tick(struct notifier_block *nb, unsigned long event, void *data)
{
    (void)nb;
    (void)event;
    (void)data;

    Device_LED_Toggle(LED_IDX_GREEN);   /* 每秒翻转一次绿灯 */
    return NOTIFY_DONE;                 /* 处理完，继续通知下一个订阅者 */
}

void App_Lighting_Init(void)
{
    Device_LED_Init();

    /* 红灯：交给 workqueue 自驱动，每 100ms 翻转一次 */
    Device_LED_SetBehavior(LED_IDX_RED, LED_MODE_BLINK, 100);

    /* 绿灯：交给 notifier 事件驱动，每秒翻转一次 */
    Device_LED_Off(LED_IDX_GREEN);                 /* 先灭，等第一个事件点亮 */
    notifier_block_init(&g_sec_nb, on_second_tick);
    sys_event_register(&g_sec_nb);                 /* 订阅"每秒事件" */
}
