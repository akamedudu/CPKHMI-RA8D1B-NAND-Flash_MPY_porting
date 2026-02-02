// micropython/py_port/mpconfigport.h

void mp_hal_set_interrupt_char(int c);
int mp_hal_get_interrupt_char(void);

#ifndef MICROPY_PY_BUILTINS_MEMORYVIEW
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#endif

#ifndef MICROPY_INCLUDED_RA8D1_MPCONFIGPORT_H
#define MICROPY_INCLUDED_RA8D1_MPCONFIGPORT_H

#include <stdint.h>
#include <alloca.h>

// Use the official MINIMUM ROM level as a base
#define MICROPY_CONFIG_ROM_LEVEL          (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// 启用事件驱动 REPL（支持 pyexec_event_repl_xxx）
#ifndef MICROPY_REPL_EVENT_DRIVEN
#define MICROPY_REPL_EVENT_DRIVEN (1)
#endif

// Force using builtin alloca, avoid external symbol
#ifndef MP_ALLOCA
#define MP_ALLOCA(n) __builtin_alloca(n)
#endif

// Do not use "%"-based string formatting impl (we patched modbuiltins)
#define MICROPY_PY_BUILTINS_STR_OP_MODULO (0)

// ---------------------------------------------------------------------------
// machine.UART 配置：使用 extmod/machine_uart.c + include file
// ---------------------------------------------------------------------------
#define MICROPY_PY_MACHINE_UART                    (0)
// #define MICROPY_PY_MACHINE_UART_INCLUDEFILE        "py_port/machine_uart.c"

// 我们实现了 mp_machine_uart_readchar / writechar
#define MICROPY_PY_MACHINE_UART_READCHAR_WRITECHAR (1)

// 目前不暴露 UART.sendbreak() 到 Python 层（C 里有 stub，设 0 没问题）
#define MICROPY_PY_MACHINE_UART_SENDBREAK          (0)

// 没有实现 UART.irq() 那套 mp_irq 逻辑
#define MICROPY_PY_MACHINE_UART_IRQ                (0)

// UART(0) 留给 REPL 用（mp_uart.c + g_uart0），machine.UART 禁止 new
#define MICROPY_HW_UART_IS_RESERVED(uart_id)       ((uart_id) == 0)

// UART id 范围：0 和 1（0 为保留，1 给 machine.UART 用）
#define MICROPY_HW_MAX_UART                        (2)
#define MICROPY_HW_MAX_LPUART                      (0)

// ---------------------------------------------------------------------------

// --- Core features we want ON ---
#define MICROPY_ENABLE_COMPILER           (1)
#define MICROPY_HELPER_REPL               (1)

#define MICROPY_MODULE_FROZEN_MPY         (0)
#define MICROPY_ENABLE_EXTERNAL_IMPORT    (0)

#define MICROPY_PY_GC                     (1)
#define MICROPY_ENABLE_GC                 (1)

#define MICROPY_PY_SYS                    (1)
#define MICROPY_PY_MICROPYTHON            (1)

#define MICROPY_USE_INTERNAL_PRINTF       (0)   // use printf from C lib

// Numeric / math basics
#define MICROPY_PY_BUILTINS_FLOAT         (1)
#define MICROPY_PY_BUILTINS_COMPLEX       (1)
#define MICROPY_PY_MATH                   (1)
#define MICROPY_PY_CMATH                  (1)

// bytearray is ok to keep
#define MICROPY_PY_BUILTINS_BYTEARRAY     (1)

// Disable builtin open(), we do not provide file objects yet
#define MICROPY_PY_BUILTINS_OPEN          (0)

// --- Disable extension modules which cause undefined mp_module_xxx ---
#define MICROPY_PY_ARRAY              (0)
#define MICROPY_PY_BINASCII           (1)
#define MICROPY_PY_COLLECTIONS        (0)
#define MICROPY_PY_HASHLIB            (0)
#define MICROPY_PY_HEAPQ              (0)
#define MICROPY_PY_IO                 (0)
#define MICROPY_PY_IO_IOBASE          (0)   // no IOBase without full io
#define MICROPY_PY_JSON               (0)
#define MICROPY_PY_MACHINE            (1)
#define MICROPY_PY_MACHINE_I2C        (1)
#define MICROPY_PY_OS                 (0)
#define MICROPY_PY_RANDOM             (0)
#define MICROPY_PY_RE                 (0)
#define MICROPY_PY_STRUCT             (1)
#define MICROPY_PY_TIME               (0)   // do not expose "time" module for now
#define MICROPY_PY_DEFLATE            (0)
#define MICROPY_PY_UCTYPES            (0)

// utime 使用 mp_hal 后端
#define MICROPY_PY_UTIME              (1)
#define MICROPY_PY_UTIME_MP_HAL       (1)

// VFS / filesystem disabled for now
#define MICROPY_VFS                   (0)
#define MICROPY_VFS_FAT               (0)
#define MICROPY_PY_IO_FILEIO          (0)

// --- Help() support (this does NOT require extra mp_module_XXX) ---
#define MICROPY_PY_BUILTINS_HELP           (1)
#define MICROPY_PY_BUILTINS_HELP_MODULES   (1)

// --- Misc config ---
#define MICROPY_FLOAT_IMPL                (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_LONGINT_IMPL              (MICROPY_LONGINT_IMPL_LONGLONG)  \
    // Enable long long support for large integers (needed for ticks_cpu)

#define MICROPY_ALLOC_PATH_MAX            (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT    (16)

// Basic integer types
typedef intptr_t  mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long      mp_off_t;

// Board name / MCU name
#define MICROPY_HW_BOARD_NAME  "RA8D1-minimal"
#define MICROPY_HW_MCU_NAME    "RA8D1"

// Heap size (tune later if needed)
#define MICROPY_HEAP_SIZE      (32 * 1024)

// Use VM state as port state
#define MP_STATE_PORT MP_STATE_VM

// Use Micropython's internal errno numbers
#define MICROPY_USE_INTERNAL_ERRNO 1

// Enable MicroPython scheduler for IRQ callbacks
#define MICROPY_ENABLE_SCHEDULER (1)

// Event poll hook: 让调度队列里的 callback 能在阻塞期间被执行
// mp_handle_pending 在 py/runtime.h 里声明
#define MICROPY_EVENT_POLL_HOOK   mp_handle_pending(true);

#endif // MICROPY_INCLUDED_RA8D1_MPCONFIGPORT_H
