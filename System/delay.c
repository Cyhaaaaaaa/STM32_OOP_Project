#include "delay.h"

// 全局系统心跳计数器（每1ms递增1次）
// volatile 关键字防止编译器优化，因为它会在中断中被修改
static volatile uint32_t sys_tick_ms = 0; 
static uint32_t sys_tick_us_fac = 0; // 微秒延时的乘数

/**
  * @brief  初始化延迟函数 (配置为 1ms 心跳中断)
  */
void Delay_Init(void)
{
    // 配置 SysTick 产生 1ms 的中断
    // SystemCoreClock 默认是 72000000 (72MHz)
    // 72000000 / 1000 = 72000，即每 72000 个时钟周期进一次中断 (恰好是 1ms)
    SysTick_Config(SystemCoreClock / 1000); 
    
    // 计算 1us 需要的时钟周期数 (72MHz下为 72)
    sys_tick_us_fac = SystemCoreClock / 1000000;
}

/**
  * @brief 驱动系统心跳 (由 stm32f10x_it.c 中的 SysTick_Handler 调用)
  */
void Delay_IncTick(void)
{
    sys_tick_ms++; // 系统的全局时间，每 1ms 加一
}

/**
  * @brief 获取当前系统时间 (毫秒)
  */
uint32_t Get_SysTick(void)
{
    return sys_tick_ms;
}

/**
  * @brief  微秒级延时 (精准读取寄存器，不依赖中断次数)
  */
void Delay_us(uint32_t nus)
{
    uint32_t ticks = nus * sys_tick_us_fac;
    uint32_t t0 = SysTick->VAL; // 记录初始值
    uint32_t t1;
    uint32_t elapsed = 0;
    
    // 循环判断已经走过的 tick 数
    while (elapsed < ticks) {
        t1 = SysTick->VAL;
        if (t1 != t0) {
            if (t1 < t0) {
                elapsed += t0 - t1;
            } else {
                // 如果 t1 > t0，说明 SysTick 的 24位递减计数器刚刚重装载过了
                elapsed += SysTick->LOAD + 1 - t1 + t0;
            }
            t0 = t1;
        }
    }
}

/**
  * @brief  毫秒级延时 (阻塞式，基于心跳)
  */
void Delay_ms(uint32_t nms)
{
    uint32_t start = sys_tick_ms;
    while ((sys_tick_ms - start) < nms) {
        // 阻塞等待，但此时底层的 SysTick 依然在运行
    }
}

/**
  * @brief 简单的非阻塞超时检查器
  * @usage 
  * static uint32_t my_timer = 0;
  * if (systick_timeout(&my_timer, 500)) { 
  * // 每隔500ms执行一次这里的代码
  * }
  */
bool systick_timeout(uint32_t *timestamp, uint32_t timeout_ms)
{
    // 如果达到了指定的超时时间
    if (sys_tick_ms - *timestamp >= timeout_ms) {
        *timestamp = sys_tick_ms; // 自动重置时间戳为当前时间
        return true;
    }
    return false;
}

/**
  * @brief 状态保持检测器
  * @usage
  * static State_Timer_t key_timer = {0};
  * uint8_t key_status = GPIO_ReadInputDataBit(...); // 实时读取按键状态
  * // 如果按键保持按下(假设按下为1)超过 20ms，返回 true
  * if (State_Keep_Timeout(&key_timer, key_status, 20)) {
  * // 执行按键按下逻辑 (彻底替代传统的 Delay_ms(20) 消抖)
  * }
  */
bool State_Keep_Timeout(State_Timer_t *timer, uint8_t current_state, uint32_t timeout_ms)
{
    // 1. 如果是第一次运行，或者状态发生了突变（被干扰了）
    if (timer->is_first == false || timer->last_state != current_state) {
        timer->last_state = current_state; // 记录下当前的新状态
        timer->timestamp = sys_tick_ms;    // 刷新时间戳起点
        timer->is_first = true;            // 标记已初始化
        return false;                      // 状态变了，肯定不算保持，返回 false
    }
    
    // 2. 如果状态没有发生变化，检查“保持的时间”是否达标
    if (sys_tick_ms - timer->timestamp >= timeout_ms) {
        return true; // 恭喜，状态稳如老狗，时间也够了！
    }
    
    return false;
}
