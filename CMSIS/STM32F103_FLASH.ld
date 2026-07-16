/*****************************************************************************
 * STM32F103_FLASH.ld
 *
 * Linker script for STM32F103C8T6 (Medium Density).
 *
 * Memory layout (STM32F103C8T6):
 *   - Flash:  64KB  (0x08000000 - 0x0800FFFF)
 *   - SRAM:   20KB  (0x20000000 - 0x20004FFF)
 *
 * If you use a different chip (e.g. STM32F103CBT6 has 128KB Flash),
 * adjust the LENGTH values below accordingly.
 *
 * This file is used ONLY by the GCC toolchain (arm-none-eabi-gcc).
 * The original Keil MDK project (Project/Project.uvprojx) uses its own
 * scatter-loading file and is unaffected.
 *****************************************************************************/

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH =  64K
    SRAM  (rwx) : ORIGIN = 0x20000000, LENGTH =  20K
}

/* Highest address of the user mode stack */
_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    /* ========== Vector Table ========== */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* ========== Program Code ========== */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)

        /* C++ exception support (if used) */
        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ========== ARM Exception Unwind Tables ========== */
    .ARM.extab :
    {
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /* ========== Read-Only Data ========== */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
    } > FLASH

    /* ========== C++ Constructor/Destructor Tables ========== */
    .preinit_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array*))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array*))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > FLASH

    /* ========== Initialized Data (.data) ========== */
    /* VMA in SRAM, LMA in FLASH (right after all read-only sections) */
    _sidata = LOADADDR(.data);

    .data : ALIGN(4)
    {
        . = ALIGN(4);
        _sdata = .;
        __data_start__ = _sdata;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        _edata = .;
        __data_end__ = _edata;
    } > SRAM AT > FLASH

    /* ========== Uninitialized Data (.bss) ========== */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > SRAM

    /* ========== Heap ========== */
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Heap_Size;
        . = ALIGN(8);
    } > SRAM

    /DISCARD/ :
    {
        *(.note.gnu.build-id)
        *(.comment)
    }
}

_Heap_Size = 0x00000200;
