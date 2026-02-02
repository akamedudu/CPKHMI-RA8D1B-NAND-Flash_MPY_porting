// extmod/modutime_mphal.c
// 最小 utime（基于 mp_hal_*），兼容 bare-metal 端口
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#ifndef STATIC
#define STATIC static
#endif


#if MICROPY_PY_UTIME && MICROPY_PY_UTIME_MP_HAL

// --- sleep(seconds) ---
STATIC mp_obj_t mod_utime_sleep(mp_obj_t seconds_in) {
    // 支持 int/float 秒；float 情况转成毫秒
    #if MICROPY_PY_BUILTINS_FLOAT
    if (mp_obj_is_float(seconds_in)) {
        mp_float_t s = mp_obj_get_float(seconds_in);
        mp_uint_t ms = (mp_uint_t)(s * 1000.0f);
        mp_hal_delay_ms(ms);
        return mp_const_none;
    }
    #endif
    mp_int_t s = mp_obj_get_int(seconds_in);
    while (s-- > 0) {
        mp_hal_delay_ms(1000);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_utime_sleep_obj, mod_utime_sleep);

// --- sleep_ms(ms) ---
STATIC mp_obj_t mod_utime_sleep_ms(mp_obj_t ms_in) {
    mp_uint_t ms = mp_obj_get_int(ms_in);
    mp_hal_delay_ms(ms);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_utime_sleep_ms_obj, mod_utime_sleep_ms);

// --- sleep_us(us) ---
STATIC mp_obj_t mod_utime_sleep_us(mp_obj_t us_in) {
    mp_uint_t us = mp_obj_get_int(us_in);
    mp_hal_delay_us(us);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_utime_sleep_us_obj, mod_utime_sleep_us);

// --- ticks_ms() ---
STATIC mp_obj_t mod_utime_ticks_ms(void) {
    return mp_obj_new_int_from_uint(mp_hal_ticks_ms());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_utime_ticks_ms_obj, mod_utime_ticks_ms);

// --- ticks_us() ---
STATIC mp_obj_t mod_utime_ticks_us(void) {
    return mp_obj_new_int_from_uint(mp_hal_ticks_us());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_utime_ticks_us_obj, mod_utime_ticks_us);

// --- ticks_cpu() ---
STATIC mp_obj_t mod_utime_ticks_cpu(void) {
    // Convert to unsigned long long to ensure proper handling of large values
    unsigned long long cpu_ticks = (unsigned long long)mp_hal_ticks_cpu();
    return mp_obj_new_int_from_ull(cpu_ticks);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_utime_ticks_cpu_obj, mod_utime_ticks_cpu);

// --- ticks_diff(end, start) ---
// Calculate the difference between two ticks values, handling wrap-around
STATIC mp_obj_t mod_utime_ticks_diff(mp_obj_t end_in, mp_obj_t start_in) {
    mp_uint_t end = mp_obj_get_int(end_in);
    mp_uint_t start = mp_obj_get_int(start_in);
    // Handle wrap-around: if end < start, wrap-around occurred
    // The result is a signed integer representing the difference
    mp_int_t diff = (mp_int_t)(end - start);
    return mp_obj_new_int(diff);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_utime_ticks_diff_obj, mod_utime_ticks_diff);

// 模块全局表
STATIC const mp_rom_map_elem_t mp_module_utime_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),   MP_ROM_QSTR(MP_QSTR_utime) },
    { MP_ROM_QSTR(MP_QSTR_sleep),      MP_ROM_PTR(&mod_utime_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms),   MP_ROM_PTR(&mod_utime_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_us),   MP_ROM_PTR(&mod_utime_sleep_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms),   MP_ROM_PTR(&mod_utime_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_us),   MP_ROM_PTR(&mod_utime_ticks_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_cpu),  MP_ROM_PTR(&mod_utime_ticks_cpu_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mod_utime_ticks_diff_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_utime_globals, mp_module_utime_globals_table);

// 模块对象
const mp_obj_module_t mp_module_utime = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_utime_globals,
};

// 自动注册（若端口支持）
MP_REGISTER_MODULE(MP_QSTR_utime, mp_module_utime);

#endif // MICROPY_PY_UTIME && MICROPY_PY_UTIME_MP_HAL
