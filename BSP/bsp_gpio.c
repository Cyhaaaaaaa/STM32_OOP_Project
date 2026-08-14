#include "bsp_gpio.h"
#include "stm32f10x.h" // 全村唯一包含芯片库的地方

// 不透明指针的真正实体：只保存真实的寄存器地址和引脚宏
// 这个结构体对外部是完全不透明的，外部无法访问其成员，只能通过BSP提供的接口来操作
//但是要在.h文件中声明，因为我们的构造函数要返回一个GPIO_Pin类型的指针，而这个结构体在.h文件中声明
struct GPIO_Pin {
    GPIO_TypeDef* port;
    uint16_t      pin;
};
// 专业解释：.h 里只有 `struct GPIO_Pin;` 这个"名字"，真正的成员定义在这里（.c 文件内部）。
// 于是只有本文件知道它内部存的是 STM32 的 GPIO_TypeDef* 端口基地址和引脚宏；
// 别的文件连 sizeof(struct GPIO_Pin) 都算不出来，彻底实现信息隐藏。

// 内部私有翻译函数：把“通用枚举”翻译成“STM32方言”
//这个状态机函数的作用是把BSP层的通用枚举类型（BSP_Port_t和BSP_Pin_t）翻译成STM32方言的GPIO_TypeDef*和uint16_t类型的引脚宏，以及对应的RCC时钟宏
//传入的第一个参数为BSP_Port_t的枚举，第二个参数为BSP_Pin_t的枚举，第三个参数是一个指向GPIO_TypeDef*的指针，用来存储翻译后的GPIO_TypeDef*地址，
//第四个参数是一个指向uint16_t的指针，用来存储翻译后的引脚宏，第五个参数是一个指向uint32_t的指针，用来存储翻译后的RCC时钟宏
static void BSP_GPIO_Translate(BSP_Port_t bsp_port, BSP_Pin_t bsp_pin,
                               GPIO_TypeDef** out_port, uint16_t* out_pin, uint32_t* out_rcc) {
// 专业解释：
// 1. static 修饰符：这个函数只在当前 .c 文件内可见（文件作用域），不会被外部调用，
//    是 C 里"私有函数"的标准写法——相当于 OOP 里的 private 方法。
// 2. 返回多个结果：C 函数只能 return 一个值，要"输出"多个结果就得靠「指针参数」。
//    调用方把变量的地址传进来（&obj->port 这种），函数往地址里写值，调用方就拿到了。
//    这里 out_port 是二级指针（GPIO_TypeDef**），因为它要修改的是"一个指针变量"本身。

    // 1. 翻译 Port 和时钟 (不同芯片家族的时钟总线可能不同，这里是隔离差异的好地方)
    switch (bsp_port) {
        case BSP_PORT_A: 
            *out_port = GPIOA; 
            *out_rcc = RCC_APB2Periph_GPIOA; 
            break;
        case BSP_PORT_B: 
            *out_port = GPIOB; 
            *out_rcc = RCC_APB2Periph_GPIOB; 
            break;
        case BSP_PORT_C: 
            *out_port = GPIOC; 
            *out_rcc = RCC_APB2Periph_GPIOC; 
            break;
        default:
            // 严谨的做法：可以加个断言 assert_param
            break;
    }
    
    // 2. 翻译 Pin：利用底层规律直接位移，极致高效
    *out_pin = (uint16_t)(1 << bsp_pin);
}
// 专业解释：引脚号 0~15 在 STM32 里正好对应位掩码 0x0001~0x8000（即 1 左移 N 位）。
// 比如 PIN13 → 1<<13 = 0x2000，正是 GPIO_Pin_13 的值。所以不用再写 16 个 case，
// 一行位运算就完成了枚举→位掩码的映射——既简洁又零查表开销。

