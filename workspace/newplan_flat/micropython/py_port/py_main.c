#include "py/compile.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "py/mphal.h"

static char heap[MICROPY_HEAP_SIZE];

int py_main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    mp_stack_ctrl_init();
    mp_stack_set_top(&argc);
    // mp_stack_set_limit(MICROPY_STACK_LIMIT); // 如果你在 mpconfigport.h 里定义了的话再打开

    gc_init(heap, heap + sizeof(heap));
    mp_init();
    readline_init0();

    mp_hal_stdout_tx_str("MicroPython on RA8D1\r\n");

    for (;;)
    {
        pyexec_friendly_repl();
    }

    mp_deinit();
    return 0;
}
