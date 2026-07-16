#include "bsp_uart.h"
#include "stm32f10x.h" // 全村唯一包含芯片库的地方

/* ================================================================ */
/*  不透明指针的真正实体                                              */
/* ================================================================ */
struct UART_Periph {
    USART_TypeDef*        usart;        // 实际的 USART 实例指针
    BSP_UART_RxCallback_t rx_callback;  // 接收回调函数指针，这是一个函数指针，指向上层注册的回调函数
};

/* ================================================================ */
/*  静态内存池（不用 malloc，编译期确定大小）                          */
/* ================================================================ */
static struct UART_Periph pool[3];
static uint8_t pool_idx = 0;

// 按 BSP_UART_1/2/3 索引快速查找，供 it.c 中断中使用
static struct UART_Periph* instances[3] = {0, 0, 0};

/* ================================================================ */
/*  内部翻译结构体：存放翻译后的所有 STM32 底层参数                    */
/* ================================================================ */
typedef struct {
    USART_TypeDef* usart_base;      // USART1 / USART2 / USART3
    // TX 引脚信息
    GPIO_TypeDef*  tx_port;         // TX 引脚所在的 GPIO 端口
    uint16_t       tx_pin;          // TX 引脚的 GPIO 引脚号
    uint32_t       tx_gpio_rcc;     // GPIO 时钟（都在 APB2）
    // RX 引脚信息
    GPIO_TypeDef*  rx_port;         // RX 引脚所在的 GPIO 端口
    uint16_t       rx_pin;          // RX 引脚的 GPIO 引脚号
    uint32_t       rx_gpio_rcc;     // GPIO 时钟（都在 APB2）
    // USART 时钟
    uint32_t       usart_rcc;       // USART1→APB2, USART2/3→APB1
    uint8_t        usart_on_apb1;   // 1=APB1, 0=APB2
    // NVIC
    uint8_t        irq_channel;     // USART1=37, USART2=38, USART3=39
} UART_TranslateResult_t;

/* ================================================================ */
/*  内部私有翻译函数：把"通用枚举"翻译成"STM32 方言"                  */
/* ================================================================ */
static void BSP_UART_Translate(BSP_UART_t uart, UART_TranslateResult_t* out)
{
    // 先清零，防止残留值
    out->usart_base   = 0;
    out->tx_port      = 0;
    out->rx_port      = 0;
    out->usart_on_apb1 = 0;

    switch (uart) {
        case BSP_UART_1:
            // --- USART1: TX=PA9, RX=PA10, 时钟在 APB2 ---
            out->usart_base   = USART1;
            out->tx_port      = GPIOA;
            out->tx_pin       = GPIO_Pin_9;
            out->tx_gpio_rcc  = RCC_APB2Periph_GPIOA;
            out->rx_port      = GPIOA;
            out->rx_pin       = GPIO_Pin_10;
            out->rx_gpio_rcc  = RCC_APB2Periph_GPIOA;
            out->usart_rcc    = RCC_APB2Periph_USART1;
            out->usart_on_apb1 = 0;
            out->irq_channel  = USART1_IRQn;
            break;

        case BSP_UART_2:
            // --- USART2: TX=PA2, RX=PA3, 时钟在 APB1 ---
            out->usart_base   = USART2;
            out->tx_port      = GPIOA;
            out->tx_pin       = GPIO_Pin_2;
            out->tx_gpio_rcc  = RCC_APB2Periph_GPIOA;
            out->rx_port      = GPIOA;
            out->rx_pin       = GPIO_Pin_3;
            out->rx_gpio_rcc  = RCC_APB2Periph_GPIOA;
            out->usart_rcc    = RCC_APB1Periph_USART2;
            out->usart_on_apb1 = 1;
            out->irq_channel  = USART2_IRQn;
            break;

        case BSP_UART_3:
            // --- USART3: TX=PB10, RX=PB11, 时钟在 APB1 ---
            out->usart_base   = USART3;
            out->tx_port      = GPIOB;
            out->tx_pin       = GPIO_Pin_10;
            out->tx_gpio_rcc  = RCC_APB2Periph_GPIOB;
            out->rx_port      = GPIOB;
            out->rx_pin       = GPIO_Pin_11;
            out->rx_gpio_rcc  = RCC_APB2Periph_GPIOB;
            out->usart_rcc    = RCC_APB1Periph_USART3;
            out->usart_on_apb1 = 1;
            out->irq_channel  = USART3_IRQn;
            break;

        default:
            break;
    }
}

/* ================================================================ */
/*  内部辅助：配置一个 GPIO 引脚                                      */
/* ================================================================ */
static void BSP_UART_ConfigPin(GPIO_TypeDef* port, uint16_t pin,
                               GPIOMode_TypeDef mode, uint32_t rcc)
{
    RCC_APB2PeriphClockCmd(rcc, ENABLE);

    GPIO_InitTypeDef gi;
    gi.GPIO_Pin   = pin;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    gi.GPIO_Mode  = mode;
    GPIO_Init(port, &gi);
}

/* ================================================================ */
/*  构造函数                                                         */
/* ================================================================ */
struct UART_Periph* UART_Periph_Create(UART_Config_t* cfg)
{
    if (!cfg) return 0;          // 配置指针不能为空
    if (pool_idx >= 3) return 0; // 池子满了

    struct UART_Periph* obj = &pool[pool_idx++];  // 从池子中取一个对象
    obj->rx_callback = 0; // 初始没有回调

