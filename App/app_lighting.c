// App/app_lighting.c (业务逻辑的黑盒)
#include "app_lighting.h"
#include "bsp_led.h"   // 引入 LED 对象
#include "delay.h"     // 引入非阻塞时间服务

// 私有变量被完美地锁在这个文件里，绝不污染 main.c
static uint32_t blink_timer = 0; 
static uint32_t current_blink_interval = 100; // 默认 100ms 闪一次

// 组件初始化
void App_Lighting_Init(void) {
    // 依赖注入：在这里决定业务层要控制哪个具体的硬件对象
    SysLED->Init(SysLED); 
    blink_timer = 0; // 初始化时间戳
}

// 核心业务任务 (交给 main 函数的 while(1) 去无限调用)
void App_Lighting_Task(void) {
    // 所有的闪烁逻辑、非阻塞调度，全部在这个黑盒里完成
    if (systick_timeout(&blink_timer, current_blink_interval)) {
        SysLED->Toggle(SysLED); 
    }
}

