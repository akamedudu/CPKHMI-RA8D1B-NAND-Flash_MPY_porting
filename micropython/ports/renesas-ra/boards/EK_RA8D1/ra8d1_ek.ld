/*
                  Linker File for RA8D1 MCU (R7FA8D1BHECBD)
                  EK-RA8D1 Board Configuration
                  Non-secure domain configuration
                  
                  Clock Configuration (from bsp_clock_cfg.h):
                  - XTAL: 24MHz
                  - PLL1P: 480MHz (CPU Clock)
                  - ICLK: 240MHz (System Clock /2)
                  - PCLKA: 120MHz (Peripheral Clock A /4)
                  - PCLKB: 60MHz (Peripheral Clock B /8)
                  
                  Memory Layout:
                  - Code Flash: 1920KB (0x02000000 - 0x021DFFFF)
                  - File System: 128KB (0x021E0000 - 0x021FFFFF)
                  - SRAM: 1MB total, 896KB available for non-secure (0x22000000 - 0x220DFFFF)
                  - Data Flash: 64KB, using 12KB for storage
*/

/* Linker script to configure memory regions. */
MEMORY
{
  FLASH (rx)         : ORIGIN = 0x02000000, LENGTH = 0x001E0000  /* 1920KB Non-secure Code Flash */
  FLASH_FS (r)       : ORIGIN = 0x021E0000, LENGTH = 0x00020000  /* 128KB File System in Flash */
  RAM (rwx)          : ORIGIN = 0x22000000, LENGTH = 0x000E0000  /* 896KB Non-secure SRAM */
 /* DATA_FLASH (rx)    : ORIGIN = 0x40100000, LENGTH = 0x00003000   12KB Data Flash */
  DATA_FLASH (rx)    : ORIGIN = 0x27000000, LENGTH = 0x00003000  /* 12KB Data Flash */
  ID_CODE (rx)       : ORIGIN = 0x0100A150, LENGTH = 0x00000010  /* 16 bytes ID Code */
}

/* Library configurations */
/*GROUP(libgcc.a libc.a libm.a libnosys.a) */

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        _stext = .;
        __ROM_Start = .;

        /* Even though the vector table is not 256 entries (1KB) long, we still allocate that much
         * space because ROM registers are at address 0x400 and there is very little space
         * in between. */
        KEEP(*(.fixed_vectors*))
        KEEP(*(.application_vectors*))
        __Vectors_End = .;
        __end__ = .;

        /* ROM Registers start at address 0x00000400 */
        . = __ROM_Start + 0x400;
        KEEP(*(.rom_registers*))

        /* Reserving 0x100 bytes of space for ROM registers. */
        . = __ROM_Start + 0x500;

        *(.text*)

        KEEP(*(.version))
        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)
        __usb_dev_descriptor_start_fs = .;
        KEEP(*(.usb_device_desc_fs*))
        __usb_cfg_descriptor_start_fs = .;
        KEEP(*(.usb_config_desc_fs*))
        __usb_interface_descriptor_start_fs = .;
        KEEP(*(.usb_interface_desc_fs*))
        __usb_descriptor_end_fs = .;
        __usb_dev_descriptor_start_hs = .;
        KEEP(*(.usb_device_desc_hs*))
        __usb_cfg_descriptor_start_hs = .;
        KEEP(*(.usb_config_desc_hs*))
        __usb_interface_descriptor_start_hs = .;
        KEEP(*(.usb_interface_desc_hs*))
        __usb_descriptor_end_hs = .;

        KEEP(*(.eh_frame*))

        __ROM_End = .;
        _etext = .;
    } > FLASH = 0xFF

    __Vectors_Size = __Vectors_End - __Vectors;

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;

    /* If DTC is used, put the DTC vector table at the start of SRAM.
       This avoids memory holes due to 1K alignment required by it. */
    .fsp_dtc_vector_table (NOLOAD) :
    {
        . = ORIGIN(RAM);
        *(.fsp_dtc_vector_table)
    } > RAM

    .noinit (NOLOAD):
    {
        . = ALIGN(4);
        __noinit_start = .;
        KEEP(*(.noinit*))
        __noinit_end = .;
    } > RAM

    /* Initialized data section */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        __DATA_START__ = _sdata; /* FSP UPPPERCASE ALIAS */
        
        /* Your original content */
        *(vtable)
        *(.data.*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);

        __Code_In_RAM_Start = .;
        KEEP(*(.code_in_ram*))
        __Code_In_RAM_End = .;
        
        . = ALIGN(4);
        /* All data end */
        _edata = .;
        __DATA_END__ = _edata;   /* FSP UPPPERCASE ALIAS */
    } > RAM AT > FLASH

    /* Store the LMA (Load Address) of .data in Flash */
    _sidata = LOADADDR(.data);
    __DATA_ROM_START__ = _sidata; /* FSP UPPPERCASE ALIAS */

    /* BSS section (uninitialized data) */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __BSS_START__ = _sbss;  /* FSP UPPPERCASE ALIAS */
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __BSS_END__ = _ebss;    /* FSP UPPPERCASE ALIAS */
    } > RAM

    /* Heap section - grows upward from end of BSS */
    .heap (NOLOAD):
    {
        . = ALIGN(8);
        __HeapBase = .;
        __end__ = .;
        end = __end__;
        /* Heap grows from here up to the stack limit */
    } > RAM

    /* Define heap limit (will be set at runtime or as symbol) */
    __HeapLimit = ORIGIN(RAM) + LENGTH(RAM);

    /* Stacks are stored in this section - placed at end of RAM, grows downward */
    .stack_dummy (NOLOAD):
    {
        . = ALIGN(8);
        _sstack = .;
        __StackLimit = .;
        /* Main stack */
        KEEP(*(.stack))
        __StackTop = .;
        /* Thread stacks */
        KEEP(*(.stack*))
        __StackTopAll = .;
        _estack = .;
    } > RAM

    PROVIDE(__stack = __StackTopAll);

    /* This symbol represents the end of user allocated RAM. The RAM after this symbol can be used
       at run time for things such as ThreadX memory pool allocations. */
    __RAM_segment_used_end__ = ALIGN(__StackTopAll , 4);

    /* Data flash. */
    .data_flash :
    {
        __Data_Flash_Start = .;
        KEEP(*(.data_flash*))
        __Data_Flash_End = .;
    } > DATA_FLASH

    .id_code :
    {
        __ID_Code_Start = .;
        KEEP(*(.id_code*))
        __ID_Code_End = .;
    } > ID_CODE

    /* Internal flash storage for MicroPython filesystem */
    .flash_fs :
    {
        . = ALIGN(4);
        __Flash_FS_Start = .;
        KEEP(*(.flash_fs*))
        __Flash_FS_End = .;
    } > FLASH_FS
}
/* produce a link error if there is not this amount of RAM for these sections */
/* _minimum_stack_size = 2K; */
/* _minimum_heap_size = 16K; */

/* Define the top end of the stack.  The stack is full descending so begins just
   above last byte of RAM.  Note that EABI requires the stack to be 8-byte
   aligned for a call. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
_heap_start = __HeapBase;	/* heap starts just after statically allocated memory */
_heap_end = __HeapLimit;	/* tunable */

_micropy_hw_internal_flash_storage_start = ORIGIN(FLASH_FS);
_micropy_hw_internal_flash_storage_end   = ORIGIN(FLASH_FS) + LENGTH(FLASH_FS);