// 构造函数
struct GPIO_Pin* GPIO_Pin_Create(GPIO_Config_t* cfg) {
    // 简单的静态内存池，避免使用 malloc
    static struct GPIO_Pin pool[16];
    static uint8_t pool_idx = 0;
    // 专业解释：
    // 1. 静态内存池：static 局部变量只在第一次进入函数时初始化一次，之后函数退出也不销毁，
    //    所以能"记住"已经分配了几个对象。这是嵌入式里替代 malloc 的经典做法——
    //    内存占用在编译期就固定死了（16 个对象），不会出现动态分配带来的碎片和失败风险。
    // 2. pool_idx++：每创建一次就往后挪一格，相当于从池子里"领走"一个空位。

    // 如果超出了池子大小，直接返回 NULL (或者做断言保护)
    if (pool_idx >= 16) return 0;
    // 专业解释：越界保护。返回 0（即 NULL）让上层知道"对象创建失败"，上层可以据此处理，
    // 避免继续使用无效指针导致跑飞（HardFault）。

    struct GPIO_Pin* obj = &pool[pool_idx++];
    uint32_t rcc_clock = 0;
    
    // 获取真实的底层硬件参数
    BSP_GPIO_Translate(cfg->port, cfg->pin, &obj->port, &obj->pin, &rcc_clock);

    // 开启时钟
    RCC_APB2PeriphClockCmd(rcc_clock, ENABLE);

    // 配置底层结构体
    GPIO_InitTypeDef gi;
    gi.GPIO_Pin = obj->pin;
    gi.GPIO_Speed = GPIO_Speed_50MHz;
    // 专业解释：GPIO_InitTypeDef 是 STM32 标准库用来描述引脚属性的结构体。
    // 把翻译后的 pin、以及"速度"填充进去，再交给 GPIO_Init 一次性配置。

    switch(cfg->mode){
        case BSP_GPIO_MODE_OUT_PP:
            gi.GPIO_Mode = GPIO_Mode_Out_PP;
            break;
        case BSP_GPIO_MODE_IN_PULLUP:
            gi.GPIO_Mode = GPIO_Mode_IPU;
            break;
        case BSP_GPIO_MODE_IN_FLOATING:
            gi.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            break;
    }
    // 专业解释：又是一次「通用枚举 → STM32 方言」的翻译，把 BSP 的工作模式映射到标准库宏。
    // 推挽输出(Out_PP)=能主动输出高低电平；上拉输入(IPU)=输入且内部上拉；浮空输入=输入无上下拉。

    GPIO_Init(obj->port, &gi);

    // 默认输出高电平，防止外设在上电瞬间乱动
    if(cfg->mode == BSP_GPIO_MODE_OUT_PP) {
        GPIO_SetBits(obj->port, obj->pin);
    }
    // 专业解释：很多外设（如 LED 负极接 GPIO）默认状态由引脚电平决定，上电初始化瞬间若输出低电平
    // 可能误动作。这里先拉高到一个安全默认态，再交给上层按需控制。

    return obj;
}

// ---------------- 操作接口 ----------------

void GPIO_SetHigh(struct GPIO_Pin* pin) {
    if (pin) GPIO_SetBits(pin->port, pin->pin);
}

void GPIO_SetLow(struct GPIO_Pin* pin) {
    if (pin) GPIO_ResetBits(pin->port, pin->pin);
}

void GPIO_Toggle(struct GPIO_Pin* pin) {
    if (pin) pin->port->ODR ^= pin->pin; // 直接操作寄存器最高效
    // 专业解释：^= 是「异或赋值」。ODR 是输出数据寄存器，pin->pin 是对应引脚的位掩码。
    // 异或的性质：某一位是 0 会翻成 1，是 1 会翻成 0，即"翻转"；其它位因为异或 0 而保持不变。
    // 直接操作寄存器省去了库函数的调用开销，是追求效率时的惯用写法。
}

uint8_t GPIO_GetLevel(struct GPIO_Pin* pin) {
    if (!pin) return 0;
    return (pin->port->IDR & pin->pin) ? 1 : 0;
    // 专业解释：IDR 是输入数据寄存器，&（按位与）只保留目标引脚那一比特；
    // 若非零说明该脚当前是高电平，返回 1；否则返回 0。这样把"读寄存器"包装成"读 0/1"。
}

