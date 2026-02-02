/*
 * machine_uart.c - RA8D1 UART Implementation using FSP r_sci_b_uart driver
 *
 * 设计原则：
 *  - UART(0) 保留给 REPL，由 mp_uart.c 独占管理（包括 uart_callback）。
 *  - machine.UART 这里只实现 UART(1)（假设映射到 g_uart1 / SCI9）。
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/stream.h"
#include "py/ringbuf.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "machine_uart.h"
#include "r_sci_b_uart.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef STATIC
#define STATIC static
#endif

// 仅支持 machine.UART(1)。UART(0) 由 mp_uart.c 管理，用于 REPL。
STATIC ra_uart_obj_t *g_uart1_obj = NULL;

// 调试：统计 UART1 IRQ 触发次数（RX / TX）
volatile uint32_t g_uart1_irq_rx_cnt = 0;
volatile uint32_t g_uart1_irq_tx_cnt = 0;

// 前置声明
STATIC void uart_obj_deinit(ra_uart_obj_t *self);
STATIC void uart_obj_init_helper(ra_uart_obj_t *self, uint32_t baudrate);

// ========== UART1 中断回调（给 machine.UART 使用） ==========

void uart1_callback(uart_callback_args_t *p_args) {
    if (g_uart1_obj == NULL) {
        return;
    }

    ra_uart_obj_t *self = g_uart1_obj;

    if (p_args->event == UART_EVENT_RX_CHAR) {
        // RX 单字节中断
        g_uart1_irq_rx_cnt++;

        uint8_t data = (uint8_t)(p_args->data & 0xFFu);
        ringbuf_put(&self->rx_buf, data);

    } else if (p_args->event == UART_EVENT_TX_COMPLETE) {
        // TX 完成中断
        g_uart1_irq_tx_cnt++;
        self->tx_complete = true;
    }
}

// ========== UART 对象实现 ==========

STATIC void uart_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u)", (unsigned)self->uart_id, (unsigned)self->baudrate);
}

// 初始化 helper（打开硬件 / 设置回调 / 配置波特率 / 初始化 ringbuf）
STATIC void uart_obj_init_helper(ra_uart_obj_t *self, uint32_t baudrate) {
    fsp_err_t err;

    // 如果之前已经 open 过，先关掉
    if (self->is_open) {
        self->uart_instance->p_api->close(self->uart_instance->p_ctrl);
        self->is_open = false;
    }

    // 打开 UART 硬件
    err = self->uart_instance->p_api->open(self->uart_instance->p_ctrl, self->uart_instance->p_cfg);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
            MP_ERROR_TEXT("Failed to open UART %u: %d"), self->uart_id, (int)err);
    }

    // 只给 UART1 设置回调（machine.UART 不碰 UART0）
    err = self->uart_instance->p_api->callbackSet(
        self->uart_instance->p_ctrl,
        uart1_callback,
        NULL,
        &self->callback_memory
        );
    if (err != FSP_SUCCESS) {
        self->uart_instance->p_api->close(self->uart_instance->p_ctrl);
        mp_raise_msg_varg(&mp_type_RuntimeError,
            MP_ERROR_TEXT("Failed to set UART callback: %d"), (int)err);
    }

    // 配置波特率（如果传入不为 0）
    if (baudrate != 0) {
        sci_b_baud_setting_t baud_setting;
        err = R_SCI_B_UART_BaudCalculate(
            baudrate,
            false, // bitrate_modulation
            0,     // baud_rate_error_x_1000
            &baud_setting
            );
        if (err == FSP_SUCCESS) {
            (void)self->uart_instance->p_api->baudSet(self->uart_instance->p_ctrl, &baud_setting);
            // 这里即使失败也不强制报错，继续用默认波特率
        }
        self->baudrate = baudrate;
    }

    // 初始化 ring buffer（rx_buf_storage / rx_buf 的具体结构在 machine_uart.h / ringbuf.h 中定义）
    if (self->rx_buf_storage == NULL) {
        self->rx_buf_storage = m_new(uint8_t, UART_RX_BUF_SIZE);
        // 注意：ringbuf_alloc 的实现来自端口自身，这里沿用原有接口
        ringbuf_alloc(&self->rx_buf, UART_RX_BUF_SIZE);
    } else {
        // 重置 ring buffer 指针
        self->rx_buf.iget = 0;
        self->rx_buf.iput = 0;
    }

    self->is_open = true;
    self->tx_complete = true;

    // 记录全局对象指针（仅 UART1）
    if (self->uart_id == 1) {
        g_uart1_obj = self;
    }
}

// 关闭 UART / 释放资源
STATIC void uart_obj_deinit(ra_uart_obj_t *self) {
    if (self->is_open) {
        // 终止正在进行的收发
        self->uart_instance->p_api->communicationAbort(self->uart_instance->p_ctrl, UART_DIR_RX_TX);

        // 关闭 UART
        self->uart_instance->p_api->close(self->uart_instance->p_ctrl);
        self->is_open = false;
    }

    // 释放 ring buffer 存储（如果你 port 的 ringbuf_alloc 自己分配内存，这里可以不用释放 rx_buf_storage）
    if (self->rx_buf_storage != NULL) {
        m_del(uint8_t, self->rx_buf_storage, UART_RX_BUF_SIZE);
        self->rx_buf_storage = NULL;
    }

    // 清理全局指针（只维护 UART1）
    if (self->uart_id == 1 && g_uart1_obj == self) {
        g_uart1_obj = NULL;
    }
}

// 构造函数：UART(id, baudrate=115200, ...)
STATIC mp_obj_t uart_obj_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {

    enum {
        ARG_id,
        ARG_baudrate,
        ARG_bits,
        ARG_parity,
        ARG_stop,
        ARG_timeout,
        ARG_timeout_char,
        ARG_invert,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,           MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate,     MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits,         MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity,       MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop,         MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_timeout,      MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout_char, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_invert,       MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(
        n_args, n_kw, args,
        MP_ARRAY_SIZE(allowed_args), allowed_args, vals
        );

    // 只允许 UART(1)，UART(0) 为 REPL 保留
    int uart_id = vals[ARG_id].u_int;
    if (uart_id != 1) {
        mp_raise_msg_varg(&mp_type_ValueError,
            MP_ERROR_TEXT("UART(%d) doesn't exist or is reserved"), uart_id);
    }

    // 映射到 FSP 实例（假设 g_uart1 是在 hal_data.c 里生成的 uart_instance_t）
    const uart_instance_t *uart_instance = &g_uart1;

    // 检查是否已经被占用
    if (g_uart1_obj != NULL) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("UART(1) is already in use"));
    }

    // 分配并初始化对象
    ra_uart_obj_t *self = m_new_obj(ra_uart_obj_t);
    self->base.type = type;
    self->uart_instance = uart_instance;
    self->uart_id = (uint8_t)uart_id;
    self->baudrate = vals[ARG_baudrate].u_int;
    self->is_open = false;
    self->rx_buf_storage = NULL;
    self->tx_complete = true;

    // 初始化 UART 硬件
    uart_obj_init_helper(self, self->baudrate);

    return MP_OBJ_FROM_PTR(self);
}

// init(baudrate=115200, ...)
STATIC mp_obj_t uart_obj_init(size_t n_args, const mp_obj_t *args) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    enum {
        ARG_baudrate,
        ARG_bits,
        ARG_parity,
        ARG_stop,
        ARG_timeout,
        ARG_timeout_char,
        ARG_invert,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate,     MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits,         MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity,       MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop,         MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_timeout,      MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout_char, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_invert,       MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(
        n_args - 1, 0, args + 1,
        MP_ARRAY_SIZE(allowed_args), allowed_args, vals
        );

    uint32_t baudrate = vals[ARG_baudrate].u_int;
    uart_obj_init_helper(self, baudrate);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(uart_obj_init_obj, 1, 10, uart_obj_init);

// deinit()
STATIC mp_obj_t uart_obj_deinit_func(mp_obj_t self_in) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uart_obj_deinit(self);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_obj_deinit_obj, uart_obj_deinit_func);

// read(nbytes=None)
STATIC mp_obj_t uart_obj_read(size_t n_args, const mp_obj_t *args) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("UART not initialized"));
    }

    size_t avail = ringbuf_avail(&self->rx_buf);
    size_t nbytes;

    if (n_args == 1 || (n_args > 1 && args[1] == mp_const_none)) {
        // uart.read() / uart.read(None) -> 读出所有可用数据
        nbytes = avail;
    } else {
        mp_int_t req = mp_obj_get_int(args[1]);
        nbytes = (req < 0) ? avail : (size_t)req;
        if (nbytes > avail) {
            nbytes = avail;
        }
    }

    if (avail == 0 || nbytes == 0) {
        return mp_const_empty_bytes;
    }

    uint8_t *buf = m_new(uint8_t, nbytes);
    for (size_t i = 0; i < nbytes; i++) {
        int c = ringbuf_get(&self->rx_buf);
        if (c < 0) {
            nbytes = i;
            break;
        }
        buf[i] = (uint8_t)c;
    }

    return mp_obj_new_bytes(buf, nbytes);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(uart_obj_read_obj, 1, 2, uart_obj_read);

// readline()
STATIC mp_obj_t uart_obj_readline(mp_obj_t self_in) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("UART not initialized"));
    }

    vstr_t vstr;
    vstr_init(&vstr, 16);

    while (ringbuf_avail(&self->rx_buf) > 0) {
        int c = ringbuf_get(&self->rx_buf);
        if (c < 0) {
            break;
        }
        vstr_add_byte(&vstr, (uint8_t)c);
        if (c == '\n' || c == '\r') {
            break;
        }
    }

    return mp_obj_new_str_from_vstr(&vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_obj_readline_obj, uart_obj_readline);

// readinto(buf, nbytes=None)
STATIC mp_obj_t uart_obj_readinto(size_t n_args, const mp_obj_t *args) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("UART not initialized"));
    }

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);

    size_t nbytes = bufinfo.len;
    if (n_args > 2) {
        nbytes = mp_obj_get_int(args[2]);
        if (nbytes > bufinfo.len) {
            nbytes = bufinfo.len;
        }
    }

    size_t avail = ringbuf_avail(&self->rx_buf);
    if (avail == 0) {
        return mp_obj_new_int(0);
    }
    if (nbytes > avail) {
        nbytes = avail;
    }

    uint8_t *buf = (uint8_t *)bufinfo.buf;
    for (size_t i = 0; i < nbytes; i++) {
        int c = ringbuf_get(&self->rx_buf);
        if (c < 0) {
            return mp_obj_new_int((mp_int_t)i);
        }
        buf[i] = (uint8_t)c;
    }

    return mp_obj_new_int((mp_int_t)nbytes);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(uart_obj_readinto_obj, 2, 3, uart_obj_readinto);

// write(buf) - 非阻塞发送（只要 FSP write() 成功就立即返回）
STATIC mp_obj_t uart_obj_write(mp_obj_t self_in, mp_obj_t buf_in) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("UART not initialized"));
    }

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len == 0) {
        return mp_obj_new_int(0);
    }

    // 启动一次发送（FSP 是异步 API，这里不等待 TX 完成）
    fsp_err_t err = self->uart_instance->p_api->write(
        self->uart_instance->p_ctrl,
        (uint8_t const *)bufinfo.buf,
        (uint32_t)bufinfo.len
        );

    if (err == FSP_ERR_IN_USE) {
        // UART 正在忙（上一次发送尚未完成），这里返回 0 表示本次没有发送任何字节
        return mp_obj_new_int(0);
    } else if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_OSError,
            MP_ERROR_TEXT("UART write failed: %d"), (int)err);
    }

    self->tx_complete = false;
    return mp_obj_new_int((mp_int_t)bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(uart_obj_write_obj, uart_obj_write);

// any() - 返回 RX 可读字节数
STATIC mp_obj_t uart_obj_any(mp_obj_t self_in) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_open) {
        return mp_obj_new_int(0);
    }

    size_t avail = ringbuf_avail(&self->rx_buf);
    return mp_obj_new_int((mp_int_t)avail);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_obj_any_obj, uart_obj_any);

// txdone() - 发送是否完成（靠 TX_COMPLETE 中断置位）
STATIC mp_obj_t uart_obj_txdone(mp_obj_t self_in) {
    ra_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_open) {
        return mp_const_false;
    }

    return mp_obj_new_bool(self->tx_complete);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_obj_txdone_obj, uart_obj_txdone);

// 调试接口：UART.help() -> (rx_irq_count, tx_irq_count)
STATIC mp_obj_t uart_help(mp_obj_t self_in) {
    (void)self_in;
    mp_obj_t tuple[2];
    tuple[0] = mp_obj_new_int_from_uint(g_uart1_irq_rx_cnt);
    tuple[1] = mp_obj_new_int_from_uint(g_uart1_irq_tx_cnt);
    return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_help_obj, uart_help);

// UART 成员方法表
STATIC const mp_rom_map_elem_t uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&uart_obj_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&uart_obj_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),     MP_ROM_PTR(&uart_obj_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&uart_obj_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&uart_obj_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),    MP_ROM_PTR(&uart_obj_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),      MP_ROM_PTR(&uart_obj_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_txdone),   MP_ROM_PTR(&uart_obj_txdone_obj) },

    // 调试接口：UART.help() 显示 IRQ 计数
    { MP_ROM_QSTR(MP_QSTR_help),     MP_ROM_PTR(&uart_help_obj) },
};
STATIC MP_DEFINE_CONST_DICT(uart_locals_dict, uart_locals_dict_table);

// UART 类型定义（注意：名字必须是 machine_uart_type，供 modmachine.c 引用）
MP_DEFINE_CONST_OBJ_TYPE(
    machine_uart_type,
    MP_QSTR_UART,
    MP_TYPE_FLAG_NONE,
    make_new, uart_obj_make_new,
    print, uart_obj_print,
    locals_dict, &uart_locals_dict
);
