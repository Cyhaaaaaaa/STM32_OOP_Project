#include "delay.h"

/* * 静态全局变量，用于保存 1us 和 1ms 需要的 SysTick 时钟节拍数。
 * 作用域仅限本文件，不对外暴露，体现封装思想。
 */
static uint8_t  fac_us = 0;   // us延时倍乘数
static uint16_t fac_ms = 0;   // ms延时倍乘数

/**
  * @brief  初始化延迟函数
  * @param  无
  * @retval 无
  */
void Delay_Init(void)
{
    // 选择 SysTick 时钟源为 HCLK/8 (默认 HCLK = 72MHz，所以 SysTick 时钟为 9MHz)
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    
    // 9MHz 即 1秒钟计数 9,000,000 次，所以 1us 计数 9 次
    fac_us = SystemCoreClock / 8000000; 
    
    // 1ms = 1000us
    fac_ms = (uint16_t)fac_us * 1000;
}

/**
  * @brief  微秒级延时
  * @param  nus: 要延时的微秒数 (注意：nus的值不要过大，最大值不要超过 1864135 us)
  * @retval 无
  */
void Delay_us(uint32_t nus)
{
    uint32_t temp;
    
    SysTick->LOAD = nus * fac_us;             // 加载时间值 (计数值 = 时间 * 倍乘数)
    SysTick->VAL  = 0x00;                     // 清空计数器 (写入任意值都会清零 VAL 寄存器)
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk; // 开启 SysTick 开始倒数计时
    
    // 循环等待：判断 ENABLE 位是否还在且第 16 位 (COUNTFLAG) 是否为 1 (计数到 0 时硬件置 1)
    do {
        temp = SysTick->CTRL;
    } while ((temp & 0x01) && !(temp & (1 << 16)));
    
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk; // 关闭 SysTick
    SysTick->VAL  = 0x00;                      // 清空计数器
}

/**
  * @brief  毫秒级延时 (改进版，无最大时间限制)
  * @param  nms: 要延时的毫秒数 (随意多大均可)
  * @retval 无
  */
void Delay_ms(uint32_t nms)
{
    uint32_t i;
    
    // 为了防止 SysTick 24位寄存器溢出，将其拆解为每次循环 1ms
    for(i = 0; i < nms; i++)
    {
        Delay_us(1000); // 1ms = 1000us
    }
}