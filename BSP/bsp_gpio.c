#include "bsp_gpio.h"
#include "stm32f10x.h" // 全村唯一包含芯片库的地方

// 不透明指针的真正实体：只保存真实的寄存器地址和引脚宏
struct GPIO_Pin {
    GPIO_TypeDef* port;
    uint16_t      pin;
};

// 内部私有翻译函数：把“通用枚举”翻译成“STM32方言”
static void BSP_GPIO_Translate(BSP_Port_t bsp_port, BSP_Pin_t bsp_pin, 
                               GPIO_TypeDef** out_port, uint16_t* out_pin, uint32_t* out_rcc) {
    
    // 1. 翻译 Port 和时钟 (不同芯片家族的时钟总线可能不同，这里是隔离差异的好地方)
    switch (bsp_port) {
        case BSP_PORT_A: 
            *out_port = GPIOA; 
            *out_rcc = RCC_APB2Periph_GPIOA; 
            break;
        case BSP_PORT_B: 
            *out_port = GPIOB; 
            *out_rcc = RCC_APB2Periph_GPIOB; 
            break;
        case BSP_PORT_C: 
            *out_port = GPIOC; 
            *out_rcc = RCC_APB2Periph_GPIOC; 
            break;
        default:
            // 严谨的做法：可以加个断言 assert_param
            break;
    }
    
    // 2. 翻译 Pin：利用底层规律直接位移，极致高效
    *out_pin = (uint16_t)(1 << bsp_pin);
}

// 构造函数
struct GPIO_Pin* GPIO_Pin_Create(GPIO_Config_t* cfg) {
    // 简单的静态内存池，避免使用 malloc
    static struct GPIO_Pin pool[16];
    static uint8_t pool_idx = 0;
    
    // 如果超出了池子大小，直接返回 NULL (或者做断言保护)
    if (pool_idx >= 16) return 0; 
    
    struct GPIO_Pin* obj = &pool[pool_idx++];
    uint32_t rcc_clock = 0;
    
    // 获取真实的底层硬件参数
    BSP_GPIO_Translate(cfg->port, cfg->pin, &obj->port, &obj->pin, &rcc_clock);

    // 开启时钟
    RCC_APB2PeriphClockCmd(rcc_clock, ENABLE);

    // 配置底层结构体
    GPIO_InitTypeDef gi;
    gi.GPIO_Pin = obj->pin;
    gi.GPIO_Speed = GPIO_Speed_50MHz;

    switch(cfg->mode){
        case BSP_GPIO_MODE_OUT_PP:
            gi.GPIO_Mode = GPIO_Mode_Out_PP;
            break;
        case BSP_GPIO_MODE_IN_PULLUP:
            gi.GPIO_Mode = GPIO_Mode_IPU;
            break;
        case BSP_GPIO_MODE_IN_FLOATING:
            gi.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            break;
    }
    
    GPIO_Init(obj->port, &gi);
    
    // 默认输出高电平，防止外设在上电瞬间乱动
    if(cfg->mode == BSP_GPIO_MODE_OUT_PP) {
        GPIO_SetBits(obj->port, obj->pin); 
    }
    
    return obj;
}

// ---------------- 操作接口 ----------------

void GPIO_SetHigh(struct GPIO_Pin* pin) {
    if (pin) GPIO_SetBits(pin->port, pin->pin);
}

void GPIO_SetLow(struct GPIO_Pin* pin) {
    if (pin) GPIO_ResetBits(pin->port, pin->pin);
}

void GPIO_Toggle(struct GPIO_Pin* pin) {
    if (pin) pin->port->ODR ^= pin->pin; // 直接操作寄存器最高效
}

uint8_t GPIO_GetLevel(struct GPIO_Pin* pin) {
    if (!pin) return 0;
    return (pin->port->IDR & pin->pin) ? 1 : 0;
}

