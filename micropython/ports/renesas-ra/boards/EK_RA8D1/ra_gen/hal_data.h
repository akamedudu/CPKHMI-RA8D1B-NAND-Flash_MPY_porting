/* generated HAL header file - do not edit */
#ifndef HAL_DATA_H_
#define HAL_DATA_H_
#include <stdint.h>
#include "bsp_api.h"
#include "common_data.h"
#include "r_sci_b_uart.h"
#include "r_uart_api.h"
#if defined(USE_FSP_FLASH)
#include "r_flash_api.h"
#include "r_flash_hp.h"
#endif
FSP_HEADER
/** UART on SCI Instance. */
extern const uart_instance_t g_uart0;

/** Access the UART instance using these structures when calling API functions directly (::p_api is not used). */
extern sci_b_uart_instance_ctrl_t g_uart0_ctrl;
extern const uart_cfg_t g_uart0_cfg;
extern const sci_b_uart_extended_cfg_t g_uart0_cfg_extend;

#ifndef uart_callback
void uart_callback(uart_callback_args_t *p_args);
#endif

#if defined(USE_FSP_FLASH)
/** Flash HP Instance. */
extern const flash_instance_t g_flash0;

/** Access the Flash instance using these structures when calling API functions directly (::p_api is not used). */
extern flash_hp_instance_ctrl_t g_flash0_ctrl;
extern const flash_cfg_t g_flash0_cfg;

#ifndef callback_flash
void callback_flash(flash_callback_args_t *p_args);
#endif
#endif

void hal_entry(void);
void g_hal_init(void);
FSP_FOOTER
#endif /* HAL_DATA_H_ */
