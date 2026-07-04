#include "device_led.h"
#include "bsp_gpio.h"
// 绝对没有任何 STM32/GD32 相关的头文件！

// 定义黑盒遥控器，习惯性赋 0 (NULL) 防止野指针
static struct GPIO_Pin* led_green = 0; 
static struct GPIO_Pin* led_red = 0;

// 用纯粹的抽象语言描述你的物理连线（利用 C99 指定初始化器，极度清晰）
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

void Device_LED_Init(void) {
    // 把“配置单”交给 BSP 工厂，拿回“遥控器”
    led_green = GPIO_Pin_Create(&led_green_cfg);
    led_red   = GPIO_Pin_Create(&led_red_cfg);
}

// 以下全部变成了拿着“遥控器”按按键的纯逻辑操作
void Device_LED_Green_On(void)      { GPIO_SetLow(led_green); }  
void Device_LED_Green_Off(void)     { GPIO_SetHigh(led_green); }
void Device_LED_Green_Toggle(void)  { GPIO_Toggle(led_green); }

void Device_LED_Red_On(void)        { GPIO_SetLow(led_red); }  
void Device_LED_Red_Off(void)       { GPIO_SetHigh(led_red); }
void Device_LED_Red_Toggle(void)    { GPIO_Toggle(led_red); }
