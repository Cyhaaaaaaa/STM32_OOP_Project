#ifndef __BSP_UART_H__
#define __BSP_UART_H__

#include <stdint.h>

// 1. 不透明指针声明（对外隐藏真实的寄存器细节）
struct UART_Periph;
// 专业解释：和 GPIO 一样的「不透明类型」套路——对外只给指针类型，成员定义藏在 bsp_uart.c。
// 上层拿到 struct UART_Periph* 后只能调用下面的接口函数，无法直接碰 USART 寄存器。

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
// 专业解释：同样是「工厂方法」的订单结构体。上层只描述串口参数（波特率/数据位/停止位/校验位），
// 完全不关心这些参数最终写到哪个寄存器。构造函数内部负责翻译并落地到 STM32 的 USART 外设。

// 5. 接收回调函数类型（逐字节回调，由上层注册）
typedef void (*BSP_UART_RxCallback_t)(uint8_t byte);
// 专业解释：函数指针类型。它定义了一种"函数形状"：接收一个 uint8_t 参数、无返回值。
// 上层把自己的处理函数（如把字节塞进环形缓冲区）传进来注册，底层收到数据时通过这个指针"反向调用"上层，
// 实现「回调 (callback)」——这是 C 里模拟"事件通知/观察者模式"的核心手段。

// 6. BSP UART 操作接口，作用是返回一个 UART 实例的指针，用于bsp层的底层操作
struct UART_Periph* UART_Periph_Create(UART_Config_t* cfg);
// 专业解释：UART 的构造函数，输入订单、输出不透明对象指针，内部完成引脚复用配置、
// 时钟使能、USART 参数初始化、NVIC 中断使能等一整套"造对象"流程。

// 中断服务入口（由 stm32f10x_it.c 中的 USARTx_IRQHandler 调用）
void BSP_UART_IRQHandler(struct UART_Periph* uart);
// 专业解释：这是底层中断处理逻辑的"落脚点"。真正的 ISR（中断服务函数，在 stm32f10x_it.c）
// 只负责"查到实例、转发进来"，具体怎么处理（读哪个标志、取什么字节、调哪个回调）都在这里完成，
// 让中断文件保持极简，业务逻辑仍归属 BSP 层。

// 获取已创建实例的指针（供 it.c 在中断中快速查找）
struct UART_Periph* BSP_UART_GetInstance(BSP_UART_t uart);
// 专业解释：中断发生时，ISR 只有"是哪个 USART 触发"这一条信息，没有对象指针。
// 这个函数根据编号去全局实例表里查回对应对象，再交给 IRQHandler 处理，是中断与对象之间的桥梁。

// 注册接收回调（上层传入 Middlewares 环形缓冲区的写入函数等）
void BSP_UART_RegisterRxCallback(struct UART_Periph* uart, BSP_UART_RxCallback_t cb);

// 发送接口（阻塞式，等待 TXE 完成后返回）
void UART_SendByte(struct UART_Periph* uart, uint8_t data);
void UART_SendBytes(struct UART_Periph* uart, const uint8_t* data, uint16_t len);
void UART_SendString(struct UART_Periph* uart, const char* str);

#endif
