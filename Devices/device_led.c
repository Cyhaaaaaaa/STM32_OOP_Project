#include "device_led.h"
#include "bsp_gpio.h"   /* 只依赖 BSP 抽象接口，绝对不碰 stm32f10x.h */
#include "workqueue.h"  /* 事件驱动：让 LED 自己定时翻转 */
#include "delay.h"      /* 只要 Get_SysTick() 时间基准 */

/*
 * 设计说明（事件驱动版 vs 轮询版）：
 *   轮询版：App 每圈调用 Task()，Task 里用 systick_timeout 判断到点没、再翻转。
 *   事件版：App 只"声明"红灯要 100ms 闪一次，之后不再过问；设备内部挂一个
 *           workqueue 任务，每 20ms 醒来一次检查"该翻转了吗"，到点就翻，
 *           翻完再把自己挂回队列——完全自驱动，不占 App 的一行轮询代码。
 *
 * 这个 20ms 的 work 就是 sc2016x 里 module_led 那套"灯语播放器"的极简雏形：
 *   一个定时 tick 推进时间线，把"怎么闪"表达成数据，而不是一堆 if/else。
 */

/* 每个 LED 通道的内部状态（设备层私有，上层看不到） */
typedef struct {
    struct GPIO_Pin *pin;        /* BSP 返回的"遥控器" */
    LED_Mode_t       mode;       /* 当前行为：常灭/常亮/闪烁 */
    uint16_t         period_ms;  /* 闪烁的翻转间隔 */
    uint32_t         last_toggle;/* 上次翻转的时刻 */
    bool             level;      /* 逻辑电平：true=亮，false=灭 */
} LED_Channel;

static LED_Channel g_leds[LED_IDX_COUNT];

/*
 * 硬件描述（配置单）：绿灯接 PC13、红灯接 PA1，推挽输出。
 * 专业解释：LED 负极接 GPIO，正极接 VCC，所以"亮"= 引脚拉低。
 * 这个"低电平点亮"的物理事实被封装在下面的 led_apply_level 里，
 * 上层只管说"亮/灭"，不用知道是拉高还是拉低。
 */
static GPIO_Config_t led_green_cfg = {
    .port = BSP_PORT_C,
    .pin  = BSP_PIN_13,
    .mode = BSP_GPIO_MODE_OUT_PP
};

static GPIO_Config_t led_red_cfg = {
    .port = BSP_PORT_A,
    .pin  = BSP_PIN_1,
    .mode = BSP_GPIO_MODE_OUT_PP
};

/* 自驱动 tick 周期：20ms 醒来检查一次 */
#define LED_TICK_MS  20U

/* 把"逻辑亮灭"翻译成"物理高低电平"，并缓存状态 */
static void led_apply_level(LED_Channel *ch, bool on)
{
    if (!ch || !ch->pin) {
        return;
    }
    if (on) {
        GPIO_SetLow(ch->pin);   /* 低电平 = 点亮（本板 LED 约定） */
    } else {
        GPIO_SetHigh(ch->pin);  /* 高电平 = 熄灭 */
    }
    ch->level = on;
}

/* ---- 立即控制接口（事件驱动用） ---- */

void Device_LED_On(LED_Index_t idx)
{
    if (idx < LED_IDX_COUNT) {
        led_apply_level(&g_leds[idx], true);
    }
}

void Device_LED_Off(LED_Index_t idx)
{
    if (idx < LED_IDX_COUNT) {
        led_apply_level(&g_leds[idx], false);
    }
}

void Device_LED_Toggle(LED_Index_t idx)
{
    if (idx < LED_IDX_COUNT) {
        led_apply_level(&g_leds[idx], !g_leds[idx].level);
    }
}

/* ---- 行为描述接口（自驱动用） ---- */

void Device_LED_SetBehavior(LED_Index_t idx, LED_Mode_t mode, uint16_t period_ms)
{
    if (idx >= LED_IDX_COUNT) {
        return;
    }

    LED_Channel *ch = &g_leds[idx];
    ch->mode      = mode;
    ch->period_ms = period_ms;

    /* 立即应用初始状态，避免要等下一个 tick 才生效 */
    switch (mode) {
    case LED_MODE_ON:
        led_apply_level(ch, true);
        break;
    case LED_MODE_OFF:
        led_apply_level(ch, false);
        break;
    case LED_MODE_BLINK:
        ch->last_toggle = Get_SysTick();
        led_apply_level(ch, false);   /* 从"灭"开始，下一次 tick 翻到"亮" */
        break;
    }
}

/* ---- 自驱动 work（设备层私有） ---- */

static struct work_struct g_led_work;

/*
 * 20ms 醒来一次：扫描所有"闪烁"通道，到翻转间隔就翻转一次。
 * 处理完再把自己挂回队列，形成自调度。
 */
static void led_tick_work_cb(void *arg)
{
    (void)arg;

    uint32_t now = Get_SysTick();

    for (int i = 0; i < LED_IDX_COUNT; i++) {
        LED_Channel *ch = &g_leds[i];

        if (ch->mode == LED_MODE_BLINK && ch->period_ms != 0) {
            /* (int32_t) 有符号比较，防 uint32 回绕 */
            if ((int32_t)(now - ch->last_toggle) >= ch->period_ms) {
                ch->last_toggle = now;
                led_apply_level(ch, !ch->level);
            }
        }
    }

    /* 自调度：过 20ms 再醒。这就是替代主循环轮询的关键一步。 */
    schedule_delayed_work(&g_led_work, LED_TICK_MS);
}

void Device_LED_Init(void)
{
    /* 1. 用配置单创建 GPIO 对象（依赖注入 + 工厂） */
    g_leds[LED_IDX_GREEN].pin = GPIO_Pin_Create(&led_green_cfg);
    g_leds[LED_IDX_RED].pin   = GPIO_Pin_Create(&led_red_cfg);

    /* 2. 全部先置"常灭"的安全默认态 */
    for (int i = 0; i < LED_IDX_COUNT; i++) {
        g_leds[i].mode        = LED_MODE_OFF;
        g_leds[i].period_ms   = 0;
        g_leds[i].last_toggle = 0;
        led_apply_level(&g_leds[i], false);
    }

    /* 3. 启动自驱动任务：LED 从此"自己会闪"，App 不用每圈轮询它 */
    INIT_WORK(&g_led_work, "led_tick", led_tick_work_cb, NULL);
    schedule_delayed_work(&g_led_work, LED_TICK_MS);
}
