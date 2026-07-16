#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include <stdint.h>

// 1. 不透明指针声明（对外隐藏真实的寄存器细节）
struct UART_Periph;

// 2. 硬件无关的词汇表（通用枚举）
typedef enum {
    BSP_UART_1,
    BSP_UART_2,
    BSP_UART_3
    // 如果后续换用大容量芯片（如 STM32F4），可以直接在这里扩展 BSP_UART_4 等
} BSP_UART_t;

// 3. 通用枚举（数据位、停止位、校验位）
typedef enum {
    BSP_UART_DATA_8 = 0,
    BSP_UART_DATA_9 = 1
} BSP_UART_DataBits_t;

// 3. 通用枚举（停止位）
typedef enum {
    BSP_UART_STOP_1   = 0,
    BSP_UART_STOP_0_5 = 1,
    BSP_UART_STOP_2   = 2,
    BSP_UART_STOP_1_5 = 3
} BSP_UART_StopBits_t;

// 3. 通用枚举（校验位）
typedef enum {
    BSP_UART_PARITY_NONE = 0,
    BSP_UART_PARITY_ODD  = 1,
    BSP_UART_PARITY_EVEN = 2
} BSP_UART_Parity_t;

// 4. 纯净的配置结构体（设备层给 BSP 下达的"订单"）
typedef struct {
    BSP_UART_t          uart;       // 选择的 UART 实例
    uint32_t            baudrate;   // 波特率
    BSP_UART_DataBits_t data_bits;  // 数据位
    BSP_UART_StopBits_t stop_bits;  // 停止位
    BSP_UART_Parity_t   parity;     // 校验位
} UART_Config_t;

// 5. 接收回调函数类型（逐字节回调，由上层注册）
typedef void (*BSP_UART_RxCallback_t)(uint8_t byte);

// 6. BSP UART 操作接口，作用是返回一个 UART 实例的指针，用于bsp层的底层操作
struct UART_Periph* UART_Periph_Create(UART_Config_t* cfg); 

// 中断服务入口（由 stm32f10x_it.c 中的 USARTx_IRQHandler 调用）
void BSP_UART_IRQHandler(struct UART_Periph* uart);

// 获取已创建实例的指针（供 it.c 在中断中快速查找）
struct UART_Periph* BSP_UART_GetInstance(BSP_UART_t uart);

// 注册接收回调（上层传入 Middlewares 环形缓冲区的写入函数等）
void BSP_UART_RegisterRxCallback(struct UART_Periph* uart, BSP_UART_RxCallback_t cb);

// 发送接口（阻塞式，等待 TXE 完成后返回）
void UART_SendByte(struct UART_Periph* uart, uint8_t data);
void UART_SendBytes(struct UART_Periph* uart, const uint8_t* data, uint16_t len);
void UART_SendString(struct UART_Periph* uart, const char* str);

#endif
