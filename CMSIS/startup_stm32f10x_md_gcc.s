@*******************************************************************************
@* File Name          : startup_stm32f10x_md_gcc.s
@* Author             : MCD Application Team (GCC adaptation)
@* Version            : V3.5.1
@* Description        : STM32F10x Medium Density Devices vector table for GCC.
@*                      This module performs:
@*                      - Set the initial SP
@*                      - Set the initial PC == Reset_Handler
@*                      - Set the vector table entries with the exceptions ISR
@*                      - Configure the clock system (SystemInit)
@*                      - Branches to main
@*
@*                      Target: STM32F103C8T6 (Medium Density)
@*                      Toolchain: arm-none-eabi-gcc (GNU Assembler syntax)
@*
@*                      This file is a separate GCC port and does NOT modify
@*                      the original Keil startup_stm32f10x_md.s.
@*
@* Copyright (c) 2011 STMicroelectronics.
@* All rights reserved.
@*******************************************************************************

    .syntax     unified
    .cpu        cortex-m3
    .fpu        softvfp
    .thumb

    .global     g_pfnVectors
    .global     Default_Handler

@ ========== Stack & Heap sizes ==========
.equ  _Stack_Size,  0x00000400
.equ  _Heap_Size,   0x00000200

@ ========== Stack section ==========
.section .stack, "aw", %nobits
    .balign  8
_Stack_Begin:
    .space   _Stack_Size
__initial_sp:
_estack:

@ ========== Heap section ==========
.section .heap, "aw", %nobits
    .balign  8
__heap_base:
    .space   _Heap_Size
__heap_limit:

@ ========== Vector Table ==========
.section .isr_vector, "a", %progbits
    .type     g_pfnVectors, %object
    .size     g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word    _estack
    .word    Reset_Handler
    .word    NMI_Handler
    .word    HardFault_Handler
    .word    MemManage_Handler
    .word    BusFault_Handler
    .word    UsageFault_Handler
    .word    0
    .word    0
    .word    0
    .word    0
    .word    SVC_Handler
    .word    DebugMon_Handler
    .word    0
    .word    PendSV_Handler
    .word    SysTick_Handler

    @ External Interrupts
    .word    WWDG_IRQHandler
    .word    PVD_IRQHandler
    .word    TAMPER_IRQHandler
    .word    RTC_IRQHandler
    .word    FLASH_IRQHandler
    .word    RCC_IRQHandler
    .word    EXTI0_IRQHandler
    .word    EXTI1_IRQHandler
    .word    EXTI2_IRQHandler
    .word    EXTI3_IRQHandler
    .word    EXTI4_IRQHandler
    .word    DMA1_Channel1_IRQHandler
    .word    DMA1_Channel2_IRQHandler
    .word    DMA1_Channel3_IRQHandler
    .word    DMA1_Channel4_IRQHandler
    .word    DMA1_Channel5_IRQHandler
    .word    DMA1_Channel6_IRQHandler
    .word    DMA1_Channel7_IRQHandler
    .word    ADC1_2_IRQHandler
    .word    USB_HP_CAN1_TX_IRQHandler
    .word    USB_LP_CAN1_RX0_IRQHandler
    .word    CAN1_RX1_IRQHandler
    .word    CAN1_SCE_IRQHandler
    .word    EXTI9_5_IRQHandler
    .word    TIM1_BRK_IRQHandler
    .word    TIM1_UP_IRQHandler
    .word    TIM1_TRG_COM_IRQHandler
    .word    TIM1_CC_IRQHandler
    .word    TIM2_IRQHandler
    .word    TIM3_IRQHandler
    .word    TIM4_IRQHandler
    .word    I2C1_EV_IRQHandler
    .word    I2C1_ER_IRQHandler
    .word    I2C2_EV_IRQHandler
    .word    I2C2_ER_IRQHandler
    .word    SPI1_IRQHandler
    .word    SPI2_IRQHandler
    .word    USART1_IRQHandler
    .word    USART2_IRQHandler
    .word    USART3_IRQHandler
    .word    EXTI15_10_IRQHandler
    .word    RTCAlarm_IRQHandler
    .word    USBWakeUp_IRQHandler

