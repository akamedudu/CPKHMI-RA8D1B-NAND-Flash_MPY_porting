#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_UART_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_UART_H

#include "py/obj.h"
#include "py/ringbuf.h"
#include "r_uart_api.h"  // FSP UART API types

// Default RX buffer size
#define UART_RX_BUF_SIZE (256)

// UART object structure
typedef struct _ra_uart_obj_t {
    mp_obj_base_t base;
    const uart_instance_t *uart_instance;  // Pointer to FSP UART instance (g_uart0 or g_uart1)
    uint8_t uart_id;                      // MicroPython UART ID (0 or 1)
    uint32_t baudrate;                    // Current baudrate
    bool is_open;                         // Track if UART is open
    ringbuf_t rx_buf;                     // Ring buffer for RX data
    uint8_t *rx_buf_storage;              // Allocated storage for ring buffer
    volatile bool tx_complete;            // Flag for TX completion
    uart_callback_args_t callback_memory;  // Memory for callback arguments
} ra_uart_obj_t;

// Forward declaration of UART type
extern const mp_obj_type_t ra_uart_type;

// Global UART objects (for REPL integration)
extern ra_uart_obj_t *g_uart0_obj;  // UART0 object (for REPL)

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_UART_H

