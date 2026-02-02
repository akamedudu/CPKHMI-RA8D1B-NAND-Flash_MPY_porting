/*
 * led.c - RA8D1 LED 控制实现
 *
 *  Created on: 2025年11月19日
 *      Author: soulmate
 */

 #include <stdio.h>
 #include "py/runtime.h"
 #include "py/mphal.h"
 #include "hal_data.h"
 #include "bsp_api.h"
 #include "led.h"
 
 // LED 引脚定义 - 根据您的硬件配置
 #define LED1_PIN    BSP_IO_PORT_10_PIN_01  // P10_01 (USER LED)
 
 // LED 对象结构
 typedef struct _ra_led_obj_t {
     mp_obj_base_t base;
     mp_uint_t led_id;
     bsp_io_port_pin_t led_pin;
 } ra_led_obj_t;
 
 // LED 对象数组（静态分配）
 static const ra_led_obj_t ra_led_obj[] = {
     {{&ra_led_type}, 1, LED1_PIN},
     // 如果有更多 LED，在这里添加
 };
 
 #define NUM_LEDS MP_ARRAY_SIZE(ra_led_obj)
 
 // ========== 底层硬件控制函数 ==========
 
 void led_init(void) {
     // 初始化所有 LED 为熄灭状态
     // 注意：FSP 配置已经初始化了引脚，这里只是确保状态
     for (int i = 0; i < NUM_LEDS; i++) {
         led_state(i + 1, 0);  // 熄灭
     }
 }
 
 void led_state(ra_led_t led, int state) {
     if (led < 1 || led > NUM_LEDS) {
         return;
     }
     
     bsp_io_port_pin_t led_pin = ra_led_obj[led - 1].led_pin;
     bsp_io_level_t level = state ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;
     
     R_IOPORT_PinWrite(&g_ioport_ctrl, led_pin, level);
 }
 
 void led_toggle(ra_led_t led) {
     if (led < 1 || led > NUM_LEDS) {
         return;
     }
     
     bsp_io_port_pin_t led_pin = ra_led_obj[led - 1].led_pin;
     bsp_io_level_t current_level;
     
     // 读取当前状态
     R_IOPORT_PinRead(&g_ioport_ctrl, led_pin, &current_level);
     
     // 翻转状态
     bsp_io_level_t new_level = (current_level == BSP_IO_LEVEL_HIGH) ? 
                                 BSP_IO_LEVEL_LOW : BSP_IO_LEVEL_HIGH;
     R_IOPORT_PinWrite(&g_ioport_ctrl, led_pin, new_level);
 }
 
 // ========== MicroPython 绑定 ==========
 
 static void led_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
     ra_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
     mp_printf(print, "LED(%u)", self->led_id);
 }
 
 /// \classmethod \constructor(id)
 /// 创建 LED 对象
 ///   - `id` 是 LED 编号，1 表示 USER LED (P10_01)
 static mp_obj_t led_obj_make_new(const mp_obj_type_t *type, size_t n_args,
                                    size_t n_kw, const mp_obj_t *args) {
     // 检查参数
     mp_arg_check_num(n_args, n_kw, 1, 1, false);
     
     // 获取 LED 编号
     mp_int_t led_id = mp_obj_get_int(args[0]);
     
     // 检查 LED 编号
     if (led_id < 1 || led_id > NUM_LEDS) {
         mp_raise_msg_varg(&mp_type_ValueError, 
                          MP_ERROR_TEXT("LED(%d) doesn't exist"), led_id);
     }
     
     // 返回静态 LED 对象
     return MP_OBJ_FROM_PTR(&ra_led_obj[led_id - 1]);
 }
 
 /// \method on()
 /// 点亮 LED
 static mp_obj_t led_obj_on(mp_obj_t self_in) {
     ra_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
     led_state(self->led_id, 0);
     return mp_const_none;
 }
 static MP_DEFINE_CONST_FUN_OBJ_1(led_obj_on_obj, led_obj_on);
 
 /// \method off()
 /// 熄灭 LED
 static mp_obj_t led_obj_off(mp_obj_t self_in) {
     ra_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
     led_state(self->led_id, 1);
     return mp_const_none;
 }
 static MP_DEFINE_CONST_FUN_OBJ_1(led_obj_off_obj, led_obj_off);
 
 /// \method toggle()
 /// 翻转 LED 状态
 static mp_obj_t led_obj_toggle(mp_obj_t self_in) {
     ra_led_obj_t *self = MP_OBJ_TO_PTR(self_in);
     led_toggle(self->led_id);
     return mp_const_none;
 }
 static MP_DEFINE_CONST_FUN_OBJ_1(led_obj_toggle_obj, led_obj_toggle);
 
 /// \method value([val])
 /// 获取或设置 LED 状态
 static mp_obj_t led_obj_value(size_t n_args, const mp_obj_t *args) {
     ra_led_obj_t *self = MP_OBJ_TO_PTR(args[0]);
     
     if (n_args == 1) {
         // 读取状态
         bsp_io_level_t level;
         R_IOPORT_PinRead(&g_ioport_ctrl, self->led_pin, &level);
         return mp_obj_new_int(level);
     } else {
         // 设置状态
         int state = mp_obj_is_true(args[1]);
         led_state(self->led_id, state);
         return mp_const_none;
     }
 }
 static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(led_obj_value_obj, 1, 2, led_obj_value);
 
 // LED 类的方法字典
 static const mp_rom_map_elem_t led_locals_dict_table[] = {
     { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&led_obj_on_obj) },
     { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&led_obj_off_obj) },
     { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&led_obj_toggle_obj) },
     { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&led_obj_value_obj) },
 };
 static MP_DEFINE_CONST_DICT(led_locals_dict, led_locals_dict_table);
 
 // LED 类型定义
 MP_DEFINE_CONST_OBJ_TYPE(
     ra_led_type,
     MP_QSTR_LED,
     MP_TYPE_FLAG_NONE,
     make_new, led_obj_make_new,
     print, led_obj_print,
     locals_dict, &led_locals_dict
 );
