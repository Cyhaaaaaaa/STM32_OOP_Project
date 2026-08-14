#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include <stdint.h>

// 1. 不透明指针声明（对外隐藏真实的寄存器细节，极其关键）
struct GPIO_Pin;
// 专业解释：这里只「前向声明」结构体名字，不暴露它的成员——这就是 C 语言实现封装（encapsulation）的手段。
// 上层（Device/App）只能拿到 struct GPIO_Pin* 这个指针类型，却看不到结构体里有什么字段，
// 于是无法直接碰底层寄存器，只能调用 BSP 提供的函数。真正的定义在 bsp_gpio.c 里，叫做「不透明类型 (opaque type)」。
// 好处：将来换芯片（比如 GD32），只要改 bsp_gpio.c 内部实现，上层代码一行都不用动。

// 2. 硬件无关的词汇表（通用枚举）
typedef enum {
    BSP_PORT_A,
    BSP_PORT_B,
    BSP_PORT_C,
    BSP_PORT_D,
    // 如果后续换用大容量芯片，可以直接在这里扩展 BSP_PORT_E 等
} BSP_Port_t;
// 专业解释：这是分层的关键——BSP 层用一套「自己发明、跟具体芯片无关」的抽象词汇。
// 上层说"我要 A 口的 13 号脚"，而不直接说"我要 GPIOA 的 PIN13"（那是 STM32 专属说法）。
// 翻译成 STM32 方言的脏活，全部藏在 bsp_gpio.c 的翻译函数里。

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
// 专业解释：这就是「工厂方法模式 (Factory Method)」里的"订单/参数"。上层只描述"我要什么"，
// 不关心"怎么造"。构造函数 GPIO_Pin_Create 拿到这张订单，内部完成时钟使能、引脚初始化、返回对象指针。
// 配合 C99 的「指定初始化器」(.port = ...)，调用点读起来像一份清单，一目了然。

// 4. BSP 操作接口
struct GPIO_Pin* GPIO_Pin_Create(GPIO_Config_t* cfg);
// 专业解释：构造函数。注意返回的是 struct GPIO_Pin* 不透明指针——这就是你手里那张"遥控器"。
// 上层拿到它后，只能传给下面这些 Set/Toggle/Get 函数，无法私自改内部寄存器。

void GPIO_SetHigh(struct GPIO_Pin* pin);
void GPIO_SetLow(struct GPIO_Pin* pin);
void GPIO_Toggle(struct GPIO_Pin* pin);
uint8_t GPIO_GetLevel(struct GPIO_Pin* pin);
// 专业解释：这些是对象的「方法」。C 语言没有真正的类，就用"结构体指针作为第一个参数"来模拟
// 面向对象里的 this 指针——函数操作的始终是传进来的那个对象，这正是 C 里最经典的 OOP 套路。

#endif
