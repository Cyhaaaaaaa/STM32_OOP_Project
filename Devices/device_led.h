#ifndef __DEVICE_LED_H__
#define __DEVICE_LED_H__

#include <stdint.h>

/*
 * 这一层叫「设备层 (Device Layer)」：它只关心"LED 这个设备能做什么"，
 * 完全不关心 LED 接在哪个芯片的哪个引脚——那些是 BSP 层的职责。
 *
 * 本文件从"轮询版"升级成"事件驱动版"后，接口分成了两类：
 *   1) 立即控制接口（On/Off/Toggle）——给"事件回调"用，事件来了就动作一下；
 *   2) 行为描述接口（SetBehavior）——给"自驱动"用，把"怎么闪"描述成数据，
 *      交给设备内部一个 workqueue 任务自己去跑，App 不用再每圈轮询。
 */

/* LED 编号（设备层的"索引词汇"，比写死 green/red 函数更通用） */
typedef enum {
    LED_IDX_GREEN = 0,
    LED_IDX_RED,
    LED_IDX_COUNT,   /* 总个数，不是真实 LED */
} LED_Index_t;

/* LED 的显示模式 */
typedef enum {
    LED_MODE_OFF   = 0,   /* 常灭 */
    LED_MODE_ON    = 1,   /* 常亮 */
    LED_MODE_BLINK = 2,   /* 闪烁（需要 period_ms） */
} LED_Mode_t;

/*
 * 立即控制接口：开 / 关 / 翻转。
 * 用法场景：notifier 回调里"收到事件就翻转一下"，事件驱动就用这三个。
 */
void Device_LED_Init(void);
void Device_LED_On(LED_Index_t idx);
void Device_LED_Off(LED_Index_t idx);
void Device_LED_Toggle(LED_Index_t idx);

/*
 * 行为描述接口：设置某个 LED 的"行为"。
 *   mode      : 常灭 / 常亮 / 闪烁
 *   period_ms : 闪烁模式下，每间隔 period_ms 翻转一次（亮、灭各占 period_ms）
 * 用法场景：App 初始化时"声明"红灯要 100ms 闪一次，之后就不用管它了，
 * 设备内部一个 workqueue 任务会自动按这个节拍翻转。
 */
void Device_LED_SetBehavior(LED_Index_t idx, LED_Mode_t mode, uint16_t period_ms);

#endif /* __DEVICE_LED_H__ */
