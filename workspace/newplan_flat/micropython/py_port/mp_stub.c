// mp_stub.c
#include "py/runtime.h"
#include "py/lexer.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"

#include <stdint.h>

//==================== 基础端口钩子（和 machine 无关） ====================//

// 由于目前没有文件系统，from file 的 lexer 一律报错 ENOENT
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
    return NULL;
}

/*
 * 最简 GC：扫描当前栈区
 * 依赖 hal_entry.c 已正确调用：mp_stack_set_top + mp_stack_set_limit + mp_stack_ctrl_init
 */
void gc_collect(void) {
    gc_collect_start();

    // 把一些“可能在寄存器里的值”压到栈上，增大被扫描到的概率
    volatile uintptr_t regs[16];
    for (size_t i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++) {
        regs[i] = 0;
    }

    // 当前 sp 近似取 regs 的地址（在栈上）
    void **sp = (void **)&regs[0];

    // 栈顶由 mp_stack_set_top 设置，Cortex-M 栈向下增长：top > sp
    void *stack_top = (void *)MP_STATE_THREAD(stack_top);

    if ((uintptr_t)stack_top > (uintptr_t)sp) {
        mp_uint_t n = (mp_uint_t)(((uintptr_t)stack_top - (uintptr_t)sp) / sizeof(void *));
        gc_collect_root(sp, n);
    }

    gc_collect_end();
}

// nlr_jump 失败时调用：一般是致命异常
void nlr_jump_fail(void *val) {
    (void)val;
    mp_hal_stdout_tx_str("FATAL: unhandled exception (nlr_jump_fail)\r\n");
    while (1) {
        __BKPT(0);
    }
}
