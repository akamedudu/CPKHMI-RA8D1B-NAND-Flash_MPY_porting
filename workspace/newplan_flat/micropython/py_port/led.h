/*
 * led.h - RA8D1 LED 控制
 */

 #ifndef PY_PORT_LED_H_
 #define PY_PORT_LED_H_
 
 #include "py/obj.h"
 
 // LED 编号枚举
 typedef enum {
     RA_LED1 = 1,  // 对应 P10_01 (USER LED)
 } ra_led_t;
 
 // 底层控制函数
 void led_init(void);
 void led_state(ra_led_t led, int state);
 void led_toggle(ra_led_t led);
 
 // MicroPython LED 类型
 extern const mp_obj_type_t ra_led_type;
 
 #endif /* PY_PORT_LED_H_ */