    // 1. 翻译：通用枚举 → STM32 底层参数
    UART_TranslateResult_t t;
    BSP_UART_Translate(cfg->uart, &t);

    obj->usart = t.usart_base;

    // 2. 配置 TX 引脚（复用推挽输出）和 RX 引脚（浮空输入）
    BSP_UART_ConfigPin(t.tx_port, t.tx_pin, GPIO_Mode_AF_PP,       t.tx_gpio_rcc);
    BSP_UART_ConfigPin(t.rx_port, t.rx_pin, GPIO_Mode_IN_FLOATING, t.rx_gpio_rcc);

    // 3. 开启 USART 时钟（USART1 在 APB2，USART2/3 在 APB1）
    if (t.usart_on_apb1) {
        RCC_APB1PeriphClockCmd(t.usart_rcc, ENABLE);
    } else {
        RCC_APB2PeriphClockCmd(t.usart_rcc, ENABLE);
    }

    // 4. 配置 USART 参数
    USART_InitTypeDef ui;
    USART_StructInit(&ui); // 先填充默认值

    ui.USART_BaudRate            = cfg->baudrate;
    ui.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    ui.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    // 翻译数据位
    switch (cfg->data_bits) {
        case BSP_UART_DATA_9:
            ui.USART_WordLength = USART_WordLength_9b;
            break;
        case BSP_UART_DATA_8:
        default:
            ui.USART_WordLength = USART_WordLength_8b;
            break;
    }

    // 翻译停止位
    switch (cfg->stop_bits) {
        case BSP_UART_STOP_0_5:
            ui.USART_StopBits = USART_StopBits_0_5;
            break;
        case BSP_UART_STOP_2:
            ui.USART_StopBits = USART_StopBits_2;
            break;
        case BSP_UART_STOP_1_5:
            ui.USART_StopBits = USART_StopBits_1_5;
            break;
        case BSP_UART_STOP_1:
        default:
            ui.USART_StopBits = USART_StopBits_1;
            break;
    }

    // 翻译校验位
    switch (cfg->parity) {
        case BSP_UART_PARITY_ODD:
            ui.USART_Parity = USART_Parity_Odd;
            break;
        case BSP_UART_PARITY_EVEN:
            ui.USART_Parity = USART_Parity_Even;
            break;
        case BSP_UART_PARITY_NONE:
        default:
            ui.USART_Parity = USART_Parity_No;
            break;
    }

    USART_Init(obj->usart, &ui);

    // 5. 使能 USART 外设
    USART_Cmd(obj->usart, ENABLE);

    // 6. 使能 RXNE 接收中断
    USART_ITConfig(obj->usart, USART_IT_RXNE, ENABLE);

    // 7. 配置 NVIC 中断优先级并使能
    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel                   = t.irq_channel;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority        = 0;
    nvic.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nvic);

    // 8. 记录实例到索引表，供 it.c 中断中快速查找
    instances[cfg->uart] = obj;

    return obj;
}

/* ================================================================ */
/*  获取已创建实例（供 it.c 在中断中快速查找）                         */
/* ================================================================ */
struct UART_Periph* BSP_UART_GetInstance(BSP_UART_t uart)
{
    if (uart >= BSP_UART_1 && uart <= BSP_UART_3) {
        return instances[uart];
    }
    return 0;
}

/* ================================================================ */
/*  注册接收回调                                                     */
/* ================================================================ */
void BSP_UART_RegisterRxCallback(struct UART_Periph* uart, BSP_UART_RxCallback_t cb)
{
    if (uart) {
        uart->rx_callback = cb;
    }
}

/* ================================================================ */
/*  中断服务入口（由 stm32f10x_it.c 的 USARTx_IRQHandler 调用）       */
/* ================================================================ */
void BSP_UART_IRQHandler(struct UART_Periph* uart)
{
    if (!uart) return;

    // 接收寄存器非空中断（收到一个字节）
    if (USART_GetITStatus(uart->usart, USART_IT_RXNE) != RESET) {
        uint8_t byte = (uint8_t)USART_ReceiveData(uart->usart);
        // 如果上层注册了回调，逐字节回调
        if (uart->rx_callback) {
            uart->rx_callback(byte);
        }
    }
}

/* ================================================================ */
/*  发送接口（阻塞式）                                                */
/* ================================================================ */

/**
  * @brief  发送单个字节（阻塞等待 TXE）
  */
void UART_SendByte(struct UART_Periph* uart, uint8_t data)
{
    if (!uart) return;
    // 等待发送数据寄存器空
    while (USART_GetFlagStatus(uart->usart, USART_FLAG_TXE) == RESET);
    USART_SendData(uart->usart, (uint16_t)data);
}

/**
  * @brief  发送多个字节（阻塞，逐字节等待）
  */
void UART_SendBytes(struct UART_Periph* uart, const uint8_t* data, uint16_t len)
{
    if (!uart || !data) return;
    uint16_t i;
    for (i = 0; i < len; i++) {
        UART_SendByte(uart, data[i]);
    }
}

/**
  * @brief  发送字符串（阻塞，遇到 \0 结束）
  */
void UART_SendString(struct UART_Periph* uart, const char* str)
{
    if (!uart || !str) return;
    while (*str) {
        UART_SendByte(uart, (uint8_t)(*str));
        str++;
    }
}
