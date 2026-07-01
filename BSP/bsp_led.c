#include "bsp_led.h"
#include "stm32f10x.h"      // 允许且仅允许在这里包含硬件底层库
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/* ==================================================================== */
/* ===================== 1. 定义与实例化硬件配置 ======================== */
/* ==================================================================== */

// 定义真实的硬件配置结构体 (Private，对应用层完全隐身)
typedef struct {
    GPIO_TypeDef* port;
    uint16_t      pin;
    uint32_t      rcc_clock;
} LED_HW_Config_t;

// 实例化 PC13 的硬件配置
static LED_HW_Config_t PC13_Config = {
    GPIOC,
    GPIO_Pin_13,
    RCC_APB2Periph_GPIOC
};

// 1. 新增 PA1 的硬件配置参数
static LED_HW_Config_t PA1_Config = {
    GPIOA,
    GPIO_Pin_1,
    RCC_APB2Periph_GPIOA
};

/* ==================================================================== */
/* ======================= 2. 底层行为的具体实现 ======================== */
/* ==================================================================== */

// 注意：所有函数都加了 static 修饰，严禁外部直接调用这些函数！
// 外部只能通过对象里的指针来调用。

static void LED_HW_Init(LED_Device_t* dev) {
    // 将万能指针强转回我们真实的硬件配置
    LED_HW_Config_t* cfg = (LED_HW_Config_t*)dev->hw_config;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 开启时钟
    RCC_APB2PeriphClockCmd(cfg->rcc_clock, ENABLE);
    
    // 配置 GPIO
    GPIO_InitStructure.GPIO_Pin = cfg->pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(cfg->port, &GPIO_InitStructure);
    
    // PC13 是低电平点亮，所以初始化默认置高（灭灯）
    GPIO_SetBits(cfg->port, cfg->pin);
}

static void LED_HW_TurnOn(LED_Device_t* dev) {
    LED_HW_Config_t* cfg = (LED_HW_Config_t*)dev->hw_config;
    GPIO_ResetBits(cfg->port, cfg->pin); // 拉低，亮
}

static void LED_HW_TurnOff(LED_Device_t* dev) {
    LED_HW_Config_t* cfg = (LED_HW_Config_t*)dev->hw_config;
    GPIO_SetBits(cfg->port, cfg->pin); // 拉高，灭
}

static void LED_HW_Toggle(LED_Device_t* dev) {
    LED_HW_Config_t* cfg = (LED_HW_Config_t*)dev->hw_config;
    // 巧用寄存器异或操作，实现电平翻转
    cfg->port->ODR ^= cfg->pin; 
}


/* ==================================================================== */
/* ======================== 3. 依赖注入与装配 =========================== */
/* ==================================================================== */

// 实例化一个 LED 对象，并把上面写的“函数”和“硬件属性”装配进去
static LED_Device_t System_LED_Instance = {
    .Init    = LED_HW_Init,
    .TurnOn  = LED_HW_TurnOn,
    .TurnOff = LED_HW_TurnOff,
    .Toggle  = LED_HW_Toggle,
    .hw_config = &PC13_Config  // 把 PC13 的参数挂载给万能指针
};

// 3. 【新增】装配新的 PA1 对象 (方法全复用，只换配置包)
static LED_Device_t Run_LED_Instance = {
    .Init    = LED_HW_Init,      // 依然指向同一个公用函数！
    .TurnOn  = LED_HW_TurnOn,
    .TurnOff = LED_HW_TurnOff,
    .Toggle  = LED_HW_Toggle,
    .hw_config = &PA1_Config     // 唯一不同的地方：挂载了 PA1 的配置
};

// 对外暴露这个对象的指针！应用层拿到的就是这个。
LED_Device_t* SysLED = &System_LED_Instance;
LED_Device_t* RunLED = &Run_LED_Instance;

