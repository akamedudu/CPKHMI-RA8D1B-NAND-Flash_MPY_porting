/*
 * machine_pin.c - RA8D1 GPIO Pin 控制实现
 *
 *  Created on: 2025年11月19日
 *      Author: soulmate
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "hal_data.h"
#include "bsp_api.h"  // 已包含 bsp_irq.h 和 R_ICU 寄存器定义
#include "r_ioport_api.h"
#include "r_ioport.h"  // 需要包含此头文件以使用 IOPORT_CFG_NMOS_ENABLE
#include "r_external_irq_api.h"  // FSP External IRQ API
#include "r_icu.h"  // 包含 ICU_OPEN 定义和 icu_instance_ctrl_t
#include "common_data.h"  // 包含 g_external_irq_s2 定义
#include "vector_data.h"  // 包含 ICU_IRQ12_IRQn 定义
#include "machine_pin.h"
 
 // 定义 STATIC 宏
 #ifndef STATIC
 #define STATIC static
 #endif
 
// ========== Pin 对象结构 ==========

// 中断回调上下文结构定义
// ra_pin_obj_t 已经在 machine_pin.h 中定义
struct _pin_irq_context_t {
    mp_obj_t handler;              // Python 回调函数
    ra_pin_obj_t *pin_obj;         // 指向 Pin 对象
    mp_int_t trigger;              // 触发模式：IRQ_RISING 或 IRQ_FALLING
};
 
 // ========== 引脚 ID 验证辅助函数 ========== 
 
 // RA8D1 224-pin BGA 封装的引脚有效位掩码表
 // 每个 Port 使用 16 位掩码，bit N 表示 Pin N 是否有效
 // 例如：0xFFFF 表示所有 Pin 0-15 都有效，0xF3FF 表示 Pin 0-11 和 Pin 14-15 有效（Pin 12-13 无效）
 static const uint16_t valid_pin_mask_per_port[] = {
     // Port 0: P000-P011, P014-P015 (缺失 P012, P013)
     0xCFFF,  // 0b1100111111111111
     
     // Port 1: P100-P107, P112-P115 (缺失 P108-P111)
     0xF0FF,  // 0b1111000011111111
     
     // Port 2: P200-P201, P206-P213 (缺失 P202-P205)
     0xFFC3,  // 0b1111111111000011
     
     // Port 3: P300-P312 (全部有效)
     0x1FFF,  // 0b0001111111111111
     
     // Port 4: P400-P415 (全部有效)
     0xFFFF,  // 0b1111111111111111
     
     // Port 5: P500-P515 (全部有效)
     0xFFFF,  // 0b1111111111111111
     
     // Port 6: P600-P607, P609-P615 (缺失 P608)
     0xFEFF,  // 0b1111111011111111
     
     // Port 7: P700-P715 (全部有效)
     0xFFFF,  // 0b1111111111111111
     
     // Port 8: P800-P815 (全部有效)
     0xFFFF,  // 0b1111111111111111
     
     // Port 9: P902-P915 (缺失 P900, P901)
     0xFFFC,  // 0b1111111111111100
     
     // Port A (10): PA00-PA15 (全部有效)
     0xFFFF,  // 0b1111111111111111
     
     // Port B (11): PB00-PB07 (缺失 PB08-PB15)
     0x00FF,  // 0b0000000011111111
     
     // Port C (12): 未使用（根据配置文件中未出现 PCxx）
     0x0000,
     
     // Port D (13): 未使用
     0x0000,
     
     // Port E (14): 未使用
     0x0000,
 };
 
 #define VALID_PIN_MASK_ARRAY_SIZE (sizeof(valid_pin_mask_per_port) / sizeof(valid_pin_mask_per_port[0]))
 
 // 验证引脚 ID 是否符合 RA8D1 的实际物理引脚分布
 static bool pin_id_is_valid(bsp_io_port_pin_t pin_id) {
     uint16_t port = (pin_id >> 8) & 0xFF;
     uint16_t pin = pin_id & 0xFF;
     
     // 检查 Port 范围：0-14 (0x00-0x0E)
     if (port >= VALID_PIN_MASK_ARRAY_SIZE) {
         return false;
     }
     
     // 检查 Pin 范围：0-15
     if (pin > 15) {
         return false;
     }
     
     // 使用位掩码检查该 Pin 是否在允许的掩码内
     uint16_t valid_mask = valid_pin_mask_per_port[port];
     if ((1U << pin) & valid_mask) {
         return true;
     }
     
     return false;
 }
 
 // 将 Python 传入的 id 转换为 bsp_io_port_pin_t
 static bsp_io_port_pin_t pin_id_from_python(mp_obj_t id_in) {
     mp_int_t id = mp_obj_get_int(id_in);
     
     // 直接转换为 bsp_io_port_pin_t 类型
     bsp_io_port_pin_t pin_id = (bsp_io_port_pin_t)id;
     
     // 验证引脚 ID
     if (!pin_id_is_valid(pin_id)) {
         mp_raise_ValueError(MP_ERROR_TEXT("Invalid Pin ID"));
     }
     
     return pin_id;
 }
 
 // ========== Pin ID 到 IRQ 通道映射 ==========

// 引脚中断映射表结构体
typedef struct _pin_irq_map_t {
    bsp_io_port_pin_t pin;     // 引脚 ID
    uint8_t icu_channel;       // 对应的 ICU 通道 (0-15)
} pin_irq_map_t;

// 引脚中断映射表
// 根据 RA8D1 硬件特性配置常用引脚映射
// 注意：需要确保对应的 FSP External IRQ 实例已配置
static const pin_irq_map_t pin_irq_map[] = {
    // 当前支持的映射：P008 -> ICU Channel 12
    {BSP_IO_PORT_00_PIN_08, 12},

    // 示例扩展映射（需要对应的 FSP External IRQ 实例）
    // 以下是常见的中断引脚示例，根据实际硬件配置启用：
    // {BSP_IO_PORT_01_PIN_05, 0},   // P105 -> ICU Channel 0 (示例)
    // {BSP_IO_PORT_04_PIN_00, 1},   // P400 -> ICU Channel 1 (示例)
    // {BSP_IO_PORT_02_PIN_03, 2},   // P203 -> ICU Channel 2 (示例)

    // TODO: 根据实际硬件添加更多映射
    // 需要在 FSP 配置中添加对应的 External IRQ Stack
};

// 获取映射表大小
#define PIN_IRQ_MAP_SIZE (sizeof(pin_irq_map) / sizeof(pin_irq_map_t))

// FSP External IRQ 实例数组
// 索引严格对应 ICU Channel 编号 (0-15)
// 注意：目前只定义了 g_external_irq_s2 (配置为 ICU Channel 12)
// 其他实例需要通过 FSP 配置生成或手动定义
static const external_irq_instance_t * const m_irq_instances[16] = {
    NULL,  // ICU Channel 0
    NULL,  // ICU Channel 1
    NULL,  // ICU Channel 2
    NULL,  // ICU Channel 3
    NULL,  // ICU Channel 4
    NULL,  // ICU Channel 5
    NULL,  // ICU Channel 6
    NULL,  // ICU Channel 7
    NULL,  // ICU Channel 8
    NULL,  // ICU Channel 9
    NULL,  // ICU Channel 10
    NULL,  // ICU Channel 11
    &g_external_irq_s2,  // ICU Channel 12
    NULL,  // ICU Channel 13
    NULL,  // ICU Channel 14
    NULL,  // ICU Channel 15
};

// Pin ID 到 ICU 通道的映射
static int pin_id_to_icu_channel(bsp_io_port_pin_t pin_id) {
    // 遍历映射表查找对应的 ICU 通道
    for (size_t i = 0; i < PIN_IRQ_MAP_SIZE; i++) {
        if (pin_irq_map[i].pin == pin_id) {
            return pin_irq_map[i].icu_channel;
        }
    }

    // 未找到映射
    return -1;
}

// 获取 External IRQ 实例
static const external_irq_instance_t *pin_id_to_external_irq_instance(bsp_io_port_pin_t pin_id) {
    // 先找到对应的 ICU 通道号
    int channel = pin_id_to_icu_channel(pin_id);
    if (channel == -1) {
        return NULL;
    }

    // 从实例数组中获取对应的 FSP 实例
    // ICU Channel 直接作为数组索引
    if (channel >= 0 && channel < 16) {
        return m_irq_instances[channel];
    }

    return NULL;
}

// ========== 中断处理函数 ==========

// FSP External IRQ 回调函数
// 这个函数会被 FSP 的 r_icu_isr 调用
void external_irq_callback(external_irq_callback_args_t *p_args) {
    // 从回调参数中获取上下文
    pin_irq_context_t *ctx = (pin_irq_context_t *)p_args->p_context;

    if (ctx != NULL && ctx->handler != MP_OBJ_NULL) {
        // 只使用调度器，不在中断上下文中直接调用 Python 函数
        // 这可以避免栈溢出和内存管理问题
        #if MICROPY_ENABLE_SCHEDULER
        mp_sched_schedule(ctx->handler, MP_OBJ_FROM_PTR(ctx->pin_obj));
        // 注意：这里我们不检查返回值，因为调度失败不应该导致系统崩溃
        #else
        // 如果没有调度器，我们简单地忽略中断
        // 在中断上下文中调用 Python 是非常危险的
        #endif
    }
}

// ========== 引脚配置辅助函数 ==========
 
// 配置引脚模式和上拉/下拉
static void pin_configure(ra_pin_obj_t *self) {
    uint32_t cfg = 0;
    
    // 设置方向
    if (self->mode == MP_PIN_MODE_OUT) {
        // 推挽输出模式
        cfg |= IOPORT_CFG_PORT_DIRECTION_OUTPUT;
        cfg |= IOPORT_CFG_PORT_OUTPUT_LOW;  // 默认输出低
    } else if (self->mode == MP_PIN_MODE_OPEN_DRAIN) {
        // 开漏输出模式
        cfg |= IOPORT_CFG_PORT_DIRECTION_OUTPUT;
        cfg |= IOPORT_CFG_PORT_OUTPUT_LOW;  // 默认输出低
        cfg |= IOPORT_CFG_NMOS_ENABLE;      // 启用 NMOS 开漏输出
        // 注意：不启用 PMOS_ENABLE，这样只有 NMOS 可以拉低，PMOS 不会拉高
        // 当输出高电平时，NMOS 关闭，引脚处于高阻态 (High-Z)
        // 
        // 开漏模式下的上拉配置：
        // - 如果用户请求了 PULL_UP，IOPORT_CFG_PULLUP_ENABLE 会在下面被添加
        // - 上拉对于开漏输出非常重要：当输出"高"（高阻态）时，上拉电阻确保引脚呈现高电平
        // - 如果没有上拉，开漏输出高电平时引脚将处于浮空状态，电平不确定
    } else {
        // 输入模式
        cfg |= IOPORT_CFG_PORT_DIRECTION_INPUT;
    }
    
    // 设置上拉/下拉
    if (self->pull == MP_PIN_PULL_UP) {
        cfg |= IOPORT_CFG_PULLUP_ENABLE;
        // 注意：在开漏模式下，上拉配置特别重要，用于确保输出高电平时引脚呈现高电平
        // 在推挽输出模式下，上拉是可选的（通常不需要，因为 PMOS 可以直接驱动高电平）
        // 在输入模式下，上拉用于提供默认的高电平状态
    }
    // 注意：FSP 只有上拉选项，下拉需要通过外部硬件实现
    // 如果需要下拉，可能需要使用其他配置选项
    
    // 应用配置
    fsp_err_t err = R_IOPORT_PinCfg(&g_ioport_ctrl, self->pin_id, cfg);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to configure pin: 0x%04X"), 
                         (unsigned int)self->pin_id);
    }
}
 
 // ========== MicroPython Pin 类实现 ==========
 
 static void pin_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
     ra_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
     mp_printf(print, "Pin(0x%04X, mode=%d, pull=%d)", 
              (unsigned int)self->pin_id, self->mode, self->pull);
 }
 
/// \classmethod \constructor(id, mode=-1, pull=-1)
/// 创建 Pin 对象
///   - `id`: 引脚 ID，对应 FSP 的 BSP_IO_PORT_XX_PIN_XX 枚举值（如 0x0A01 表示 P10_01）
///   - `mode`: 模式，Pin.IN, Pin.OUT 或 Pin.OPEN_DRAIN
///   - `pull`: 上拉/下拉，Pin.PULL_NONE, Pin.PULL_UP, Pin.PULL_DOWN
 static mp_obj_t pin_obj_make_new(const mp_obj_type_t *type, size_t n_args,
                                  size_t n_kw, const mp_obj_t *args) {
     // 解析参数
     enum { ARG_id, ARG_mode, ARG_pull };
     static const mp_arg_t allowed_args[] = {
         { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
         { MP_QSTR_mode, MP_ARG_INT, {.u_int = -1} },
         { MP_QSTR_pull, MP_ARG_INT, {.u_int = -1} },
     };
     
     mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
     mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
     
     // 获取引脚 ID
    bsp_io_port_pin_t pin_id = pin_id_from_python(mp_obj_new_int(vals[ARG_id].u_int));     
     
    // 获取模式（默认：输入）
     mp_int_t mode = vals[ARG_mode].u_int;
     if (mode == -1) {
         mode = MP_PIN_MODE_IN;
     } else if (mode != MP_PIN_MODE_IN && mode != MP_PIN_MODE_OUT && mode != MP_PIN_MODE_OPEN_DRAIN) {
         mp_raise_ValueError(MP_ERROR_TEXT("mode must be Pin.IN, Pin.OUT, or Pin.OPEN_DRAIN"));
     }
     
     // 获取上拉/下拉（默认：无）
     mp_int_t pull = vals[ARG_pull].u_int;
     if (pull == -1) {
         pull = MP_PIN_PULL_NONE;
     } else if (pull != MP_PIN_PULL_NONE && pull != MP_PIN_PULL_UP && pull != MP_PIN_PULL_DOWN) {
         mp_raise_ValueError(MP_ERROR_TEXT("pull must be Pin.PULL_NONE, Pin.PULL_UP, or Pin.PULL_DOWN"));
     }
     
    // 创建 Pin 对象
    ra_pin_obj_t *self = m_new_obj(ra_pin_obj_t);
    self->base.type = type;
    self->pin_id = pin_id;
    self->mode = mode;
    self->pull = pull;
    self->irq_ctx = NULL;  // 初始化为无中断配置
    
    // 配置引脚
    pin_configure(self);
    
    return MP_OBJ_FROM_PTR(self);
 }
 
 /// \method value([val])
 /// 获取或设置引脚电平
 static mp_obj_t pin_obj_value(size_t n_args, const mp_obj_t *args) {
     ra_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);
     
     if (n_args == 1) {
         // 读取引脚状态
         bsp_io_level_t level;
         fsp_err_t err = R_IOPORT_PinRead(&g_ioport_ctrl, self->pin_id, &level);
         if (err != FSP_SUCCESS) {
             mp_raise_msg_varg(&mp_type_RuntimeError,
                              MP_ERROR_TEXT("Failed to read pin: 0x%04X"),
                              (unsigned int)self->pin_id);
         }
         return mp_obj_new_int((mp_int_t)level);
    } else {
        // 设置引脚状态
        mp_int_t val = mp_obj_is_true(args[1]);
        bsp_io_level_t level = val ? BSP_IO_LEVEL_HIGH : BSP_IO_LEVEL_LOW;
        
        // 写入引脚电平
        // 
        // 对于开漏模式 (MP_PIN_MODE_OPEN_DRAIN) 的处理：
        // - 写入 HIGH (1): FSP 硬件自动关闭 NMOS 驱动，引脚进入高阻态 (High-Z)
        //   此时引脚电平由外部上拉电阻（如果配置了 IOPORT_CFG_PULLUP_ENABLE）或外部电路决定
        //   这是符合 MicroPython 开漏输出语义的正确行为：输出"高"对应高阻态
        // - 写入 LOW (0): FSP 硬件使能 NMOS 驱动，引脚被拉低到 GND
        //
        // 注意：FSP 的 R_IOPORT_PinWrite 在引脚配置为开漏模式时，会根据写入的电平值
        // 自动控制 NMOS 的开关状态，无需额外的 API 调用。PMOS 在开漏模式下始终禁用
        // （由 pin_configure 中的 IOPORT_CFG_NMOS_ENABLE 配置保证）。
        //
        // 对于推挽输出模式 (MP_PIN_MODE_OUT)：
        // - 写入 HIGH (1): 引脚输出高电平（通过 PMOS）
        // - 写入 LOW (0): 引脚输出低电平（通过 NMOS）
        fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl, self->pin_id, level);
        if (err != FSP_SUCCESS) {
            mp_raise_msg_varg(&mp_type_RuntimeError,
                             MP_ERROR_TEXT("Failed to write pin: 0x%04X"),
                             (unsigned int)self->pin_id);
        }
        return mp_const_none;
    }
 }
 static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_obj_value_obj, 1, 2, pin_obj_value);
 
/// \method on()
/// 设置引脚为高电平
/// 在开漏模式下，这将使引脚进入高阻态（High-Z），需要上拉电阻才能呈现高电平
static mp_obj_t pin_obj_on(mp_obj_t self_in) {
    ra_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bsp_io_level_t level = BSP_IO_LEVEL_HIGH;
    
    // 写入高电平
    // 对于开漏模式：FSP 硬件自动关闭 NMOS，引脚进入高阻态
    // 对于推挽模式：FSP 硬件使能 PMOS，引脚输出高电平
    fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl, self->pin_id, level);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to write pin: 0x%04X"),
                         (unsigned int)self->pin_id);
    }
    return mp_const_none;
}
 static MP_DEFINE_CONST_FUN_OBJ_1(pin_obj_on_obj, pin_obj_on);
 
 /// \method off()
 /// 设置引脚为低电平
 /// 在开漏模式下，这将使能 NMOS 驱动，将引脚拉低到 GND
 static mp_obj_t pin_obj_off(mp_obj_t self_in) {
     ra_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
     bsp_io_level_t level = BSP_IO_LEVEL_LOW;
     
     // 写入低电平
     // 对于开漏模式：FSP 硬件使能 NMOS，引脚被拉低到 GND
     // 对于推挽模式：FSP 硬件使能 NMOS，引脚输出低电平
     fsp_err_t err = R_IOPORT_PinWrite(&g_ioport_ctrl, self->pin_id, level);
     if (err != FSP_SUCCESS) {
         mp_raise_msg_varg(&mp_type_RuntimeError,
                          MP_ERROR_TEXT("Failed to write pin: 0x%04X"),
                          (unsigned int)self->pin_id);
     }
     return mp_const_none;
 }
 static MP_DEFINE_CONST_FUN_OBJ_1(pin_obj_off_obj, pin_obj_off);
 
// 修改 machine_pin.c 中的 pin_obj_irq 函数


// [修复] micropython/py_port/machine_pin.c 中的 pin_obj_irq 函数

static mp_obj_t pin_obj_irq(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    ra_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // ========== 1. 参数解析 (绝对不能少!) ==========
    enum { ARG_handler, ARG_trigger };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = MP_PIN_IRQ_RISING} },
    };
    
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kwargs, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
    
    // 获取解析出的变量 (之前报错就是因为缺少这两行)
    mp_obj_t handler = vals[ARG_handler].u_obj;
    mp_int_t trigger = vals[ARG_trigger].u_int;
    
    // 验证触发模式
    if (trigger != MP_PIN_IRQ_RISING && trigger != MP_PIN_IRQ_FALLING && trigger != 3 /*BOTH*/) {
        mp_raise_ValueError(MP_ERROR_TEXT("trigger must be IRQ_RISING or IRQ_FALLING"));
    }

    // ========== 2. 获取硬件实例 ==========
    const external_irq_instance_t *p_irq_instance = pin_id_to_external_irq_instance(self->pin_id);
    if (!p_irq_instance) {
        mp_raise_msg_varg(&mp_type_ValueError,
                         MP_ERROR_TEXT("Pin 0x%04X does not support interrupts (no IRQ mapping found)"),
                         (unsigned int)self->pin_id);
    }

    // ========== 3. 配置逻辑 ==========
    
    // 总是先关闭 IRQ 实例，以便重新配置
    p_irq_instance->p_api->close(p_irq_instance->p_ctrl);

    // 处理 Handler 和 Context
    if (handler != MP_OBJ_NULL && handler != mp_const_none) {
        // 验证 handler 是可调用对象
        if (!mp_obj_is_callable(handler)) {
            mp_raise_TypeError(MP_ERROR_TEXT("handler must be callable"));
        }

        if (self->irq_ctx == NULL) {
            self->irq_ctx = m_new(pin_irq_context_t, 1);
        }
        self->irq_ctx->handler = handler;
        self->irq_ctx->pin_obj = self;
        self->irq_ctx->trigger = trigger;
    } else {
        // 如果 handler 为 None，说明是禁用中断，直接返回即可（因为前面已经 Close 了）
        if (self->irq_ctx != NULL) {
             m_del(pin_irq_context_t, self->irq_ctx, 1);
             self->irq_ctx = NULL;
        }
        return mp_const_none;
    }

    // 配置引脚为 IRQ 模式 (设置 ISEL 位)
    // 必须在 Open 之前做
    uint32_t pin_cfg = IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_IRQ_ENABLE;
    if (self->pull == MP_PIN_PULL_UP) pin_cfg |= IOPORT_CFG_PULLUP_ENABLE;
    
    // 调用 FSP API 配置引脚
    R_IOPORT_PinCfg(&g_ioport_ctrl, self->pin_id, pin_cfg);

    // ========== 4. 创建 Shadow Config 并 Open ==========
    
    // 复制 const 配置到临时变量
    external_irq_cfg_t irq_cfg_shadow = *(p_irq_instance->p_cfg); 
    
    // 修改 Trigger
    if (trigger == MP_PIN_IRQ_RISING) {
        irq_cfg_shadow.trigger = EXTERNAL_IRQ_TRIG_RISING;
    } else if (trigger == MP_PIN_IRQ_FALLING) {
        irq_cfg_shadow.trigger = EXTERNAL_IRQ_TRIG_FALLING;
    } else {
        irq_cfg_shadow.trigger = EXTERNAL_IRQ_TRIG_BOTH_EDGE; 
    }

    // 绑定回调函数
    irq_cfg_shadow.p_callback = external_irq_callback;
    irq_cfg_shadow.p_context = self->irq_ctx;

    // 使用新配置重新 Open
    fsp_err_t err = p_irq_instance->p_api->open(p_irq_instance->p_ctrl, &irq_cfg_shadow);
    
    if (err != FSP_SUCCESS) {
         mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("IRQ Open failed: %d"), err);
    }

    // 启用中断
    p_irq_instance->p_api->enable(p_irq_instance->p_ctrl);

    return mp_const_none;
}



 static MP_DEFINE_CONST_FUN_OBJ_KW(pin_obj_irq_obj, 0, pin_obj_irq);
 
// Pin 类的方法字典
static const mp_rom_map_elem_t pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&pin_obj_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&pin_obj_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&pin_obj_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&pin_obj_irq_obj) },
    
    // Pin 模式常量
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(MP_PIN_MODE_IN) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(MP_PIN_MODE_OUT) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(MP_PIN_MODE_OPEN_DRAIN) },
    
    // Pin 上拉/下拉常量
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(MP_PIN_PULL_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(MP_PIN_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(MP_PIN_PULL_DOWN) },
    
    // Pin 中断触发模式常量
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(MP_PIN_IRQ_RISING) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(MP_PIN_IRQ_FALLING) },
};
static MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);
 
 // Pin 类型定义
 MP_DEFINE_CONST_OBJ_TYPE(
     ra_pin_type,
     MP_QSTR_Pin,
     MP_TYPE_FLAG_NONE,
     make_new, pin_obj_make_new,
     print, pin_obj_print,
     locals_dict, &pin_locals_dict
 );