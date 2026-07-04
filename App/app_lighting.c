#include "app_lighting.h"
#include "device_led.h"
#include "delay.h"   


static uint32_t led_green_timer = 0;
static uint32_t led_red_timer = 0;


void App_Lighting_Init(void) {
    Device_LED_Init();        // 告诉设备层：初始化灯
}

void App_Lighting_Task(void) {
      // 500ms 闪烁一次
    if (systick_timeout(&led_green_timer, 300)) {
        Device_LED_Green_Toggle();  
    }
    if (systick_timeout(&led_red_timer, 100)) {
        Device_LED_Red_Toggle();  
    }
}
