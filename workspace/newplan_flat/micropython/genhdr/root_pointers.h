// Root pointers header - manually created
#define MICROPY_PORT_ROOT_POINTERS

#if MICROPY_PY_SYS_ARGV
mp_obj_list_t mp_sys_argv_obj;
#endif

#if MICROPY_PY_SYS_ATTR_DELEGATION
mp_obj_t sys_mutable[MP_SYS_MUTABLE_NUM];
#endif

#if MICROPY_HELPER_REPL
vstr_t * repl_line;
#endif

#if MICROPY_HELPER_REPL
const char *readline_hist[MICROPY_READLINE_HISTORY_SIZE];
#endif

#if MICROPY_ENABLE_SCHEDULER
mp_sched_item_t sched_queue[MICROPY_SCHEDULER_DEPTH];
#endif
