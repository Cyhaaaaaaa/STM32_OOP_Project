#ifndef __APP_LIGHTING_H
#define __APP_LIGHTING_H

/*
 * App 层（应用/业务层）：整个分层架构的最顶层，只表达"业务需求"。
 *
 * 注意这里相比"轮询版"的升级：
 *   以前有 App_Lighting_Init() + App_Lighting_Task() 两个函数，main 每圈要调 Task。
 *   现在只有 App_Lighting_Init() 一个函数——因为 App 不再需要"每圈被轮询"了：
 *     - 红灯的闪烁是 App 在 Init 里"声明"了一个行为（100ms 闪），交给设备自驱动；
 *     - 绿灯的闪烁是 App 订阅了"每秒事件"，事件来了回调自动翻转。
 *   App 只负责"说清楚要什么"，然后躺着等事件发生，这就是事件驱动的味道。
 */

void App_Lighting_Init(void);

#endif
