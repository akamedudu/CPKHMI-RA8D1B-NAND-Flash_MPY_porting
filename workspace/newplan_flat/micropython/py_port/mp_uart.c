/* mp_uart.c -- RA8D1 UART: TX + RX ring buffer (stable raw REPL/mpremote) */

#include "hal_data.h"
#include "bsp_api.h"            // __WFI/__NOP
#include "r_sci_b_uart.h"

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"         // mp_keyboard_interrupt (if enabled)

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ---- Ctrl key values (avoid dependency on pyexec/repl headers) ---- */
#ifndef MP_CHAR_CTRL_C
#define MP_CHAR_CTRL_C (3)
#endif

/* 对外接口 */
void mp_uart_init(void);
int  mp_uart_rx_any(void);
int  mp_uart_rx_chr(void);

/* -------------------------------------------------
 * interrupt char (Ctrl-C)
 * ------------------------------------------------- */
static volatile int s_interrupt_char = -1;

void mp_hal_set_interrupt_char(int c) {
    s_interrupt_char = c;
}

int mp_hal_get_interrupt_char(void) {
    return s_interrupt_char;
}

/* -------------------------------------------------
 * RX ring buffer
 * ------------------------------------------------- */
#define MP_UART_RX_BUF_SIZE (1024)

static uint8_t s_rx_buf[MP_UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

static inline uint16_t rb_next(uint16_t x) {
    return (uint16_t)((x + 1U) % MP_UART_RX_BUF_SIZE);
}

static inline bool rb_is_empty(void) {
    return s_rx_head == s_rx_tail;
}

static void rb_put(uint8_t c) {
    uint16_t next = rb_next(s_rx_head);
    if (next == s_rx_tail) {
        // 满了就丢（也可改为覆盖 tail）
        return;
    }
    s_rx_buf[s_rx_head] = c;
    s_rx_head = next;
}

static int rb_get(void) {
    if (rb_is_empty()) {
        return -1;
    }
    uint8_t c = s_rx_buf[s_rx_tail];
    s_rx_tail = rb_next(s_rx_tail);
    return (int)c;
}

/* -------------------------------------------------
 * 1-byte read arming
 * ------------------------------------------------- */
static volatile uint8_t s_rx_one = 0;
static volatile bool    s_rx_armed = false;
static volatile bool    s_rx_need_arm = true;

/* ISR 内强力重试 arm，避免偶发 IN_USE 断链 */
static void mp_uart_arm_read_1_isr(void) {
    if (s_rx_armed) {
        return;
    }

    for (int i = 0; i < 8; i++) {
        fsp_err_t err = g_uart0.p_api->read(g_uart0.p_ctrl, (uint8_t *)&s_rx_one, 1);
        if (err == FSP_SUCCESS) {
            s_rx_armed = true;
            s_rx_need_arm = false;
            return;
        }
        if (err != FSP_ERR_IN_USE) {
            break;
        }
        __NOP();
    }

    // 仍挂不上：让前台补挂，避免永久断链
    s_rx_need_arm = true;
    s_rx_armed = false;
}

/* 前台兜底补挂 */
static void mp_uart_arm_read_1_poll(void) {
    if (s_rx_armed && !s_rx_need_arm) {
        return;
    }

    fsp_err_t err = g_uart0.p_api->read(g_uart0.p_ctrl, (uint8_t *)&s_rx_one, 1);
    if (err == FSP_SUCCESS) {
        s_rx_armed = true;
        s_rx_need_arm = false;
    } else {
        s_rx_need_arm = true;
        // err==IN_USE: 下次再试；其它错误：也留给下次再试
    }
}

/* -------------------------------------------------
 * FSP 配置里 g_uart0_cfg.callback 指向的全局符号
 * ------------------------------------------------- */
void uart_callback(uart_callback_args_t *p_args) {
    if (!p_args) {
        return;
    }

    if (p_args->event == UART_EVENT_RX_COMPLETE) {
        s_rx_armed = false;

        uint8_t c = (uint8_t)s_rx_one;

        // Ctrl-C：如果启用了 MICROPY_KBD_EXCEPTION，就触发 KeyboardInterrupt
        if (s_interrupt_char >= 0 && c == (uint8_t)s_interrupt_char) {
            #if MICROPY_KBD_EXCEPTION
            mp_keyboard_interrupt();
            #else
            rb_put(c);
            #endif
        } else {
            rb_put(c);
        }

        mp_uart_arm_read_1_isr();
    } else {
        s_rx_armed = false;
        s_rx_need_arm = true;
        mp_uart_arm_read_1_isr();
    }
}

/* -------------------------------------------------
 * init
 * ------------------------------------------------- */
void mp_uart_init(void) {
    s_rx_head = s_rx_tail = 0;
    s_rx_armed = false;
    s_rx_need_arm = true;

    // 默认 Ctrl-C
    mp_hal_set_interrupt_char(MP_CHAR_CTRL_C);

    // 补挂第一次接收
    mp_uart_arm_read_1_poll();
}

/* -------------------------------------------------
 * TX (stdout)
 * ------------------------------------------------- */
static void uart_wait_tx_complete(void) {
    sci_b_uart_instance_ctrl_t *p_ctrl = (sci_b_uart_instance_ctrl_t *)g_uart0.p_ctrl;

    volatile uint32_t timeout = 1000000U;
    while (p_ctrl->tx_src_bytes > 0 && timeout > 0) {
        __NOP();
        timeout--;
    }

    timeout = 100000U;
    while ((p_ctrl->p_reg->CSR_b.TEND == 0) && timeout > 0) {
        __NOP();
        timeout--;
    }
}

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (len == 0) {
        return 0;
    }

    fsp_err_t err;
    do {
        err = g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t const *)str, (uint32_t)len);
    } while (err == FSP_ERR_IN_USE);

    if (err != FSP_SUCCESS) {
        return 0;
    }

    uart_wait_tx_complete();
    return (mp_uint_t)len;
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    char last = 0;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\n' && last != '\r') {
            const char cr = '\r';
            mp_hal_stdout_tx_strn(&cr, 1);
        }
        mp_hal_stdout_tx_strn(&c, 1);
        last = c;
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn_cooked(str, strlen(str));
}

/* -------------------------------------------------
 * RX API for uart_core.c
 * ------------------------------------------------- */
int mp_uart_rx_any(void) {
    mp_uart_arm_read_1_poll();
    return !rb_is_empty();
}

int mp_uart_rx_chr(void) {
    for (;;) {
        int c = rb_get();
        if (c >= 0) {
            mp_uart_arm_read_1_poll();
            return c;
        }
        mp_uart_arm_read_1_poll();
        __WFI();
    }
}
