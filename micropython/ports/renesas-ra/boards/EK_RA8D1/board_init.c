// #include <stdbool.h>

// #include "py/mphal.h"

// #include "boardctrl.h"
// #include "led.h"
// #include "systick.h"
// #include "hal_data.h"

// #define LED_BLINK_PERIOD_MS   (2000U)

// static bool s_led_timer_running = false;
// static uint32_t s_led_last_toggle_ms = 0;

// extern void g_common_init(void);

// static void ek_ra8d1_led_systick_cb(uint32_t tick_ms) {
//     if (!s_led_timer_running) {
//         return;
//     }
//     if ((uint32_t)(tick_ms - s_led_last_toggle_ms) >= LED_BLINK_PERIOD_MS) {
//         s_led_last_toggle_ms = tick_ms;
//         led_toggle(1);
//     }
// }

// static void ek_ra8d1_led_timer_start(void) {
//     if (s_led_timer_running) {
//         return;
//     }
//     s_led_last_toggle_ms = mp_hal_ticks_ms();
//     systick_enable_dispatch(SYSTICK_DISPATCH_BOARD, ek_ra8d1_led_systick_cb);
//     s_led_timer_running = true;
// }

// static void ek_ra8d1_led_timer_stop(void) {
//     if (s_led_timer_running) {
//         systick_disable_dispatch(SYSTICK_DISPATCH_BOARD);
//         s_led_timer_running = false;
//     }
//     led_state(1, 0);
// }

// void ek_ra8d1_before_soft_reset_loop(boardctrl_state_t *state) {
//     g_common_init();
//     boardctrl_before_soft_reset_loop(state);
//     ek_ra8d1_led_timer_start();
// }

// void ek_ra8d1_end_soft_reset(boardctrl_state_t *state) {
//     ek_ra8d1_led_timer_stop();
//     boardctrl_end_soft_reset(state);
// }

// // FSP UART callback - MicroPython uses ra_sci layer directly,
// // but FSP configuration still references this callback.
// // Provide empty implementation to satisfy linker.
// void uart_callback(uart_callback_args_t *p_args) {
//     // Not used - MicroPython uses ra_sci layer instead of FSP UART API
//     (void)p_args;
// }



#include <stdbool.h>
#include "py/mphal.h"
#include "boardctrl.h"
#include "led.h"
#include "systick.h"
#include "hal_data.h" // 必须包含 FSP 头文件

// 声明 FSP 初始化函数 (在 ra_gen/common_data.c 中)
extern void g_common_init(void);

// 声明 FSP 必须的串口回调 (在 ra_gen/hal_data.c 中被引用)
// 必须定义这个，否则链接时会报错 "undefined reference to uart_callback"
void uart_callback(uart_callback_args_t *p_args) {
    (void)p_args; // MicroPython 底层有自己的处理，这里留空即可防止报错
}

#define LED_BLINK_PERIOD_MS   (1000U)

static bool s_led_timer_running = false;
static uint32_t s_led_last_toggle_ms = 0;

static void ek_ra8d1_led_systick_cb(uint32_t tick_ms) {
    if (!s_led_timer_running) {
        return;
    }
    if ((uint32_t)(tick_ms - s_led_last_toggle_ms) >= LED_BLINK_PERIOD_MS) {
        s_led_last_toggle_ms = tick_ms;
        // 翻转 LED (使用 MicroPython 定义的 LED1)
        led_toggle(1);
    }
}

static void ek_ra8d1_led_timer_start(void) {
    if (s_led_timer_running) {
        return;
    }
    s_led_last_toggle_ms = mp_hal_ticks_ms();
    systick_enable_dispatch(SYSTICK_DISPATCH_BOARD, ek_ra8d1_led_systick_cb);
    s_led_timer_running = true;
}

static void ek_ra8d1_led_timer_stop(void) {
    if (s_led_timer_running) {
        systick_disable_dispatch(SYSTICK_DISPATCH_BOARD);
        s_led_timer_running = false;
    }
    led_state(1, 0);
}

void ek_ra8d1_before_soft_reset_loop(boardctrl_state_t *state) {
    // ============================================================
    // 【关键修复 2】初始化 FSP 驱动
    // 这一步会应用 ra_gen/pin_data.c (配置引脚) 
    // 和 ra_gen/vector_data.c (配置串口中断)
    // ============================================================
    g_common_init();

    // 启动 MicroPython 的初始化流程
    boardctrl_before_soft_reset_loop(state);
    
    // 启动心跳灯
    ek_ra8d1_led_timer_start();
}

void ek_ra8d1_end_soft_reset(boardctrl_state_t *state) {
    ek_ra8d1_led_timer_stop();
    boardctrl_end_soft_reset(state);
}