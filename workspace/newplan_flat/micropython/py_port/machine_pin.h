#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_PIN_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_PIN_H

#include "py/obj.h"
#include "bsp_api.h"  // For bsp_io_port_pin_t

// Pin 模式常量
#define MP_PIN_MODE_IN    (0)
#define MP_PIN_MODE_OUT   (1)
#define MP_PIN_MODE_OPEN_DRAIN (2)

// Pin 上拉/下拉常量
#define MP_PIN_PULL_NONE  (0)
#define MP_PIN_PULL_UP    (1)
#define MP_PIN_PULL_DOWN  (2)

// Pin 中断触发模式常量
#define MP_PIN_IRQ_RISING   (1)  // 上升沿触发
#define MP_PIN_IRQ_FALLING  (2)  // 下降沿触发

// 前向声明 Pin 类型
extern const mp_obj_type_t ra_pin_type;

// 前向声明中断上下文结构（避免循环依赖）
struct _pin_irq_context_t;
typedef struct _pin_irq_context_t pin_irq_context_t;

// Pin 对象结构定义（需要在头文件中定义以便其他模块访问成员）
struct _ra_pin_obj_t {
    mp_obj_base_t base;
    bsp_io_port_pin_t pin_id;     // FSP 引脚枚举值
    mp_int_t mode;                 // 模式：IN 或 OUT
    mp_int_t pull;                 // 上拉/下拉：PULL_NONE, PULL_UP, PULL_DOWN
    pin_irq_context_t *irq_ctx;    // 中断回调上下文（不完整类型，避免循环依赖）
};
typedef struct _ra_pin_obj_t ra_pin_obj_t;

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_PIN_H