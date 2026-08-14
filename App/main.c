// App/main.c
#include "delay.h"         // 底层心跳服务
#include "misc.h"          // NVIC 优先级分组配置（芯片级全局配置）
#include "app_lighting.h"  // 灯光业务组件
#include "bsp_uart.h"      // [临时测试] UART BSP 层
#include "device_led.h"    // [临时测试] LED 设备层，用于视觉指示
// 未来你会加： #include "app_weight_sensor.h"
// 专业解释：main.c 是整个工程的入口，也是分层的"总装车间"。
// 它直接 include 了三个层级的头文件，但这属于"组装"，真正的工作顺序是：
//   初始化 → 各 App 模块 Init → 主循环里轮询各 App 模块 Task。
// 依赖方向始终保持 App → Device → BSP 单向向下，下层永远不认识上层。

/* ================================================================ */
/*  [临时测试] UART 回环测试 —— 用完后会删除，移到 Devices 层        */
/* ================================================================ */
static struct UART_Periph* test_uart;
// 专业解释：全局持有 UART 的不透明对象指针，后续发送和回调都要用它。
// static 限定了文件作用域（私有），避免被其它模块误用。

// 接收回调：每收到一个字节 → 立即回显 + 翻转绿灯表示"有数据来"
static void test_uart_rx_callback(uint8_t byte) {
    UART_SendByte(test_uart, byte);   // 回显：PC 发什么就回什么
    Device_LED_Green_Toggle();        // 绿灯闪烁 = 视觉确认收到数据
}
// 专业解释：这个函数是「回调函数」，签名必须匹配 BSP_UART_RxCallback_t（void f(uint8_t)）。
// 它不会主动被主循环调用，而是当 USART 中断收到一个字节时，由 BSP 层反向调用它。
// 注意它运行在"中断上下文"里——所以要尽量短小快，不能放耗时操作（否则会拖住整个中断）。

int main(void) {
    /* ========================================== */
    /* 1. 全局系统与内核级服务初始化              */
    /* ========================================== */

    // 配置 NVIC 中断优先级分组（必须在所有外设初始化之前调用）
    // Group 2: 2位抢占优先级(0~3) + 2位子优先级(0~3)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    // 专业解释：NVIC 优先级分组决定了 4bit 优先级怎么切分成"抢占优先级"和"子优先级"。
    // Group2 表示 2bit 抢占(0~3) + 2bit 子优先级(0~3)。抢占优先级决定"谁能打断谁"，
    // 子优先级只在两个中断同时触发时裁决先后。必须在任何 NVIC_Init 之前定好，之后不能乱改。

    Delay_Init(); // 启动 1ms 系统心跳，这是整个非阻塞架构的引擎
    // 专业解释：配置 SysTick 每 1ms 触发一次中断，中断里累加全局毫秒计数。
    // 有了这个"时间基准"，上层才能用 systick_timeout 做非阻塞定时——它是整个轮询架构的地基。

    /* ========================================== */
    /* 2. 各个业务组件 (App 层) 初始化            */
    /* ========================================== */
    App_Lighting_Init();
    // 未来你会加： App_WeightSensor_Init();

    /* ========================================== */
    /*  [临时测试] UART1 初始化: PA9(TX) PA10(RX) */
    /*  配一个 USB 转串口接到 PA9/PA10 即可测试     */
    /* ========================================== */
    UART_Config_t uart_cfg = {
        .uart      = BSP_UART_1,
        .baudrate  = 115200,
        .data_bits = BSP_UART_DATA_8,
        .stop_bits = BSP_UART_STOP_1,
        .parity    = BSP_UART_PARITY_NONE,
    };
    // 专业解释：用 C99 指定初始化器构造 UART 的"订单"（配置结构体）。
    // 这份订单完全用 BSP 的抽象词汇描述（BSP_UART_1 / DATA_8 / STOP_1），
    // 不出现任何 STM32 寄存器或引脚——引脚映射(PA9/PA10)藏在 BSP 的翻译函数里。

    test_uart = UART_Periph_Create(&uart_cfg);
    BSP_UART_RegisterRxCallback(test_uart, test_uart_rx_callback);
    // 专业解释：把配置交给工厂函数"造"出 UART 对象，再把上面的回调函数注册进去。
    // 之后只要 USART1 中断收到字节，就会自动回调 test_uart_rx_callback。

    // 上电发一条欢迎消息，确认 TX 通路正常
    UART_SendString(test_uart, "\r\n================================\r\n");
    UART_SendString(test_uart, "  BSP UART Echo Test Ready!\r\n");
    UART_SendString(test_uart, "  Send anything, I'll echo it.\r\n");
    UART_SendString(test_uart, "================================\r\n\r\n");

    /* ========================================== */
    /* 3. 主调度轮询 (大循环)                     */
    /* ========================================== */
    while (1) {
        // 疯狂轮询，各个组件内部利用 systick_timeout 自行决定是否执行
        App_Lighting_Task();   // 红灯 100ms 闪 = 系统心跳，绿灯由 UART 回调翻转
        // 专业解释：这是「超级循环 (super loop)」调度模型——主循环无限高速转圈，
        // 每圈依次调用各 App 模块的 Task()。每个 Task 内部用 systick_timeout 判断
        // "到点了吗？没到就立刻返回"，所以整个循环转得飞快，CPU 永不阻塞在某一个任务上。

        // 未来你会加： App_WeightSensor_Task();
        // 即使那个任务里包含了极度复杂的 HX711 滤波和 214.0 的系数运算，
        // 也完全不会阻塞这里灯光的闪烁。它们是完全并行的！
    }
}
