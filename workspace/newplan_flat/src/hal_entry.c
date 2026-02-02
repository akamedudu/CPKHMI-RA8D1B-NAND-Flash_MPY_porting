/* hal_entry.c */

#include "hal_data.h"
#include "bsp_api.h"
#include "r_sci_b_uart.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/pystack.h"
#include "shared/runtime/pyexec.h"
#include "shared/readline/readline.h"

/* CMSIS: DWT/CoreDebug */
#include <core_cm85.h>

/* DWT 时间初始化（在 mp_hal_ra8d1.c 里实现） */
void mp_hal_time_init(void);

/* 串口底层在 mp_uart.c 里实现 */
void mp_uart_init(void);

/*-------------------------------
 * MicroPython heap & pystack
 *------------------------------*/
#define MP_HEAP_SIZE   (512 * 1024)
static uint8_t mp_heap[MP_HEAP_SIZE];

#if MICROPY_ENABLE_PYSTACK
static mp_obj_t mp_pystack[1024];
#endif

/*-------------------------------
 * SysTick 1ms
 *------------------------------*/
volatile uint32_t g_systick_count = 0;

/* 在 mp_hal_ra8d1.c 里定义，用于记录“本 ms 起点”的 CYCCNT */
extern volatile uint32_t g_dwt_cyccnt_ms_base;

void SysTick_Handler(void) {
    g_systick_count++;

    // 记录“本 ms 起点”的 CYCCNT，供 ticks_us 计算 ms 内子微秒，保证单调不倒退
    if (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) {
        g_dwt_cyccnt_ms_base = DWT->CYCCNT;
    }
}

void hal_entry(void) {
    /* 0) FSP init */
    g_hal_init();

    /* 1) stack info for GC */
    mp_stack_ctrl_init();
    mp_stack_set_top((void *)__get_MSP());
    mp_stack_set_limit(7 * 1024);

    /* 2) SysTick & DWT */
    SysTick_Config(SystemCoreClock / 1000U);
    mp_hal_time_init();

    /* 3) UART open */
    fsp_err_t err = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
    if (FSP_SUCCESS != err) {
        while (1) { __NOP(); }
    }
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    /* 4) start RX (ring buffer) */
    mp_uart_init();

soft_reset:
    /* 5) MicroPython runtime init */
    gc_init(mp_heap, mp_heap + MP_HEAP_SIZE);

#if MICROPY_ENABLE_PYSTACK
    mp_pystack_init(mp_pystack, &mp_pystack[MP_ARRAY_SIZE(mp_pystack)]);
#endif

    mp_init();
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

    mp_hal_set_interrupt_char(CHAR_CTRL_C);

    /* 6) banner */
    mp_hal_stdout_tx_str("\r\nMicroPython RA8D1-minimal\r\n");
    mp_hal_stdout_tx_str("Ctrl-A raw REPL | Ctrl-B friendly REPL | Ctrl-D soft reboot\r\n");

    /* 7) event-driven REPL */
    pyexec_event_repl_init();

    for (;;) {
        int c = mp_hal_stdin_rx_chr();

        if (pyexec_event_repl_process_char(c)) {
            mp_hal_stdout_tx_str("\r\nsoft reboot\r\n");
            mp_deinit();
            goto soft_reset;
        }
    }
}
