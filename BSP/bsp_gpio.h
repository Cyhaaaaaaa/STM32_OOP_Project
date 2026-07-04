#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include <stdint.h>

// 1. 不透明指针声明（对外隐藏真实的寄存器细节，极其关键）
struct GPIO_Pin;

// 2. 硬件无关的词汇表（通用枚举）
typedef enum {
    BSP_PORT_A,
    BSP_PORT_B,
    BSP_PORT_C,
    BSP_PORT_D,
    // 如果后续换用大容量芯片，可以直接在这里扩展 BSP_PORT_E 等
} BSP_Port_t;

typedef enum {
    BSP_PIN_0 = 0,
    BSP_PIN_1,
    BSP_PIN_2,
    BSP_PIN_3,
    BSP_PIN_4,
    BSP_PIN_5,
    BSP_PIN_6,
    BSP_PIN_7,
    BSP_PIN_8,
    BSP_PIN_9,
    BSP_PIN_10,
    BSP_PIN_11,
    BSP_PIN_12,
    BSP_PIN_13,
    BSP_PIN_14,
    BSP_PIN_15
} BSP_Pin_t;

typedef enum {
    BSP_GPIO_MODE_OUT_PP,       // 推挽输出
    BSP_GPIO_MODE_IN_PULLUP,    // 上拉输入
    BSP_GPIO_MODE_IN_FLOATING   // 浮空输入
    // 后续用到定时器 PWM 或 SPI 时，在这里加上 BSP_GPIO_MODE_AF_PP 等
} BSP_GPIO_Mode;

// 3. 纯净的配置结构体 (设备层给 BSP 下达的“订单”)
typedef struct {
    BSP_Port_t    port;
    BSP_Pin_t     pin;
    BSP_GPIO_Mode mode;
} GPIO_Config_t;

// 4. BSP 操作接口
struct GPIO_Pin* GPIO_Pin_Create(GPIO_Config_t* cfg);

void GPIO_SetHigh(struct GPIO_Pin* pin);
void GPIO_SetLow(struct GPIO_Pin* pin);
void GPIO_Toggle(struct GPIO_Pin* pin);
uint8_t GPIO_GetLevel(struct GPIO_Pin* pin);

#endif
