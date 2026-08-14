#ifndef __DEVICE_LED_H__
#define __DEVICE_LED_H__

// 这一层叫「设备层 (Device Layer)」：它只关心"LED 这个设备能做什么"（开/关/翻转），
// 完全不关心 LED 具体接在哪个芯片的哪个引脚——那些是 BSP 层的职责。
// 头文件里只暴露"能干什么"（接口），不暴露"怎么实现"（实现都在 .c 里），
// 所以 App 层调用 Device_LED_Green_On() 时，完全不知道、也不需要知道底下是 STM32 还是别的芯片。

void Device_LED_Init(void);
void Device_LED_Green_On(void);
void Device_LED_Green_Off(void);
void Device_LED_Green_Toggle(void);
void Device_LED_Red_On(void);
void Device_LED_Red_Off(void);
void Device_LED_Red_Toggle(void);

#endif
