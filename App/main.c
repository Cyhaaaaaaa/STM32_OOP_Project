// App/main.c
#include "delay.h"         // 底层心跳服务
#include "app_lighting.h"  // 灯光业务组件
#include "bsp_uart.h"      // [临时测试] UART BSP 层
#include "device_led.h"    // [临时测试] LED 设备层，用于视觉指示
// 未来你会加： #include "app_weight_sensor.h"

/* ================================================================ */
/*  [临时测试] UART 回环测试 —— 用完后会删除，移到 Devices 层        */
/* ================================================================ */
static struct UART_Periph* test_uart;

// 接收回调：每收到一个字节 → 立即回显 + 翻转绿灯表示"有数据来"
static void test_uart_rx_callback(uint8_t byte) {
    UART_SendByte(test_uart, byte);   // 回显：PC 发什么就回什么
    Device_LED_Green_Toggle();        // 绿灯闪烁 = 视觉确认收到数据
}

int main(void) {
    /* ========================================== */
    /* 1. 全局系统与内核级服务初始化              */
    /* ========================================== */

    // 配置 NVIC 中断优先级分组（必须在所有外设初始化之前调用）
    // Group 2: 2位抢占优先级(0~3) + 2位子优先级(0~3)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    Delay_Init(); // 启动 1ms 系统心跳，这是整个非阻塞架构的引擎

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
    test_uart = UART_Periph_Create(&uart_cfg);
    BSP_UART_RegisterRxCallback(test_uart, test_uart_rx_callback);

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

        // 未来你会加： App_WeightSensor_Task();
        // 即使那个任务里包含了极度复杂的 HX711 滤波和 214.0 的系数运算，
        // 也完全不会阻塞这里灯光的闪烁。它们是完全并行的！
    }
}
