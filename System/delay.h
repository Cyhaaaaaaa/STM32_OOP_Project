#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

// 初始化延时函数
void Delay_Init(void);

// 微秒级延时
void Delay_us(uint32_t nus);

// 毫秒级延时
void Delay_ms(uint32_t nms);

#endif /* __DELAY_H */