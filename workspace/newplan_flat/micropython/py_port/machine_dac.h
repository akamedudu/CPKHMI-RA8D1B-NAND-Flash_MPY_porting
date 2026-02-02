#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_DAC_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_DAC_H

#include "py/obj.h"

// DAC object structure
typedef struct _ra_dac_obj_t {
    mp_obj_base_t base;
    uint8_t channel;          // DAC channel (0 for P014, 1 for P015)
    bool is_initialized;      // Track if DAC is initialized
} ra_dac_obj_t;

// Forward declaration of DAC type
extern const mp_obj_type_t ra_dac_type;

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_DAC_H

