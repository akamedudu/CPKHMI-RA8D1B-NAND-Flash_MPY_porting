#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_ADC_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_ADC_H

#include "py/obj.h"
#include "r_adc_api.h"  // FSP ADC API types (adc_instance_t, adc_cfg_t, etc.)

// ADC object structure
typedef struct _ra_adc_obj_t {
    mp_obj_base_t base;
    const adc_instance_t *adc_instance;
    uint8_t channel;          // ADC channel number (0-15)
    uint8_t unit;            // ADC unit (0 or 1)
    bool is_open;            // Track if ADC unit is open
} ra_adc_obj_t;

// Forward declaration of ADC type
extern const mp_obj_type_t ra_adc_type;

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_ADC_H

