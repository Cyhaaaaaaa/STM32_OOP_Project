#include "app_lighting.h"
#include "bsp_led.h" 
#include "delay.h"   

// 为两个灯分配各自独立的“秒表”
static uint32_t sys_led_timer = 0; 
static uint32_t run_led_timer = 0; 

// 初始化
void App_Lighting_Init(void) {
    SysLED->Init(SysLED); // 初始化 PC13
    RunLED->Init(RunLED); // 初始化 PA1
}

// 核心业务任务
void App_Lighting_Task(void) {
    // PC13 灯：慢闪，500ms 翻转一次
    if (systick_timeout(&sys_led_timer, 500)) {
        SysLED->Toggle(SysLED); 
    }
    
    // PA1 灯：快闪，100ms 翻转一次
    if (systick_timeout(&run_led_timer, 100)) {
        RunLED->Toggle(RunLED); 
    }
}
