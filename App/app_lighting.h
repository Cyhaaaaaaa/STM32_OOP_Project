// App/app_lighting.h
#ifndef __APP_LIGHTING_H
#define __APP_LIGHTING_H

// App 层（应用/业务层）：这是整个分层架构的最顶层，只表达"业务需求"。
// 它只依赖 Device 层提供的语义化接口（开灯、关灯、闪烁），
// 不关心引脚、寄存器、甚至不关心灯是 LED 还是继电器——那些都由下层封装好了。
// 每个业务模块用 <模块名>_Init() 初始化、<模块名>_Task() 轮询，是统一的分层约定。

void App_Lighting_Init(void);
void App_Lighting_Task(void);

#endif
