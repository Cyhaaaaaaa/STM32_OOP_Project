#ifndef __BSP_LED_H
#define __BSP_LED_H

#include <stdint.h>

// 前向声明，用于函数指针的参数类型匹配
struct LED_Device;

/**
 * @brief LED 对象结构体（类的抽象）
 * @note  绝对不允许在这里包含 stm32f10x.h 任何硬件库文件！
 */
typedef struct LED_Device {
    // --- 1. 行为 (Methods) ---
    void (*Init)(struct LED_Device* dev);
    void (*TurnOn)(struct LED_Device* dev);
    void (*TurnOff)(struct LED_Device* dev);
    void (*Toggle)(struct LED_Device* dev); // 新增翻转功能，更实用
    
    // --- 2. 属性 (Properties) ---
    // 万能指针：这是极致解耦的灵魂！它隐藏了底层的硬件细节。
    // 应用层不需要知道这里面装的是 GPIO 还是 I2C 地址。
    void* hw_config; 
} LED_Device_t;

// 外部声明：向全系统暴露的“系统指示灯”对象指针
extern LED_Device_t* SysLED;

#endif /* __BSP_LED_H */
