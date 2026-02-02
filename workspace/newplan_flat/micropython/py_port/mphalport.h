#ifndef MICROPY_INCLUDED_RA_MPHALPORT_H
#define MICROPY_INCLUDED_RA_MPHALPORT_H

#include "hal_data.h"
#include "py/obj.h"
#include "py/mphal.h"   // MicroPython HAL 接口声明

// --- tick 相关 ----------------------------------------------------
// tick 函数现在在 mp_hal_ra8d1.c 中实现，使用 DWT 高精度计数器
// 声明函数原型，实现在 mp_hal_ra8d1.c 中

mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);
mp_uint_t mp_hal_ticks_cpu(void);

// --- 延时相关：直接用 FSP 的软件延时 -------------------------------

static inline void mp_hal_delay_ms(mp_uint_t ms) {
    R_BSP_SoftwareDelay((uint32_t) ms, BSP_DELAY_UNITS_MILLISECONDS);
}

static inline void mp_hal_delay_us(mp_uint_t us) {
    R_BSP_SoftwareDelay((uint32_t) us, BSP_DELAY_UNITS_MICROSECONDS);
}

#endif // MICROPY_INCLUDED_RA_MPHALPORT_H
