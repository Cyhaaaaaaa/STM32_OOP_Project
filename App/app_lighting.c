#include "app_lighting.h"
#include "device_led.h"
#include "delay.h"


static uint32_t led_green_timer = 0;
static uint32_t led_red_timer = 0;
// 专业解释：每个需要"每隔一段时间执行一次"的任务，都配一个 uint32_t 时间戳变量。
// 它记录"上一次触发时刻"，配合 systick_timeout() 实现非阻塞定时。
// static 让它们私有且跨调用保持值（不会每次进函数都被重置成 0）。


void App_Lighting_Init(void) {
    Device_LED_Init();        // 告诉设备层：初始化灯
    // 专业解释：App 层不亲自去碰 GPIO，只把"初始化灯"这件事下放给 Device 层。
    // 这就是依赖方向：App → Device → BSP，上层只调用下一层，绝不跨层或反向依赖。
}

void App_Lighting_Task(void) {
      // 500ms 闪烁一次
    if (systick_timeout(&led_green_timer, 300)) {
        Device_LED_Green_Toggle();
    }
    if (systick_timeout(&led_red_timer, 100)) {
        Device_LED_Red_Toggle();
    }
    // 专业解释：这是「非阻塞轮询 (non-blocking polling)」的标准写法。
    // systick_timeout 检查"距上次触发是否已满 300ms"，满了才执行一次并自动重置时间戳；
    // 没满就立刻返回 false，主循环继续往下走。于是绿灯 300ms 翻一次、红灯 100ms 翻一次，
    // 两个任务"并行"互不阻塞，也完全不占用 CPU 傻等。
}
