#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_I2C_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_I2C_H

#include "py/obj.h"
#include "r_i2c_master_api.h"

// I2C frequency constants
#define MP_I2C_FREQ_100K  (100000)
#define MP_I2C_FREQ_400K  (400000)

// I2C object structure
typedef struct _ra_i2c_obj_t {
    mp_obj_base_t base;
    const i2c_master_instance_t *i2c_instance;
    uint32_t freq;
    bool is_open;
    // Synchronous transfer state (replaces global context)
    volatile bool transfer_complete;
    volatile fsp_err_t transfer_result;
    volatile i2c_master_event_t last_event;
} ra_i2c_obj_t;

// Forward declaration of I2C type
extern const mp_obj_type_t ra_i2c_type;

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_I2C_H
