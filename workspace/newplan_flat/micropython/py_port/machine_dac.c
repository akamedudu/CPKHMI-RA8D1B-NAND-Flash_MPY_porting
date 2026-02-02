/*
 * machine_dac.c - RA8D1 DAC Implementation using Direct Register Access
 *
 * Created on: 2025
 * Author: Embedded Systems Engineer
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "machine_pin.h"
#include "machine_dac.h"
// R_DAC register definitions are included via bsp_api.h -> renesas.h -> R7FA8D1BH.h

// Define STATIC macro
#ifndef STATIC
#define STATIC static
#endif

// Helper macro for manual analog pin configuration
// Use R_BSP_PinAccessEnable/Disable for PFS register access
#define RA_SETUP_ANALOG_PIN(port, pin) \
    do { \
        R_BSP_PinAccessEnable(); \
        R_PFS->PORT[port].PIN[pin].PmnPFS_b.PMR = 0; \
        R_PFS->PORT[port].PIN[pin].PmnPFS_b.ASEL = 1; \
        R_BSP_PinAccessDisable(); \
    } while(0)

// Pin to DAC channel mapping
typedef struct {
    bsp_io_port_pin_t pin;
    uint8_t channel;        // DAC channel (0 or 1)
} pin_dac_map_t;

// Pin to DAC channel mapping table (only P014 and P015)
static const pin_dac_map_t pin_dac_map[] = {
    {BSP_IO_PORT_00_PIN_14, 0},   // P014 -> DAC Channel 0
    {BSP_IO_PORT_00_PIN_15, 1},   // P015 -> DAC Channel 1
};

#define PIN_DAC_MAP_SIZE (sizeof(pin_dac_map) / sizeof(pin_dac_map_t))

// Find DAC channel for a given pin
static const pin_dac_map_t *find_dac_channel(bsp_io_port_pin_t pin_id) {
    for (size_t i = 0; i < PIN_DAC_MAP_SIZE; i++) {
        if (pin_dac_map[i].pin == pin_id) {
            return &pin_dac_map[i];
        }
    }
    return NULL;
}

// Get pin ID from Python object (Pin object or string)
static bsp_io_port_pin_t get_pin_id_from_obj(mp_obj_t pin_in) {
    // Check if it's a Pin object
    if (mp_obj_is_type(pin_in, &ra_pin_type)) {
        ra_pin_obj_t *pin_obj = MP_OBJ_TO_PTR(pin_in);
        return pin_obj->pin_id;
    }
    
    // Check if it's a string like "P014"
    if (mp_obj_is_str(pin_in)) {
        size_t len;
        const char *str = mp_obj_str_get_data(pin_in, &len);
        
        // Parse "P014" format: P + port (2 digits) + pin (2 digits)
        if (len >= 4 && str[0] == 'P') {
            int port = 0, pin = 0;
            if (len == 4) {
                // P014 format: P + 0 + 14
                port = (str[1] - '0') * 10 + (str[2] - '0');
                pin = (str[3] - '0') * 10 + (str[4] - '0');
            } else if (len == 5) {
                // P014 format: P + 0 + 1 + 4
                port = (str[1] - '0') * 10 + (str[2] - '0');
                pin = (str[3] - '0') * 10 + (str[4] - '0');
            }
            
            if (port >= 0 && port <= 15 && pin >= 0 && pin <= 15) {
                return (bsp_io_port_pin_t)((port << 8) | pin);
            }
        }
    }
    
    // Try as integer (direct pin ID)
    mp_int_t pin_id_int = mp_obj_get_int(pin_in);
    return (bsp_io_port_pin_t)pin_id_int;
}

// ========== DAC Object Implementation ==========

static void dac_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    ra_dac_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "DAC(channel=%u)", self->channel);
}

// Constructor: DAC(pin)
static mp_obj_t dac_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Parse arguments
    enum { ARG_id };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);
    
    // Get pin ID
    bsp_io_port_pin_t pin_id = get_pin_id_from_obj(vals[ARG_id].u_obj);
    
    // Find DAC channel mapping
    const pin_dac_map_t *map = find_dac_channel(pin_id);
    if (map == NULL) {
        mp_raise_msg_varg(&mp_type_ValueError,
                         MP_ERROR_TEXT("Pin 0x%04X is not a valid DAC pin (only P014 and P015 are supported)"),
                         (unsigned int)pin_id);
    }
    
    // Create DAC object
    ra_dac_obj_t *self = m_new_obj(ra_dac_obj_t);
    self->base.type = type;
    self->channel = map->channel;
    self->is_initialized = false;
    
    // Initialize DAC hardware
    // 1. Unlock registers
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SAR);
    
    // 2. Enable module clock (MSTPD20 = 0 enables DAC12)
    R_MSTP->MSTPCRD_b.MSTPD20 = 0;
    
    // 3. Manual pin configuration to Analog Mode
    uint8_t port = (pin_id >> 8) & 0xFF;
    uint8_t pin = pin_id & 0xFF;
    RA_SETUP_ANALOG_PIN(port, pin);
    
    // 4. Configure DAC
    // Set Data Format to Right-Aligned (Default - DADPR.DPSEL = 0, which is default)
    // No need to modify DADPR as right-aligned is default
    
    // Enable Output Amplifier for this channel
    if (self->channel == 0) {
        // Channel 0: Bit 6 of DAAMPCR
        R_DAC->DAAMPCR_b.DAAMP0 = 1;
    } else {
        // Channel 1: Bit 7 of DAAMPCR
        R_DAC->DAAMPCR_b.DAAMP1 = 1;
    }
    
    // Enable DAC Output for this channel
    if (self->channel == 0) {
        // Channel 0: Bit 6 of DACR (DAOE0)
        R_DAC->DACR_b.DAOE0 = 1;
        // Also enable DAE (DAC Enable) bit 5
        R_DAC->DACR_b.DAE = 1;
    } else {
        // Channel 1: Bit 7 of DACR (DAOE1)
        R_DAC->DACR_b.DAOE1 = 1;
        // Also enable DAE (DAC Enable) bit 5
        R_DAC->DACR_b.DAE = 1;
    }
    
    // 5. Lock registers
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SAR);
    
    self->is_initialized = true;
    
    return MP_OBJ_FROM_PTR(self);
}

// write(value) - Write 8-bit value (0-255), scale to 12-bit and write to DAC
static mp_obj_t dac_obj_write(mp_obj_t self_in, mp_obj_t value_in) {
    ra_dac_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    if (!self->is_initialized) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("DAC not initialized"));
    }
    
    // Get 8-bit value (0-255)
    mp_int_t value_8bit = mp_obj_get_int(value_in);
    if (value_8bit < 0 || value_8bit > 255) {
        mp_raise_ValueError(MP_ERROR_TEXT("DAC value must be 0-255"));
    }
    
    // Scale to 12-bit (0-4095)
    uint16_t value_12bit = (uint16_t)(((uint32_t)value_8bit * 4095) / 255);
    
    // Write to DAC data register
    // Unlock registers for write
//    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_SAR);
    
    // DADR is an array: DADR[0] for channel 0, DADR[1] for channel 1
    R_DAC->DADR[self->channel] = value_12bit;
    
    // Lock registers
//    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_SAR);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(dac_obj_write_obj, dac_obj_write);

// DAC class methods dictionary
static const mp_rom_map_elem_t dac_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&dac_obj_write_obj) },
};
static MP_DEFINE_CONST_DICT(dac_locals_dict, dac_locals_dict_table);

// DAC type definition
MP_DEFINE_CONST_OBJ_TYPE(
    ra_dac_type,
    MP_QSTR_DAC,
    MP_TYPE_FLAG_NONE,
    make_new, dac_obj_make_new,
    print, dac_obj_print,
    locals_dict, &dac_locals_dict
);

