# =============================================================================
#  Makefile — GCC 交叉编译 STM32F103C8T6 (Medium Density)
# =============================================================================
#  用法:
#    make          — 编译并链接工程
#    make clean    — 清除所有编译产物
#    make rebuild  — clean + 重新编译
#    make flash    — （预留）烧录到芯片，需安装 OpenOCD
#
#  注意:
#    - 本 Makefile 仅用于 arm-none-eabi-gcc 工具链
#    - Keil MDK 工程（Project/Project.uvprojx）完全不受影响
#    - 所有构建产物位于 build/ 目录下
# =============================================================================

# ---- 项目名称 ----
TARGET     = STM32_OOP

# ---- 工具链 ----
PREFIX     = arm-none-eabi
CC         = $(PREFIX)-gcc
AS         = $(PREFIX)-gcc   # 使用 gcc 汇编（预处理能力）
LD         = $(PREFIX)-gcc   # 使用 gcc 做链接（自动传入 lib 路径）
OBJCOPY    = $(PREFIX)-objcopy
OBJDUMP    = $(PREFIX)-objdump
SIZE       = $(PREFIX)-size
GDB        = $(PREFIX)-gdb

# ---- 目录 ----
BUILD_DIR  = build
SRC_DIRS   = .  # 所有路径都在 Makefile 里显式写出，不使用通配符

# ---- MCU 目标 ----
MCU_ARCH   = cortex-m3
MCU_FLAGS  = -mcpu=$(MCU_ARCH) -mthumb -mfloat-abi=soft

# ---- 编译/链接标志 ----
OPT        = -Og
SPECS      = --specs=nano.specs --specs=nosys.specs

CFLAGS    += $(MCU_FLAGS)
CFLAGS    += $(OPT)
CFLAGS    += -Wall -Wextra -Werror=implicit-function-declaration
CFLAGS    += -ffunction-sections -fdata-sections
CFLAGS    += -fno-common
CFLAGS    += -std=c99
CFLAGS    += $(SPECS)

# 调试信息
CFLAGS    += -g3 -gdwarf-4

# 宏定义
DEFINES   := -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD
CFLAGS    += $(DEFINES)

# 头文件路径
INCLUDES  := -IApp
INCLUDES  += -IBSP
INCLUDES  += -ICMSIS
INCLUDES  += -IDevices
INCLUDES  += -IDriver/FWLIB/inc
INCLUDES  += -ISystem
CFLAGS    += $(INCLUDES)

# ---- 链接标志 ----
LDFLAGS   := $(MCU_FLAGS)
LDFLAGS   += $(OPT)
LDFLAGS   += $(SPECS)
LDFLAGS   += -Wl,--gc-sections
LDFLAGS   += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS   += -Wl,--print-memory-usage
LDFLAGS   += -T CMSIS/STM32F103_FLASH.ld

# ---- 源文件列表 ----
# C 源文件
C_SOURCES  = \
    App/main.c                        \
    App/app_lighting.c                \
    BSP/bsp_gpio.c                    \
    BSP/bsp_uart.c                    \
    CMSIS/core_cm3.c                  \
    CMSIS/system_stm32f10x.c          \
    Devices/device_led.c              \
    System/delay.c                    \
    System/stm32f10x_it.c             \
    Driver/FWLIB/src/misc.c           \
    Driver/FWLIB/src/stm32f10x_adc.c  \
    Driver/FWLIB/src/stm32f10x_bkp.c  \
    Driver/FWLIB/src/stm32f10x_can.c  \
    Driver/FWLIB/src/stm32f10x_cec.c  \
    Driver/FWLIB/src/stm32f10x_crc.c  \
    Driver/FWLIB/src/stm32f10x_dac.c  \
    Driver/FWLIB/src/stm32f10x_dbgmcu.c \
    Driver/FWLIB/src/stm32f10x_dma.c  \
    Driver/FWLIB/src/stm32f10x_exti.c \
    Driver/FWLIB/src/stm32f10x_flash.c \
    Driver/FWLIB/src/stm32f10x_fsmc.c \
    Driver/FWLIB/src/stm32f10x_gpio.c \
    Driver/FWLIB/src/stm32f10x_i2c.c  \
    Driver/FWLIB/src/stm32f10x_iwdg.c \
    Driver/FWLIB/src/stm32f10x_pwr.c  \
    Driver/FWLIB/src/stm32f10x_rcc.c  \
    Driver/FWLIB/src/stm32f10x_rtc.c  \
    Driver/FWLIB/src/stm32f10x_sdio.c \
    Driver/FWLIB/src/stm32f10x_spi.c  \
    Driver/FWLIB/src/stm32f10x_tim.c  \
    Driver/FWLIB/src/stm32f10x_usart.c \
    Driver/FWLIB/src/stm32f10x_wwdg.c

# 汇编源文件（GCC 语法）
ASM_SOURCES = \
    CMSIS/startup_stm32f10x_md_gcc.s

# ---- 自动推导目标文件 ----
C_OBJS     = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
ASM_OBJS   = $(addprefix $(BUILD_DIR)/, $(ASM_SOURCES:.s=.o))
ALL_OBJS   = $(C_OBJS) $(ASM_OBJS)

# 依赖文件（用于头文件变更时自动重编译）
DEPS       = $(ALL_OBJS:.o=.d)

# ---- 主目标 ----
.PHONY: all clean rebuild

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

# ---- 链接 ----
$(BUILD_DIR)/$(TARGET).elf: $(ALL_OBJS) CMSIS/STM32F103_FLASH.ld
	@echo "=== 链接 $@ ==="
	$(LD) $(LDFLAGS) $(ALL_OBJS) -o $@
	@$(SIZE) $@
	@echo "=== 链接完成 ==="

# ---- 生成二进制文件 ----
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# ---- 编译 C 源文件 ----
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# ---- 编译汇编源文件 ----
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -x assembler-with-cpp -MMD -MP -c $< -o $@

# ---- 清理 ----
clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

# ---- 包含自动依赖 ----
-include $(DEPS)

# ---- 打印帮助信息 ----
help:
	@echo "STM32_OOP_Project — GCC 构建系统"
	@echo ""
	@echo "目标:"
	@echo "  make          = 编译 + 链接，生成 .elf / .bin / .hex"
	@echo "  make clean    = 删除 build/"
	@echo "  make rebuild  = clean + 重新编译"
	@echo "  make help     = 显示此帮助"
	@echo ""
	@echo "输出文件:"
	@echo "  build/$(TARGET).elf"
	@echo "  build/$(TARGET).bin"
	@echo "  build/$(TARGET).hex"
	@echo "  build/$(TARGET).map"
	@echo ""
	@echo "工具链: $(PREFIX)-gcc"
	@echo "MCU:    $(MCU_ARCH) (STM32F103C8T6 - Medium Density)"

# ---- 打印版本信息 ----
version:
	@$(CC) --version
