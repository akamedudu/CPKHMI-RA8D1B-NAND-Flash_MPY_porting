// micropython/py_port/mp_hal_ra8d1.c
// 目标：ticks_us 跨 >9s 不受 DWT 32-bit 回绕影响，并保证密集采样单调不倒退
// 方法：SysTick(ms)*1000 + ((CYCCNT - base_of_this_ms) -> 0~999us)
// base_of_this_ms 由 SysTick_Handler 每 1ms 更新

#include "hal_data.h"
#include "bsp_api.h"
#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include <stdint.h>
#include <core_cm85.h>

// SysTick 计数（单位：ms），在 hal_entry.c / SysTick ISR 中递增
extern volatile uint32_t g_systick_count;

// 由 SysTick ISR 在每个 ms 边界更新：记录“本 ms 起点的 CYCCNT”
volatile uint32_t g_dwt_cyccnt_ms_base = 0;

// 缓存：每毫秒多少 CPU cycles
static uint32_t s_cycles_per_ms = 480000u;   // 默认 480MHz -> 480000 cycles/ms

void mp_hal_time_init(void) {
    // 使能 DWT CYCCNT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // 计算 cycles/ms（尽量用真实 SystemCoreClock）
    uint32_t core = SystemCoreClock;
    if (core == 0) {
        core = 480000000u; // 兜底
    }
    s_cycles_per_ms = core / 1000u;
    if (s_cycles_per_ms == 0) {
        s_cycles_per_ms = 1;
    }

    // 初始化 base（避免刚启动时 sub_us 异常）
    if (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) {
        g_dwt_cyccnt_ms_base = DWT->CYCCNT;
    } else {
        g_dwt_cyccnt_ms_base = 0;
    }
}

// 毫秒延时：每 1ms 轮询一次事件调度
void mp_hal_delay_ms(mp_uint_t ms) {
    while (ms-- > 0) {
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
        MICROPY_EVENT_POLL_HOOK;
    }
}

// 微秒延时：纯阻塞
void mp_hal_delay_us(mp_uint_t us) {
    R_BSP_SoftwareDelay((uint32_t)us, BSP_DELAY_UNITS_MICROSECONDS);
}

// ticks_ms：直接用 SysTick 作为长时基
mp_uint_t mp_hal_ticks_ms(void) {
    return (mp_uint_t)g_systick_count;
}

// ticks_us：SysTick(ms)*1000 + (CYCCNT - base) 换算成 0~999us
mp_uint_t mp_hal_ticks_us(void) {
    // DWT 不可用：退化为 1ms 分辨率
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        return (mp_uint_t)g_systick_count * 1000u;
    }

    uint32_t ms1, ms2;
    uint32_t base1, base2;
    uint32_t cyc;

    // 稳定快照：确保 ms 与 base 属于同一个毫秒边界
    do {
        ms1 = g_systick_count;
        base1 = g_dwt_cyccnt_ms_base;
        cyc = DWT->CYCCNT;
        base2 = g_dwt_cyccnt_ms_base;
        ms2 = g_systick_count;
    } while (ms1 != ms2 || base1 != base2);

    // 本 ms 已经过的 cycles（无符号减法，CYCCNT 回绕也成立）
    uint32_t sub_cycles = (uint32_t)(cyc - base1);

    // 如果 SysTick 中断被延迟，sub_cycles 可能超过 1ms，这里折叠回 0~<1ms
    if (sub_cycles >= s_cycles_per_ms) {
        sub_cycles %= s_cycles_per_ms;
    }

    // sub_us in [0,999]
    uint32_t sub_us = (uint32_t)(((uint64_t)sub_cycles * 1000u) / (uint64_t)s_cycles_per_ms);
    if (sub_us > 999u) {
        sub_us = 999u;
    }

    return (mp_uint_t)ms1 * 1000u + (mp_uint_t)sub_us;
}

// ticks_cpu：高分辨率 profiling 用（仍是 32-bit CYCCNT，会在 ~8.95s 回绕）
mp_uint_t mp_hal_ticks_cpu(void) {
    if (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) {
        return (mp_uint_t)DWT->CYCCNT;
    }
    return mp_hal_ticks_us();
}
