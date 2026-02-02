// Manually created module definitions header
extern const struct _mp_obj_module_t mp_module_gc;
#undef MODULE_DEF_GC
#define MODULE_DEF_GC { MP_ROM_QSTR(MP_QSTR_gc), MP_ROM_PTR(&mp_module_gc) },

extern const struct _mp_obj_module_t mp_module_machine;
#undef MODULE_DEF_MACHINE
#define MODULE_DEF_MACHINE { MP_ROM_QSTR(MP_QSTR_machine), MP_ROM_PTR(&mp_module_machine) },

extern const struct _mp_obj_module_t mp_module_utime;
#undef MODULE_DEF_UTIME
#define MODULE_DEF_UTIME { MP_ROM_QSTR(MP_QSTR_utime), MP_ROM_PTR(&mp_module_utime) },

#define MICROPY_REGISTERED_MODULES \
    MODULE_DEF_GC \
    MODULE_DEF_MACHINE \
    MODULE_DEF_UTIME \
// MICROPY_REGISTERED_MODULES

#define MICROPY_HAVE_REGISTERED_EXTENSIBLE_MODULES 0