@ ========== Reset Handler ==========
    .section .text.Reset_Handler, "ax", %progbits
    .weak    Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:
    @ Set stack pointer to top of stack
    ldr    r0, =_estack
    msr    msp, r0

    @ Call SystemInit (clock configuration etc.)
    bl     SystemInit

    @ Copy .data section from FLASH to SRAM
    ldr    r1, =_sdata
    ldr    r2, =_edata
    ldr    r3, =_sidata
    b      .L_copy_data_done
.L_copy_data_loop:
    ldr    r4, [r3], #4
    str    r4, [r1], #4
.L_copy_data_done:
    cmp    r1, r2
    blt    .L_copy_data_loop

    @ Zero-fill .bss section
    movs   r0, #0
    ldr    r1, =_sbss
    ldr    r2, =_ebss
    b      .L_fill_bss_done
.L_fill_bss_loop:
    str    r0, [r1], #4
.L_fill_bss_done:
    cmp    r1, r2
    blt    .L_fill_bss_loop

    @ Call C++ static constructors
    bl     __libc_init_array

    @ Branch to main
    bl     main

    @ Infinite loop (should not reach here)
    b      .

    .size   Reset_Handler, .-Reset_Handler

@ ========== Default Handler (weak, infinite loop) ==========
    .type   Default_Handler, %function
Default_Handler:
    b       .
    .size   Default_Handler, .-Default_Handler

@ ========== Weak alias macro ==========
.macro  def_irq_handler  name
    .weak   \name
    .thumb_set \name, Default_Handler
.endm

    def_irq_handler  NMI_Handler
    def_irq_handler  HardFault_Handler
    def_irq_handler  MemManage_Handler
    def_irq_handler  BusFault_Handler
    def_irq_handler  UsageFault_Handler
    def_irq_handler  SVC_Handler
    def_irq_handler  DebugMon_Handler
    def_irq_handler  PendSV_Handler
    def_irq_handler  SysTick_Handler

    def_irq_handler  WWDG_IRQHandler
    def_irq_handler  PVD_IRQHandler
    def_irq_handler  TAMPER_IRQHandler
    def_irq_handler  RTC_IRQHandler
    def_irq_handler  FLASH_IRQHandler
    def_irq_handler  RCC_IRQHandler
    def_irq_handler  EXTI0_IRQHandler
    def_irq_handler  EXTI1_IRQHandler
    def_irq_handler  EXTI2_IRQHandler
    def_irq_handler  EXTI3_IRQHandler
    def_irq_handler  EXTI4_IRQHandler
    def_irq_handler  DMA1_Channel1_IRQHandler
    def_irq_handler  DMA1_Channel2_IRQHandler
    def_irq_handler  DMA1_Channel3_IRQHandler
    def_irq_handler  DMA1_Channel4_IRQHandler
    def_irq_handler  DMA1_Channel5_IRQHandler
    def_irq_handler  DMA1_Channel6_IRQHandler
    def_irq_handler  DMA1_Channel7_IRQHandler
    def_irq_handler  ADC1_2_IRQHandler
    def_irq_handler  USB_HP_CAN1_TX_IRQHandler
    def_irq_handler  USB_LP_CAN1_RX0_IRQHandler
    def_irq_handler  CAN1_RX1_IRQHandler
    def_irq_handler  CAN1_SCE_IRQHandler
    def_irq_handler  EXTI9_5_IRQHandler
    def_irq_handler  TIM1_BRK_IRQHandler
    def_irq_handler  TIM1_UP_IRQHandler
    def_irq_handler  TIM1_TRG_COM_IRQHandler
    def_irq_handler  TIM1_CC_IRQHandler
    def_irq_handler  TIM2_IRQHandler
    def_irq_handler  TIM3_IRQHandler
    def_irq_handler  TIM4_IRQHandler
    def_irq_handler  I2C1_EV_IRQHandler
    def_irq_handler  I2C1_ER_IRQHandler
    def_irq_handler  I2C2_EV_IRQHandler
    def_irq_handler  I2C2_ER_IRQHandler
    def_irq_handler  SPI1_IRQHandler
    def_irq_handler  SPI2_IRQHandler
    def_irq_handler  USART1_IRQHandler
    def_irq_handler  USART2_IRQHandler
    def_irq_handler  USART3_IRQHandler
    def_irq_handler  EXTI15_10_IRQHandler
    def_irq_handler  RTCAlarm_IRQHandler
    def_irq_handler  USBWakeUp_IRQHandler

    .end
