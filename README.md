# STM32 OOP 分层架构点灯工程 —— 设计思想与架构解析

> 一个点灯程序能有什么学问？看完这份文档，你会重新思考嵌入式软件的设计方式。

## 📖 目录

- [快速概览](#快速概览)
- [为什么用这种方式点灯？](#为什么用这种方式点灯)
- [为什么要这种架构？](#为什么要这种架构)
- [为什么要这种写法？](#为什么要这种写法)
- [核心思想](#核心思想)
- [架构亮点](#架构亮点)
- [移植到其他芯片](#移植到其他芯片)
- [需要特别注意的地方](#需要特别注意的地方)
- [工程结构](#工程结构)
- [数据流全景图](#数据流全景图)

---

## 快速概览

这是一个基于 STM32F103（Cortex-M3）的 LED 闪烁工程。但它不是你常见的"寄存器一把梭"写法 —— 这个工程的每一行代码，都在回答一个问题：

> **如果这个项目要维护 5 年、要换 3 次芯片、要加 20 个外设，代码应该怎么组织？**

答案就是一个**严格分层、接口隔离、面向对象思想指导的 C 语言工程框架**。

```
┌──────────────────────────────────────┐
│  App Layer (业务逻辑)                 │  ← "绿灯 300ms 闪一次，红灯 100ms 闪一次"
├──────────────────────────────────────┤
│  Devices Layer (设备抽象)             │  ← "有一盏绿灯，有一盏红灯，能开能关能翻转"
├──────────────────────────────────────┤
│  BSP Layer (板级支持包)               │  ← "GPIO 引脚抽象，不依赖具体芯片型号"
├──────────────────────────────────────┤
│  Driver/CMSIS (芯片驱动)              │  ← ST 标准外设库 + ARM Cortex 内核接口
└──────────────────────────────────────┘
```

---

## 为什么用这种方式点灯？

### 传统方式的痛点

在一个典型的 STM32 入门教程里，点灯代码长这样：

```c
// 传统写法 —— 随处可见的教程代码
int main(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio);

    while (1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);  // 亮
        Delay_ms(500);                        // 卡死 500ms
        GPIO_SetBits(GPIOC, GPIO_Pin_13);    // 灭
        Delay_ms(500);                        // 又卡死 500ms
    }
}
```

这段代码有三个致命问题：

| 问题 | 后果 |
|------|------|
| **阻塞延时 `Delay_ms(500)`** | CPU 在 500ms 内完全冻结，不能做任何其他事情。加一个按键检测？再加一个传感器读取？全都得排队等。 |
| **芯片寄存器散落各处** | `GPIOC`、`GPIO_Pin_13`、`RCC_APB2Periph_GPIOC` 这些 STM32 特有的名字散布在业务逻辑里。换 GD32？你得把所有文件翻一遍。 |
| **没有抽象层次** | "绿灯每 500ms 翻转一次"这个业务意图，和"操作 GPIOC 的 ODR 寄存器"这个硬件动作，混在一起无法区分。 |

### 本工程的点灯方式

```c
// App/app_lighting.c —— 业务层只表达意图
void App_Lighting_Task(void) {
    if (systick_timeout(&led_green_timer, 300)) {
        Device_LED_Green_Toggle();  // 我不管你怎么实现，翻转绿灯就行
    }
    if (systick_timeout(&led_red_timer, 100)) {
        Device_LED_Red_Toggle();    // 红灯也一样
    }
}
```

三个根本改变：

1. **非阻塞时间管理** —— `systick_timeout()` 检查时间到了没有，到了就执行，没到就立刻返回。不占用 CPU，不卡死循环。同一个 `while(1)` 里可以跑几十个任务，互不干扰。

2. **语义化命名** —— 代码写的是 `Device_LED_Green_Toggle()`，而不是 `GPIOC->ODR ^= (1 << 13)`。六个月后回来看，一眼就懂。

3. **意图与实现分离** —— 业务层说"我要绿灯闪"，设备层说"绿灯在 PC13"，BSP 层说"PC13 对应 GPIOC 的第 13 位"。每一层只关心自己该关心的东西。

---

## 为什么要这种架构？

### 分层架构全景

```
main.c                              ← 系统组装入口
  │
  ├── App/app_lighting.c            ← 业务组件：灯光控制逻辑
  │     ├── "绿灯 300ms 闪，红灯 100ms 闪"
  │     └── 使用 systick_timeout() 实现非阻塞并行
  │
  ├── Devices/device_led.c          ← 设备抽象：LED 是什么
  │     ├── "有一盏绿灯，一盏红灯"
  │     ├── "用配置单描述硬件连接"
  │     └── 不知道芯片型号，不知道 GPIO 寄存器
  │
  ├── BSP/bsp_gpio.c                ← 板级支持：GPIO 怎么操作
  │     ├── "这是全村唯一 #include 'stm32f10x.h' 的地方"
  │     ├── 翻译：BSP_PORT_C → GPIOC
  │     └── 不透明指针 + 静态内存池
  │
  ├── System/delay.c                ← 系统服务：时间基准
  │     ├── SysTick 1ms 心跳
  │     ├── systick_timeout() 非阻塞超时
  │     └── State_Keep_Timeout() 状态保持检测
  │
  └── Driver/ + CMSIS/              ← 芯片厂商提供
        ├── STM32F10x 标准外设库
        ├── CMSIS Cortex-M3 内核接口
        └── 启动文件、中断向量表
```

### 每一层解决什么问题？

#### 1. App 层 —— 业务逻辑

```c
// 这是整个系统里唯一需要"读懂你的产品需求"的地方
// 它不关心 LED 接在哪个引脚、不关心芯片型号
// 只关心：绿灯要闪多快？红灯要闪多快？
void App_Lighting_Task(void) {
    if (systick_timeout(&led_green_timer, 300)) {
        Device_LED_Green_Toggle();
    }
    if (systick_timeout(&led_red_timer, 100)) {
        Device_LED_Red_Toggle();
    }
}
```

**设计原则**：App 层绝不包含任何芯片相关的头文件。如果哪天要把这个灯光逻辑跑在 Linux 的 `/sys/class/leds/` 上，App 层的代码一行不用改。

#### 2. Devices 层 —— 设备抽象

```c
// 用"配置单"描述硬件连接，不写寄存器
static GPIO_Config_t led_green_cfg = {
    .port = BSP_PORT_C,          // 用通用枚举，不是 GPIOC
    .pin  = BSP_PIN_13,
    .mode = BSP_GPIO_MODE_OUT_PP
};

void Device_LED_Init(void) {
    led_green = GPIO_Pin_Create(&led_green_cfg);  // 把配置单交给 BSP 工厂
}

// 操作接口是纯语义的
void Device_LED_Green_Toggle(void) { GPIO_Toggle(led_green); }
```

**设计原则**：Devices 层定义"这个板子上有什么设备"，但不碰底层寄存器。它通过 `GPIO_Pin*` 这个不透明句柄与 BSP 层交互 —— 就像一个遥控器，你只管按按钮，不需要知道信号怎么编码的。

#### 3. BSP 层 —— 芯片隔离墙

```c
// bsp_gpio.c —— 全村唯一包含 stm32f10x.h 的地方
#include "stm32f10x.h"  // ← 就这一处！

// 不透明指针：对外隐藏内部结构
struct GPIO_Pin {
    GPIO_TypeDef* port;   // 上层代码永远看不到这两个字段
    uint16_t      pin;
};

// 翻译函数：把通用枚举翻译成 STM32 方言
static void BSP_GPIO_Translate(BSP_Port_t bsp_port, BSP_Pin_t bsp_pin,
                               GPIO_TypeDef** out_port, uint16_t* out_pin,
                               uint32_t* out_rcc) {
    switch (bsp_port) {
        case BSP_PORT_A: *out_port = GPIOA; *out_rcc = RCC_APB2Periph_GPIOA; break;
        case BSP_PORT_B: *out_port = GPIOB; *out_rcc = RCC_APB2Periph_GPIOB; break;
        case BSP_PORT_C: *out_port = GPIOC; *out_rcc = RCC_APB2Periph_GPIOC; break;
        // ...
    }
    *out_pin = (uint16_t)(1 << bsp_pin);  // 利用规律直接位移，不写 16 个 case
}
```

**设计原则**：BSP 层是硬件依赖的"防火墙"。所有芯片相关的代码被围堵在这一层。换芯片时，只需要重写 BSP 层的 `.c` 文件，上层代码原封不动。

### 为什么要分这么多层？一个具体例子

假设需求变更："绿灯从 PC13 改到 PB0"。

| 架构 | 需要改的文件 |
|------|-------------|
| 传统写法 | 满世界找 `GPIOC` 和 `GPIO_Pin_13`，包括中断配置、初始化、业务逻辑里到处散落的寄存器操作 |
| 本架构 | **只改 `device_led.c` 里的一个数字**：`.pin = BSP_PIN_0` 改成 `.pin = BSP_PIN_13`，`.port = BSP_PORT_B` |

---

## 为什么要这种写法？

### 1. 不透明指针（Opaque Pointer）—— 强制封装

```c
// bsp_gpio.h —— 头文件里只有声明，没有定义
struct GPIO_Pin;  // 上层只知道"有这么个类型"，不知道里面有什么

// bsp_gpio.c —— 真正的定义藏在 .c 文件里
struct GPIO_Pin {
    GPIO_TypeDef* port;
    uint16_t      pin;
};
```

**为什么这样做？** 如果不透明指针，上层代码就可以写 `led->port->ODR ^= led->pin`。看起来很方便，但这就意味着上层代码依赖了 `stm32f10x.h` 的定义。换芯片时，所有这样写的代码都会编译报错。

不透明指针强制上层只能通过 `GPIO_Toggle(led)` 这样的接口操作 —— 物理上杜绝了绕过抽象层的可能。

### 2. 静态内存池 —— 不用 malloc

```c
struct GPIO_Pin* GPIO_Pin_Create(GPIO_Config_t* cfg) {
    static struct GPIO_Pin pool[16];   // 编译期确定的内存池
    static uint8_t pool_idx = 0;

    if (pool_idx >= 16) return 0;      // 池子满了就拒绝
    struct GPIO_Pin* obj = &pool[pool_idx++];
    // ... 初始化 ...
    return obj;
}
```

**为什么不用 `malloc`？** 在嵌入式系统里，`malloc` 有两个原罪：
- **内存碎片** —— 运行几个月后，堆上可能出现大量碎片，申请 12 字节失败导致系统崩溃
- **不确定性** —— `malloc` 的执行时间不固定，在实时系统里是隐患

静态池子的大小在编译时确定，分配时间 O(1)，永远不会碎片化。

### 3. C99 指定初始化器 —— 可读性

```c
// 传统写法：字段顺序必须和结构体定义一致，容易搞错
GPIO_Config_t cfg = { BSP_PORT_C, BSP_PIN_13, BSP_GPIO_MODE_OUT_PP };

// C99 指定初始化器：顺序无关，自文档化
static GPIO_Config_t led_green_cfg = {
    .port = BSP_PORT_C,
    .pin  = BSP_PIN_13,
    .mode = BSP_GPIO_MODE_OUT_PP
};
```

每行都有 `.field = value`，不需要查结构体定义就能理解含义，新增字段时也兼容已有代码。

### 4. 非阻塞超时检查器 —— 协作式多任务的核心

```c
bool systick_timeout(uint32_t *timestamp, uint32_t timeout_ms) {
    if (sys_tick_ms - *timestamp >= timeout_ms) {
        *timestamp = sys_tick_ms;  // 自动重置基准时间
        return true;
    }
    return false;
}
```

这个看似简单的函数是整个框架能够"并行"的关键：

```c
while (1) {
    // 这两个 if 看起来是顺序执行，但效果上是并行的
    if (systick_timeout(&t1, 300)) { /* 任务A */ }  // 300ms 周期
    if (systick_timeout(&t2, 100)) { /* 任务B */ }  // 100ms 周期
    if (systick_timeout(&t3, 50))  { /* 任务C */ }  // 50ms 周期
}
```

`uint32_t` 的自然溢出特性使得 `sys_tick_ms - *timestamp` 在计数器回绕时仍然正确（无符号整数的模运算保证）。这种"超级循环 + 非阻塞检查"的方式，在没有 RTOS 的情况下实现了多任务并发。

### 5. State_Keep_Timeout —— 按键消抖不用 Delay

```c
// 传统写法：阻塞 20ms 消抖（CPU 干等 20ms！）
if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
    Delay_ms(20);  // ← 这 20ms 什么都不能做
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) {
        // 确认按下
    }
}

// 本工程写法：非阻塞状态保持检测
static State_Timer_t key_timer = {0};
uint8_t key = GPIO_GetLevel(key_pin);  // 每次循环都读
if (State_Keep_Timeout(&key_timer, key, 20)) {
    // 状态已稳定保持 20ms，确认有效！
}
```

---

## 核心思想

本工程的架构思想可以用四个关键词概括：

### 1. 依赖倒置（Dependency Inversion）

```
高层模块（App）不应该依赖低层模块（BSP/Driver），两者都应该依赖抽象（接口）。

传统方式：  App → GPIO_ResetBits(GPIOC, ...)  ← 直接依赖芯片寄存器
本工程：    App → Device_LED_Toggle() → GPIO_Toggle(opaque_ptr) → 寄存器
                    ↑ 每一层只依赖上一层的接口，不穿透
```

**唯一包含 `stm32f10x.h` 的 `.c` 文件是 `bsp_gpio.c`**。这不是巧合，是刻意设计的结果。

### 2. 接口隔离（Interface Segregation）

每一层对外暴露的接口都是"最小化"的：

| 层 | 对外暴露的接口 |
|----|---------------|
| App | `App_Lighting_Init()` / `App_Lighting_Task()` |
| Devices | `Device_LED_Green_Toggle()` / `Device_LED_Red_On()` ... |
| BSP | `GPIO_Pin_Create()` / `GPIO_Toggle()` / `GPIO_SetHigh()` ... |
| System | `systick_timeout()` / `State_Keep_Timeout()` / `Delay_ms()` ... |

上层不需要知道下层的实现细节。这就是"最少知识原则"。

### 3. 单一职责（Single Responsibility）

- `delay.c` 只管时间，不管 GPIO
- `bsp_gpio.c` 只管 GPIO 引脚操作，不管什么设备在用
- `device_led.c` 只管 LED 设备，不管业务逻辑
- `app_lighting.c` 只管灯光闪烁策略，不管 LED 接在哪个引脚

每个文件有且仅有一个修改的理由。

### 4. 开闭原则（Open/Closed）

- **对扩展开放**：加新设备（如按键、传感器）只需新增文件，不修改已有代码
- **对修改封闭**：换芯片时，上层代码零修改

```c
// main.c 的 while(1) 里加新功能：
while (1) {
    App_Lighting_Task();       // 已有，不改
    App_WeightSensor_Task();   // 新增，加一行
    App_Key_Task();            // 再加一行
    // 未来加 20 个 Task，已有代码一行不动
}
```

---

## 架构亮点

### 🌟 亮点 1：不透明指针 + 工厂模式 = C 语言的"封装"

在不支持 `private` 关键字的 C 语言里，用**前向声明 + .c 文件内定义**实现了强制封装。这不是语法层面的约束，而是**物理层面的约束** —— 别的文件根本看不到结构体成员。

### 🌟 亮点 2：全村只有一个地方 #include 芯片头文件

```c
// 全工程搜索 stm32f10x.h 的引用位置：
// bsp_gpio.c     ← 就这里
// delay.c        ← 这里也有（因为需要读 SysTick 寄存器）
// stm32f10x_it.c ← 中断服务，不可避免
// stm32f10x_conf.h ← 标准库配置，独立文件
```

其他所有 .c 文件（`app_lighting.c`、`device_led.c`）都**不包含**任何芯片相关的头文件。这意味着它们可以在任何平台编译，这是一个非常强的可移植性声明。

### 🌟 亮点 3：配置与代码分离

```c
// device_led.c —— 硬件配置集中在文件顶部的静态结构体里
static GPIO_Config_t led_green_cfg = {
    .port = BSP_PORT_C,
    .pin  = BSP_PIN_13,
    .mode = BSP_GPIO_MODE_OUT_PP
};
```

需要改硬件接线？打开 `device_led.c`，翻到最前面，改一个数字。不用在 500 行代码里大海捞针。

### 🌟 亮点 4：翻译层用"位移规律"替代 N 个 case

```c
// 传统翻译写法：
switch (bsp_pin) {
    case BSP_PIN_0:  *out_pin = GPIO_Pin_0;  break;  // 0x0001
    case BSP_PIN_1:  *out_pin = GPIO_Pin_1;  break;  // 0x0002
    // ... 写 16 个 case
}

// 本工程的写法：利用 STM32 GPIO_Pin_X 宏等于 (1<<X) 的规律
*out_pin = (uint16_t)(1 << bsp_pin);  // 一行搞定
```

这建立在一个可靠的前提上：`BSP_PIN_0 = 0, BSP_PIN_1 = 1, ...` 且 `GPIO_Pin_0 = 0x0001, GPIO_Pin_1 = 0x0002, ...`（即 `1 << n`）。但如果 ST 哪天改了宏定义（虽然概率极低），这里会出问题 —— 不依赖这个规律的话，就需要老老实实写 16 个 case。

### 🌟 亮点 5：SysTick 作为全系统的时间引擎

```
SysTick 硬件定时器 (1ms)
    │
    ▼
SysTick_Handler() → Delay_IncTick()  → sys_tick_ms++
    │                                       │
    │                                       ├── systick_timeout()   非阻塞超时
    │                                       ├── State_Keep_Timeout() 状态保持
    │                                       ├── Delay_ms()           阻塞延时（调试用）
    │                                       └── Get_SysTick()        获取全局时间
    │
    ▼
所有 App 层 Task 共用同一个时间基准
```

一个硬件定时器，支撑整个系统的所有时间需求。这就是"单一时间源"设计。

### 🌟 亮点 6：main.c 作为组装入口，清晰如目录

```c
int main(void) {
    // 1. 全局系统与内核级服务初始化
    Delay_Init();

    // 2. 各个业务组件初始化
    App_Lighting_Init();

    // 3. 主调度轮询
    while (1) {
        App_Lighting_Task();
        // App_WeightSensor_Task();  ← 未来扩展
    }
}
```

`main.c` 不需要理解任何业务细节，它只是一个**组装清单**。这种写法让 `main.c` 天然成为系统的"目录页"。

---

## 移植到其他芯片

### 移植步骤

假设从 STM32F103 移植到 **GD32F103**（引脚兼容的国产替代）：

#### 第一步：替换底层驱动

```
Driver/FWLIB/  →  替换为 GD32 的标准库
CMSIS/         →  替换为 GD32 的启动文件和内核文件
System/        →  system_gd32f10x.c 替换（主要是时钟树初始化）
```

#### 第二步：修改 BSP 翻译层（唯一需要改代码的地方）

```c
// bsp_gpio.c —— 只需要改翻译函数和构造函数里的初始化代码

// 1. 替换头文件
// #include "stm32f10x.h"  →  #include "gd32f10x.h"

// 2. 修改端口翻译（GD32 的 GPIO 基地址和时钟宏可能不同）
static void BSP_GPIO_Translate(BSP_Port_t bsp_port, BSP_Pin_t bsp_pin,
                               GPIO_TypeDef** out_port, uint16_t* out_pin,
                               uint32_t* out_rcc) {
    switch (bsp_port) {
        case BSP_PORT_A:
            *out_port = GPIOA;           // GD32 的宏名如果不同就改这里
            *out_rcc = RCU_GPIOA;        // GD32 把 RCC 改成了 RCU
            break;
        // ...
    }
    *out_pin = (uint16_t)(1 << bsp_pin); // 如果 GD32 的 GPIO_PIN_X 宏不同，改这里
}

// 3. 修改 GPIO 初始化方式（库函数调用可能不同）
// RCC_APB2PeriphClockCmd() → rcu_periph_clock_enable()
// GPIO_Init() 参数结构体可能微调
```

#### 第三步：验证编译

```c
// 这些文件的代码一行不动：
App/app_lighting.c    ← 业务逻辑，完全不依赖芯片
App/app_lighting.h    ← 接口声明
Devices/device_led.c  ← 设备抽象，只用 BSP 通用接口
Devices/device_led.h  ← 接口声明
BSP/bsp_gpio.h        ← 接口声明（除非引脚数不同需要扩展枚举）
```

### 移植的本质：寻找"隔离面"

| 隔离面以上（不变） | 隔离面以下（需要替换） |
|-------------------|----------------------|
| App 层所有文件 | Driver/FWLIB 驱动库 |
| Devices 层所有文件 | CMSIS 启动文件 |
| BSP/bsp_gpio.h 接口声明 | System 时钟配置 |
| System/delay.h 接口声明 | BSP/bsp_gpio.c 实现 |

> **核心原则**：头文件的接口声明尽量保持稳定，`.c` 文件的实现可以替换。上层代码通过 `.h` 文件认知世界，不需要知道 `.c` 里发生了什么。

### 移植到完全不同的芯片（比如 STM32 → ESP32）

如果换到架构完全不同的芯片（Cortex-M → Xtensa），BSP 层需要重写，但上层逻辑依然不动。`device_led.c` 里的 `GPIO_Config_t` 和 `GPIO_Pin*` 接口不变，只是底下的实现从寄存器操作变成了 ESP-IDF 的 `gpio_config()` 调用。

```c
// BSP 接口不变，实现可以完全不同
void GPIO_Toggle(struct GPIO_Pin* pin) {
    if (pin) {
        // STM32: pin->port->ODR ^= pin->pin;
        // ESP32: gpio_set_level(pin->pin_num, !gpio_get_level(pin->pin_num));
    }
}
```

---

## 需要特别注意的地方

### ⚠️ 1. `systick_timeout` 的 uint32_t 回绕问题

```c
// sys_tick_ms 是一个 uint32_t，每 1ms +1
// 49.7 天后会回绕到 0

bool systick_timeout(uint32_t *timestamp, uint32_t timeout_ms) {
    if (sys_tick_ms - *timestamp >= timeout_ms) {  // 无符号减法天然防回绕
        *timestamp = sys_tick_ms;
        return true;
    }
    return false;
}
```

得益于无符号整数的模运算特性，这个实现**天然正确处理回绕**。但前提是 `timeout_ms` 不能超过 `UINT32_MAX / 2`（约 24.8 天），否则可能误判。对于毫秒级超时（通常 < 60000），这不是问题。

### ⚠️ 2. 静态内存池的大小限制

```c
static struct GPIO_Pin pool[16];  // 最多支持 16 个 GPIO 对象
```

如果你的板子上有超过 16 个需要抽象为独立对象的 GPIO 引脚，需要扩大池子。这是一个**编译期决策**，不能运行时动态扩展。建议在项目初期估算好最大值，留足余量。

### ⚠️ 3. 无抢占式调度 —— 单个 Task 不能耗时过长

```c
while (1) {
    App_Lighting_Task();       // 如果这里执行了 500ms...
    App_Sensor_Task();         // 这里就要等 500ms 后才能执行
}
```

这是协作式多任务，不是抢占式的 RTOS。每个 Task 必须在极短时间内完成并返回。如果某个任务需要做耗时操作（如复杂滤波计算），应该：
- 用状态机拆分长任务
- 或者在主循环里分步执行，每次只处理一小块数据

### ⚠️ 4. 不透明指针的 NULL 检查

```c
void GPIO_Toggle(struct GPIO_Pin* pin) {
    if (pin) pin->port->ODR ^= pin->pin;  // 必须判空！
}
```

因为上层代码持有的只是指针，如果 `GPIO_Pin_Create` 失败返回 NULL，后续操作会野指针崩溃。所有操作接口都需要判空。这是用 C 语言模拟对象的代价 —— C++ 里这由 `this` 隐式检查或引用语义保证。

### ⚠️ 5. BSP 翻译层的位运算依赖于宏定义规律

```c
*out_pin = (uint16_t)(1 << bsp_pin);
```

这依赖于：
1. `BSP_Pin_t` 枚举值是连续的 `0, 1, 2, ... 15`
2. 底层 `GPIO_Pin_X` 宏等于 `(uint16_t)0x0001 << X`

如果芯片厂商的宏定义不满足条件（比如某些芯片的 `GPIO_Pin_15` 不是 `0x8000`），这里就会出错。**保守做法**：如果不确定，可以写完整的 16 个 case 映射，牺牲一点简洁性换取绝对安全。

### ⚠️ 6. 全局变量的初始化顺序

```c
static struct GPIO_Pin* led_green = 0;  // 静态初始化为 NULL
```

C 语言保证静态变量在 `main()` 之前被初始化为 0/NULL。但如果某个模块的初始化依赖另一个模块，必须在 `main()` 里手动保证顺序：

```c
Delay_Init();           // 1. 先初始化时间系统
App_Lighting_Init();    // 2. 再初始化设备（可能用到 delay）
```

### ⚠️ 7. Volatile 的必要性

```c
static volatile uint32_t sys_tick_ms = 0;
```

`sys_tick_ms` 在中断 `SysTick_Handler` 中修改，在主循环中读取。没有 `volatile`，编译器可能把 `sys_tick_ms` 缓存到寄存器里，导致主循环永远看不到中断更新的值。**跨执行上下文（中断 vs 主循环）共享的变量必须加 volatile。**

---

## 工程结构

```
STM32_OOP_Project/
├── App/                        ← 业务逻辑层（最高层）
│   ├── app_lighting.h          │  灯光控制接口
│   └── app_lighting.c          │  灯光控制实现（非阻塞闪烁策略）
│
├── Devices/                    ← 设备抽象层
│   ├── device_led.h            │  LED 设备接口
│   └── device_led.c            │  LED 设备实现（配置 + 操作映射）
│
├── BSP/                        ← 板级支持包（硬件隔离墙）
│   ├── bsp_gpio.h              │  GPIO 抽象接口
│   └── bsp_gpio.c              │  GPIO 实现（全村唯一的 stm32f10x.h include）
│
├── System/                     ← 系统服务层
│   ├── delay.h                 │  时间服务接口
│   ├── delay.c                 │  时间服务实现（SysTick 驱动）
│   ├── stm32f10x_conf.h        │  标准外设库配置
│   ├── stm32f10x_it.h          │  中断服务声明
│   └── stm32f10x_it.c          │  中断服务实现
│
├── CMSIS/                      ← ARM Cortex-M 内核接口
│   ├── core_cm3.c/h            │  Cortex-M3 内核访问
│   ├── startup_stm32f10x_md.s  │  启动文件（汇编，向量表）
│   ├── system_stm32f10x.c/h    │  系统时钟初始化
│   └── stm32f10x.h             │  芯片寄存器定义
│
├── Driver/FWLIB/               ← STM32 标准外设库
│   ├── inc/                    │  头文件（GPIO/RCC/USART/...）
│   └── src/                    │  源文件
│
├── Project/                    ← Keil MDK 工程文件
│   ├── Project.uvprojx         │  工程文件
│   └── Project.uvoptx          │  工程选项
│
├── .vscode/                    ← VS Code 配置
│   └── c_cpp_properties.json   │  IntelliSense 配置
│
└── .gitignore                  ← Git 忽略规则
```

### 文件依赖关系（include 图）

```
app_lighting.c ──→ device_led.h ──→ bsp_gpio.h
       │                                    │
       └────────── delay.h ─────────────────┘
                       │
                       └── stm32f10x.h (芯片寄存器定义)
                                │
              bsp_gpio.c ───────┘ (唯一包含芯片头文件的实现文件)
```

注意 **`device_led.c` 不包含 `stm32f10x.h`**，它的 include 链是：
```
device_led.c → bsp_gpio.h → stdint.h（仅标准类型，无芯片依赖）
```

---

## 数据流全景图

```
┌─────────────────────────────────────────────────────────────────┐
│                        SysTick 硬件定时器                        │
│                    每 1ms 触发一次中断                            │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
              SysTick_Handler()         ← stm32f10x_it.c
                         │
                         ▼
                   Delay_IncTick()      ← delay.c
                    sys_tick_ms++
                         │
           ┌─────────────┼─────────────┐
           ▼             ▼             ▼
     systick_timeout  Delay_ms   State_Keep_Timeout
      (非阻塞超时)    (阻塞延时)    (状态保持检测)
           │
           ▼
    ┌──────────────────────────────────────────┐
    │              main() 超级循环               │
    │                                           │
    │  while (1) {                              │
    │      App_Lighting_Task()  ←────────────┐  │
    │      // 未来: Task2, Task3...           │  │
    │  }                                     │  │
    └──────────────────────────────────────────┘  │
                                                  │
    ┌─────────────────────────────────────────────┘
    │
    ▼
App_Lighting_Task()                        ← app_lighting.c
    │  if (systick_timeout(&t, 300))
    │
    ▼
Device_LED_Green_Toggle()                  ← device_led.c
    │  GPIO_Toggle(led_green)
    │
    ▼
GPIO_Toggle(struct GPIO_Pin* pin)          ← bsp_gpio.c
    │  pin->port->ODR ^= pin->pin
    │
    ▼
┌──────────────────────┐
│   GPIO 硬件寄存器     │
│   PC13 引脚电平翻转   │
└──────────────────────┘
```

从顶层的"业务意图"到底层的"电平翻转"，每一层的转换都是**单向、不可跳过的**。这就是分层架构的核心价值。

---

## 总结

这个工程回答了嵌入式开发中一个根本性问题：

> 当项目从"跑通 Demo"进入"长期维护"阶段后，代码应该长什么样？

答案不是更聪明的寄存器操作技巧，不是更短的行数，甚至不是更高的性能 —— 而是**更清晰的边界**。

- **App 和 Devices 的边界**：业务逻辑 vs 设备能力
- **Devices 和 BSP 的边界**：设备抽象 vs 芯片操作
- **BSP 和 Driver 的边界**：通用接口 vs 具体芯片

把边界画清楚，一个点灯工程就是一个可以无限扩展的系统框架。

---

> 📄 **License**: This documentation is part of the STM32 OOP Project.
> 🔗 **Related**: See commit history for architecture evolution details.
