#include "device_led.h"
#include "bsp_gpio.h"
// 绝对没有任何 STM32/GD32 相关的头文件！
// 专业解释：这一行是关键约束——设备层只能 #include BSP 的抽象头文件，绝不碰 stm32f10x.h。
// 一旦出现芯片相关头文件，就说明分层被破坏了。保持这个约束，换芯片时设备层才一行都不用改。

// 定义黑盒遥控器，习惯性赋 0 (NULL) 防止野指针

//定义两个GPIO_Pin结构体指针，来存放构造函数返回的指针
static struct GPIO_Pin* led_green = 0;
static struct GPIO_Pin* led_red = 0;
// 专业解释：这是设备层的"私有成员"——它持有 BSP 层返回的不透明对象指针（那张遥控器）。
// static 让它们只在本文件可见（私有）；初始化为 0(NULL)，避免未初始化就使用导致野指针。
// 注意：这里只知道它是 struct GPIO_Pin*，完全不知道里面存的是 STM32 的端口还是引脚。

// 用纯粹的抽象语言描述你的物理连线（利用 C99 指定初始化器，极度清晰）
//定义两个GPIO_Config结构体变量，来存放LED的配置信息
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
// 专业解释：这两张"订单"描述的是硬件事实（绿灯接 C13、红灯接 A1，推挽输出），
// 用的是 BSP 定义的抽象词汇(BSP_PORT_C/BSP_PIN_13)，而不是 STM32 的 GPIOA/Pin_13。
// .port = ... 这种「指定初始化器」是 C99 特性，按字段名赋值，可读性远超按位置赋值。
// 将来换电路板只要改这两张表，其余逻辑一概不动——这就是"把变化隔离在数据里"。

void Device_LED_Init(void) {
    // 把“配置单”交给 BSP 工厂，拿回“遥控器”
    //传入GPIO_Config_t类型的指针，通过构造函数来配置真实的GPIO引脚，再返回一个GPIO_Pin类型的指针，存放在led_green和led_red中
    //GPIO_Pin这个结构体为半透明结构体，外部无法访问，需要通过BSP提供的接口来操作
    led_green = GPIO_Pin_Create(&led_green_cfg);
    led_red   = GPIO_Pin_Create(&led_red_cfg);
    // 专业解释：这就是「依赖注入 + 工厂模式」。设备层把订单(配置)交给 BSP 的工厂函数，
    // 工厂内部完成真正的硬件初始化（开时钟、配引脚），返回不透明对象指针存进 led_green/led_red。
    // 从此设备层只需握着这两张"遥控器"操作，底层细节完全被 BSP 隐藏。
}

// 以下全部变成了拿着“遥控器”按按键的纯逻辑操作
//调用bsp层封装好的函数，传入GPIO_Pin这个结构体的指针，这些函数内部可以访问到GPIO_Pin里面的成员，从而实现对LED的控制操作
void Device_LED_Green_On(void)      { GPIO_SetLow(led_green); }
void Device_LED_Green_Off(void)     { GPIO_SetHigh(led_green); }
void Device_LED_Green_Toggle(void)  { GPIO_Toggle(led_green); }

void Device_LED_Red_On(void)        { GPIO_SetLow(led_red); }
void Device_LED_Red_Off(void)       { GPIO_SetHigh(led_red); }
void Device_LED_Red_Toggle(void)    { GPIO_Toggle(led_red); }
// 专业解释：这些是设备层暴露给 App 的"语义化接口"。注意它们只描述行为(开/关/翻转)，
// 且用"低电平点亮、高电平熄灭"(SetLow=On、SetHigh=Off)这样的物理事实封装在这里。
// App 层调用 Device_LED_Green_On() 时，连"LED 是低电平亮"这种电路细节都不需要知道——
// 这就是逐层抽象的价值：每一层只负责把自己该隐藏的细节藏好。
