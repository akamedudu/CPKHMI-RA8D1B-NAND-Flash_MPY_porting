// C:\Users\demon\Desktop\newplan\workspace\newplan_flat\micropython\mphalport.h

#ifndef MICROPY_INCLUDED_MPHALPORT_H
#define MICROPY_INCLUDED_MPHALPORT_H

#include "hal_data.h"
#include "py/mpconfig.h"

// 这些函数在一个单独的 C 文件里实现
mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);
mp_uint_t mp_hal_ticks_cpu(void);

void mp_hal_delay_ms(mp_uint_t ms);
void mp_hal_delay_us(mp_uint_t us);

// 中断字符设置函数（在 mp_stub.c 中实现）
void mp_hal_set_interrupt_char(int c);

#endif // MICROPY_INCLUDED_MPHALPORT_H
