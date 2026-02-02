#include "py/mphal.h"

int mp_uart_rx_any(void);
int mp_uart_rx_chr(void);

int mp_hal_stdin_rx_any(void) {
    return mp_uart_rx_any();
}

int mp_hal_stdin_rx_chr(void) {
    return mp_uart_rx_chr();
}
