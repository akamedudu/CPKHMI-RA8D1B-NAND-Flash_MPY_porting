/*
 * modmachine.c
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "led.h"           // 引入 LED
#include "machine_pin.h"   // 引入 Pin 类型和常量定义
#include "machine_i2c.h"   // 引入 I2C 类型定义
#include "machine_spi.h"   // 引入 SPI 类型定义
#include "machine_adc.h"   // 引入 ADC 类型定义
#include "machine_dac.h"   // 引入 DAC 类型定义
#include "machine_uart.h"  // 旧 RA 端口的 UART 头文件（可以保留，也可以以后删）

// 从 py_port/machine_uart.c 引入 RA8D1 专用 machine.UART 类型
extern const mp_obj_type_t machine_uart_type;


// 定义 STATIC 宏
#ifndef STATIC
#define STATIC static
#endif

// ========== machine 模块 ==========

// machine.freq() - 返回系统核心时钟频率
STATIC mp_obj_t machine_freq(void) {
    // SystemCoreClock 在 system.h 中定义，bsp_api.h 已包含 system.h
    return mp_obj_new_int_from_uint(SystemCoreClock);
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_freq_obj, machine_freq);

// machine.unique_id() - 返回 MCU 的唯一 ID
STATIC mp_obj_t machine_unique_id(void) {
    // R_BSP_UniqueIdGet() 在 bsp_common.h 中定义，bsp_api.h 已包含 bsp_common.h
    const bsp_unique_id_t *uid = R_BSP_UniqueIdGet();
    return mp_obj_new_bytes(uid->unique_id_bytes, 16); // 16 bytes for RA8D1
}
MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);

// machine 模块的全局变量
STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_LED),         MP_ROM_PTR(&ra_led_type) },        // 导出 LED 类
    { MP_ROM_QSTR(MP_QSTR_Pin),         MP_ROM_PTR(&ra_pin_type) },        // 导出 Pin 类
    { MP_ROM_QSTR(MP_QSTR_I2C),         MP_ROM_PTR(&ra_i2c_type) },        // 导出 I2C 类
    { MP_ROM_QSTR(MP_QSTR_SPI),         MP_ROM_PTR(&ra_spi_type) },        // 导出 SPI 类
    { MP_ROM_QSTR(MP_QSTR_ADC),         MP_ROM_PTR(&ra_adc_type) },        // 导出 ADC 类
    { MP_ROM_QSTR(MP_QSTR_DAC),         MP_ROM_PTR(&ra_dac_type) },        // 导出 DAC 类
    { MP_ROM_QSTR(MP_QSTR_UART),        MP_ROM_PTR(&machine_uart_type) },  // 使用通用 machine.UART 类型
    { MP_ROM_QSTR(MP_QSTR_freq),        MP_ROM_PTR(&machine_freq_obj) },   // 导出 freq 函数
    { MP_ROM_QSTR(MP_QSTR_unique_id),   MP_ROM_PTR(&machine_unique_id_obj) }, // 导出 unique_id 函数

    // Pin 模式常量
    { MP_ROM_QSTR(MP_QSTR_IN),          MP_ROM_INT(MP_PIN_MODE_IN) },
    { MP_ROM_QSTR(MP_QSTR_OUT),         MP_ROM_INT(MP_PIN_MODE_OUT) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN),  MP_ROM_INT(MP_PIN_MODE_OPEN_DRAIN) },

    // Pin 上拉/下拉常量
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE),   MP_ROM_INT(MP_PIN_PULL_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),     MP_ROM_INT(MP_PIN_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN),   MP_ROM_INT(MP_PIN_PULL_DOWN) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

// machine 模块定义
const mp_obj_module_t mp_module_machine = {
    .base    = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&machine_module_globals,
};

// 注册模块
MP_REGISTER_MODULE(MP_QSTR_machine, mp_module_machine);
