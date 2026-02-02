#ifndef MICROPY_HW_MPCONFIGBOARD_H
#define MICROPY_HW_MPCONFIGBOARD_H

#define MICROPY_HW_BOARD_NAME       "CPKHMI-RA8D1B"
#define MICROPY_HW_MCU_NAME         "R7FA8D1BHECBD"
#define MICROPY_HW_MCU_SYSCLK       480000000   // CPU clock: 480 MHz
#define MICROPY_HW_MCU_PCLK         120000000   // Peripheral clock A: 120 MHz

// 24MHz 外部晶振
#define MICROPY_HW_XCLK             (24000000)

// 启用功能 (因为上面定义了 RA8D1，这里的 1 不会再被重定义为 0 了)
#define MICROPY_PY_MACHINE_UART     (1)
// MICROPY_PY_MACHINE_I2C 由 mpconfigboard_common.h 根据 I2C 引脚定义自动设置
// 如果需要 I2C，请定义 MICROPY_HW_I2C0_SCL 等引脚
#define MICROPY_PY_MACHINE_SPI      (1)

// REPL 串口 (SCI3 -> P408/P409)
// 注意：FSP hal_data.c 中 g_uart0 配置为 channel 3 (SCI3)
#define MICROPY_HW_UART0_TX         (pin_P409)  // SCI3_TXD
#define MICROPY_HW_UART0_RX         (pin_P408)  // SCI3_RXD
#define MICROPY_HW_UART_REPL        (HW_UART_0)
#define MICROPY_HW_UART_REPL_BAUD   (115200)

// LED
#define MICROPY_HW_USER_LED         (pin_PA01)
#define MICROPY_HW_LED1             MICROPY_HW_USER_LED
#define MICROPY_HW_LED_ON(pin)      mp_hal_pin_high(pin)
#define MICROPY_HW_LED_OFF(pin)     mp_hal_pin_low(pin)
#define MICROPY_HW_LED_TOGGLE(pin)  mp_hal_pin_toggle(pin)

// 前置声明，避免与 boardctrl.h 形成包含环
struct _boardctrl_state_t;
typedef struct _boardctrl_state_t boardctrl_state_t;

// Board-specific hooks
void ek_ra8d1_before_soft_reset_loop(boardctrl_state_t *state);
void ek_ra8d1_end_soft_reset(boardctrl_state_t *state);

#define MICROPY_BOARD_BEFORE_SOFT_RESET_LOOP  ek_ra8d1_before_soft_reset_loop
#define MICROPY_BOARD_END_SOFT_RESET         ek_ra8d1_end_soft_reset

// 板级标识宏，用于条件编译
#define MICROPY_HW_BOARD_EK_RA8D1 1

#endif