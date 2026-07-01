#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"
#include <stdbool.h>

// 初始化延时系统与心跳
void Delay_Init(void);

// 驱动系统心跳 (需在底层 SysTick_Handler 中调用)
void Delay_IncTick(void);

// 阻塞式延时（微秒级和毫秒级）
void Delay_us(uint32_t nus);
void Delay_ms(uint32_t nms);

// 获取系统当前运行的毫秒数
uint32_t Get_SysTick(void);

/**
 * @brief 非阻塞超时检查器
 * @param timestamp 存储上次触发时间的变量指针
 * @param timeout_ms 需要等待的毫秒数
 * @retval true: 时间到了 / false: 时间还没到
 */
bool systick_timeout(uint32_t *timestamp, uint32_t timeout_ms);


/**
 * @brief 状态保持检测器结构体（对象化）
 */
typedef struct {
    uint32_t timestamp;  // 状态开始的时间戳
    uint8_t  last_state; // 上一次的状态记录
    bool     is_first;   // 是否是首次运行标记
} State_Timer_t;

/**
 * @brief 状态保持超时检测
 * @param timer 状态定时器对象指针
 * @param current_state 当前被检测的实时状态
 * @param timeout_ms 状态需要稳定保持的毫秒数
 * @retval true: 状态已经稳定保持了设定的时间 / false: 状态有波动或时间未到
 */
bool State_Keep_Timeout(State_Timer_t *timer, uint8_t current_state, uint32_t timeout_ms);

#endif /* __DELAY_H */
