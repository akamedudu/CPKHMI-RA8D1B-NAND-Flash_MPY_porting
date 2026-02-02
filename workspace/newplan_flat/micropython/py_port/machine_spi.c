/*
 * machine_spi.c - RA8D1 SPI Master Implementation using FSP r_spi_b driver
 *
 * Created on: 2025年12月4日
 * Author: AI Assistant
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "machine_spi.h"
#include "common_data.h"  // Contains g_spi1 instance

// Define STATIC macro
#ifndef STATIC
#define STATIC static
#endif

// ========== Synchronous SPI Transfer Context ==========

// Context structure for synchronous SPI operations
typedef struct _spi_sync_context_t {
    volatile bool transfer_complete;
    volatile fsp_err_t transfer_result;
    volatile spi_event_t last_event;
} spi_sync_context_t;

// Global context for SPI operations (single instance, so we can only have one SPI operation at a time)
static spi_sync_context_t g_spi_sync_ctx = {0};

// Static global variables for runtime SPI configuration in RAM (safe from GC/stack issues)
static spi_cfg_t g_spi_runtime_cfg;
static spi_b_extended_cfg_t g_spi_runtime_ext_cfg;

// ========== FSP SPI Callback Implementation ==========

// SPI callback function for FSP driver
void spi_callback(spi_callback_args_t *p_args) {
    // Store the event and mark transfer as complete
    g_spi_sync_ctx.last_event = p_args->event;
    g_spi_sync_ctx.transfer_result = FSP_SUCCESS;

    // Mark transfer as complete based on event type
    if (p_args->event == SPI_EVENT_TRANSFER_COMPLETE) {
        g_spi_sync_ctx.transfer_complete = true;
    } else if (p_args->event == SPI_EVENT_TRANSFER_ABORTED) {
        g_spi_sync_ctx.transfer_result = FSP_ERR_ABORTED;
        g_spi_sync_ctx.transfer_complete = true;
    } else if (p_args->event >= SPI_EVENT_ERR_MODE_FAULT) {
        // Any error event
        g_spi_sync_ctx.transfer_result = FSP_ERR_TRANSFER_ABORTED;
        g_spi_sync_ctx.transfer_complete = true;
    }
}

// ========== Synchronous SPI Helper Functions ==========

// Initialize sync context for a new transfer
static void spi_sync_init(void) {
    g_spi_sync_ctx.transfer_complete = false;
    g_spi_sync_ctx.transfer_result = FSP_SUCCESS;
    g_spi_sync_ctx.last_event = (spi_event_t)0;
}

// Wait for transfer completion with timeout
static fsp_err_t spi_sync_wait(uint32_t timeout_ms) {
    uint32_t start_time = mp_hal_ticks_ms();

    while (!g_spi_sync_ctx.transfer_complete) {
        // Check for timeout
        if ((mp_hal_ticks_ms() - start_time) > timeout_ms) {
            return FSP_ERR_TIMEOUT;
        }

        // Allow other MicroPython tasks to run
        MICROPY_EVENT_POLL_HOOK

        // Small delay to prevent busy waiting
        mp_hal_delay_us(100);
    }

    return g_spi_sync_ctx.transfer_result;
}

// Helper function to set SPI configuration (baudrate, polarity, phase, bits, firstbit)
static fsp_err_t spi_set_config(ra_spi_obj_t *self) {
    // Close the SPI driver if it's open
    if (self->is_open) {
        self->spi_instance->p_api->close(self->spi_instance->p_ctrl);
        self->is_open = false;
    }

    // Deep copy the original configuration from Flash to RAM
    g_spi_runtime_cfg = *self->spi_instance->p_cfg;

    // Deep copy the extended configuration from Flash to RAM
    g_spi_runtime_ext_cfg = *(spi_b_extended_cfg_t *)self->spi_instance->p_cfg->p_extend;

    // Update the runtime config to point to our RAM copy of extended config
    g_spi_runtime_cfg.p_extend = (void*)&g_spi_runtime_ext_cfg;

    // Update extended configuration based on our parameters
    spi_b_extended_cfg_t *ext_cfg = &g_spi_runtime_ext_cfg;

    // Set baudrate (calculate SPBR value for desired baudrate)
    // Assuming PCLK is 150MHz, calculate divider
    // baudrate = PCLK / (2 * (SPBR + 1) * (BRDV + 1))
    // For simplicity, we'll use approximate values
    uint32_t pclk = 150000000; // 150MHz PCLK
    uint32_t target_baud = self->baudrate;

    // Calculate SPBR (SPI Baud Rate Register)
    // Use BRDV = 0 for simplicity, SPBR = (PCLK / (2 * baudrate)) - 1
    uint32_t spbr = (pclk / (2 * target_baud)) - 1;
    if (spbr > 255) spbr = 255; // Max value

    ext_cfg->spck_div.spbr = spbr;
    ext_cfg->spck_div.brdv = 0;

    // Set polarity and phase
    if (self->polarity == MP_SPI_POLARITY_LOW) {
        if (self->phase == MP_SPI_PHASE_1EDGE) {
            g_spi_runtime_cfg.clk_polarity = SPI_CLK_POLARITY_LOW;
            g_spi_runtime_cfg.clk_phase = SPI_CLK_PHASE_EDGE_ODD;
        } else {
            g_spi_runtime_cfg.clk_polarity = SPI_CLK_POLARITY_LOW;
            g_spi_runtime_cfg.clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
        }
    } else {
        if (self->phase == MP_SPI_PHASE_1EDGE) {
            g_spi_runtime_cfg.clk_polarity = SPI_CLK_POLARITY_HIGH;
            g_spi_runtime_cfg.clk_phase = SPI_CLK_PHASE_EDGE_ODD;
        } else {
            g_spi_runtime_cfg.clk_polarity = SPI_CLK_POLARITY_HIGH;
            g_spi_runtime_cfg.clk_phase = SPI_CLK_PHASE_EDGE_EVEN;
        }
    }

    // Set bit order
    if (self->firstbit == MP_SPI_FIRSTBIT_MSB) {
        g_spi_runtime_cfg.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    } else {
        g_spi_runtime_cfg.bit_order = SPI_BIT_ORDER_LSB_FIRST;
    }

    // For now, only support 8 bits
    if (self->bits != 8) {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    // Re-open with new configuration
    fsp_err_t err = self->spi_instance->p_api->open(self->spi_instance->p_ctrl, &g_spi_runtime_cfg);
    if (err != FSP_SUCCESS) {
        return err;
    }

    self->is_open = true;

    // Set the callback context
    err = self->spi_instance->p_api->callbackSet(self->spi_instance->p_ctrl,
                                               spi_callback,
                                               NULL, NULL);
    if (err != FSP_SUCCESS) {
        self->spi_instance->p_api->close(self->spi_instance->p_ctrl);
        self->is_open = false;
        return err;
    }

    return FSP_SUCCESS;
}

// ========== SPI Object Implementation ==========

// Print SPI object
static void spi_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(%u, baudrate=%u, polarity=%u, phase=%u, bits=%u, firstbit=%s)",
              self->spi_instance->p_cfg->channel,
              self->baudrate, self->polarity, self->phase, self->bits,
              self->firstbit == MP_SPI_FIRSTBIT_MSB ? "MSB" : "LSB");
}

// Constructor: SPI(id, baudrate=1000000, polarity=0, phase=0, bits=8, firstbit=MSB)
static mp_obj_t spi_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Parse arguments
    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = MP_SPI_FREQ_1M} },
        { MP_QSTR_polarity, MP_ARG_INT, {.u_int = MP_SPI_POLARITY_LOW} },
        { MP_QSTR_phase, MP_ARG_INT, {.u_int = MP_SPI_PHASE_1EDGE} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_INT, {.u_int = MP_SPI_FIRSTBIT_MSB} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

    // Get SPI ID (only support SPI1 for now)
    mp_int_t spi_id = vals[ARG_id].u_int;
    if (spi_id != 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Only SPI(1) is supported (SPI1 channel)"));
    }

    // Get parameters
    mp_int_t baudrate = vals[ARG_baudrate].u_int;
    mp_int_t polarity = vals[ARG_polarity].u_int;
    mp_int_t phase = vals[ARG_phase].u_int;
    mp_int_t bits = vals[ARG_bits].u_int;
    mp_int_t firstbit = vals[ARG_firstbit].u_int;

    // Validate parameters
    if (polarity != MP_SPI_POLARITY_LOW && polarity != MP_SPI_POLARITY_HIGH) {
        mp_raise_ValueError(MP_ERROR_TEXT("polarity must be 0 or 1"));
    }
    if (phase != MP_SPI_PHASE_1EDGE && phase != MP_SPI_PHASE_2EDGE) {
        mp_raise_ValueError(MP_ERROR_TEXT("phase must be 0 or 1"));
    }
    if (bits != 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("Only 8 bits per frame is supported"));
    }
    if (firstbit != MP_SPI_FIRSTBIT_MSB && firstbit != MP_SPI_FIRSTBIT_LSB) {
        mp_raise_ValueError(MP_ERROR_TEXT("firstbit must be 0 (MSB) or 1 (LSB)"));
    }

    // Create SPI object
    ra_spi_obj_t *self = m_new_obj(ra_spi_obj_t);
    self->base.type = type;
    self->spi_instance = &g_spi1;
    self->baudrate = baudrate;
    self->polarity = polarity;
    self->phase = phase;
    self->bits = bits;
    self->firstbit = firstbit;
    self->is_open = false;

    // Initialize the SPI driver with the specified configuration
    fsp_err_t err = spi_set_config(self);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to initialize SPI driver: %d"), err);
    }

    return MP_OBJ_FROM_PTR(self);
}

// Deinitialize SPI (called when object is deleted)
static mp_obj_t spi_obj_deinit(mp_obj_t self_in) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->is_open) {
        self->spi_instance->p_api->close(self->spi_instance->p_ctrl);
        self->is_open = false;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(spi_obj_deinit_obj, spi_obj_deinit);

// Initialize SPI with new parameters
static mp_obj_t spi_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    // Parse keyword arguments
    enum { ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

    // Update parameters if provided
    bool config_changed = false;
    if (vals[ARG_baudrate].u_int != -1) {
        self->baudrate = vals[ARG_baudrate].u_int;
        config_changed = true;
    }
    if (vals[ARG_polarity].u_int != -1) {
        self->polarity = vals[ARG_polarity].u_int;
        config_changed = true;
    }
    if (vals[ARG_phase].u_int != -1) {
        self->phase = vals[ARG_phase].u_int;
        config_changed = true;
    }
    if (vals[ARG_bits].u_int != -1) {
        self->bits = vals[ARG_bits].u_int;
        config_changed = true;
    }
    if (vals[ARG_firstbit].u_int != -1) {
        self->firstbit = vals[ARG_firstbit].u_int;
        config_changed = true;
    }

    // Validate parameters
    if (self->polarity != MP_SPI_POLARITY_LOW && self->polarity != MP_SPI_POLARITY_HIGH) {
        mp_raise_ValueError(MP_ERROR_TEXT("polarity must be 0 or 1"));
    }
    if (self->phase != MP_SPI_PHASE_1EDGE && self->phase != MP_SPI_PHASE_2EDGE) {
        mp_raise_ValueError(MP_ERROR_TEXT("phase must be 0 or 1"));
    }
    if (self->bits != 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("Only 8 bits per frame is supported"));
    }
    if (self->firstbit != MP_SPI_FIRSTBIT_MSB && self->firstbit != MP_SPI_FIRSTBIT_LSB) {
        mp_raise_ValueError(MP_ERROR_TEXT("firstbit must be 0 (MSB) or 1 (LSB)"));
    }

    // Reconfigure if parameters changed
    if (config_changed) {
        fsp_err_t err = spi_set_config(self);
        if (err != FSP_SUCCESS) {
            mp_raise_msg_varg(&mp_type_RuntimeError,
                             MP_ERROR_TEXT("Failed to reconfigure SPI: %d"), err);
        }
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(spi_obj_init_obj, 1, spi_obj_init);

// Read from SPI (returns bytes)
static mp_obj_t spi_obj_read(size_t n_args, const mp_obj_t *args) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t nbytes = mp_obj_get_int(args[1]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("SPI not initialized"));
    }

    if (nbytes <= 0) {
        return mp_obj_new_bytes(NULL, 0);
    }

    // Allocate buffer for received data
    uint8_t *buf = m_new(uint8_t, nbytes);

    // Prepare dummy transmit buffer (all zeros)
    uint8_t *tx_buf = m_new(uint8_t, nbytes);
    memset(tx_buf, 0, nbytes);

    // Start read operation (write dummy data, read real data)
    spi_sync_init();
    fsp_err_t err = self->spi_instance->p_api->writeRead(self->spi_instance->p_ctrl,
                                                        tx_buf, buf, nbytes, SPI_BIT_WIDTH_8_BITS);

    // Free transmit buffer immediately (SPI operation is asynchronous)
    m_del(uint8_t, tx_buf, nbytes);

    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start SPI read: %d"), err);
    }

    // Wait for completion
    err = spi_sync_wait(1000); // 1000ms timeout
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("SPI read timeout or error: %d"), err);
    }

    // Create bytes object from received data
    mp_obj_t result = mp_obj_new_bytes(buf, nbytes);
    m_del(uint8_t, buf, nbytes);

    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_obj_read_obj, 2, 2, spi_obj_read);

// Read into buffer
static mp_obj_t spi_obj_readinto(size_t n_args, const mp_obj_t *args) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("SPI not initialized"));
    }

    if (bufinfo.len == 0) {
        return mp_const_none;
    }

    // Prepare dummy transmit buffer (all zeros)
    uint8_t *tx_buf = m_new(uint8_t, bufinfo.len);
    memset(tx_buf, 0, bufinfo.len);

    // Start read operation (write dummy data, read into buffer)
    spi_sync_init();
    fsp_err_t err = self->spi_instance->p_api->writeRead(self->spi_instance->p_ctrl,
                                                        tx_buf, (uint8_t *)bufinfo.buf, bufinfo.len, SPI_BIT_WIDTH_8_BITS);

    // Free transmit buffer immediately (SPI operation is asynchronous)
    m_del(uint8_t, tx_buf, bufinfo.len);

    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start SPI readinto: %d"), err);
    }

    // Wait for completion
    err = spi_sync_wait(1000); // 1000ms timeout
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("SPI readinto timeout or error: %d"), err);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_obj_readinto_obj, 2, 2, spi_obj_readinto);

// Write to SPI
static mp_obj_t spi_obj_write(size_t n_args, const mp_obj_t *args) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("SPI not initialized"));
    }

    // Get buffer data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len == 0) {
        return mp_const_none;
    }

    // Allocate dummy receive buffer
    uint8_t *rx_buf = m_new(uint8_t, bufinfo.len);

    // Start write operation (write data, ignore received data)
    spi_sync_init();
    fsp_err_t err = self->spi_instance->p_api->writeRead(self->spi_instance->p_ctrl,
                                                        (uint8_t *)bufinfo.buf, rx_buf, bufinfo.len, SPI_BIT_WIDTH_8_BITS);

    // Free receive buffer immediately (SPI operation is asynchronous)
    m_del(uint8_t, rx_buf, bufinfo.len);

    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start SPI write: %d"), err);
    }

    // Wait for completion
    err = spi_sync_wait(1000); // 1000ms timeout
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("SPI write timeout or error: %d"), err);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_obj_write_obj, 2, 2, spi_obj_write);

// Write and read simultaneously
static mp_obj_t spi_obj_write_readinto(size_t n_args, const mp_obj_t *args) {
    ra_spi_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("SPI not initialized"));
    }

    // Get write buffer
    mp_buffer_info_t write_bufinfo;
    mp_get_buffer_raise(args[1], &write_bufinfo, MP_BUFFER_READ);

    // Get read buffer
    mp_buffer_info_t read_bufinfo;
    mp_get_buffer_raise(args[2], &read_bufinfo, MP_BUFFER_WRITE);

    if (write_bufinfo.len != read_bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("write and read buffers must be the same length"));
    }

    if (write_bufinfo.len == 0) {
        return mp_const_none;
    }

    // Start write-read operation
    spi_sync_init();
    fsp_err_t err = self->spi_instance->p_api->writeRead(self->spi_instance->p_ctrl,
                                                        (uint8_t *)write_bufinfo.buf,
                                                        (uint8_t *)read_bufinfo.buf,
                                                        write_bufinfo.len, SPI_BIT_WIDTH_8_BITS);

    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start SPI write_readinto: %d"), err);
    }

    // Wait for completion
    err = spi_sync_wait(1000); // 1000ms timeout
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("SPI write_readinto timeout or error: %d"), err);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_obj_write_readinto_obj, 3, 3, spi_obj_write_readinto);

// ========== SPI Type Definition ==========

// SPI class methods dictionary
static const mp_rom_map_elem_t spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&spi_obj_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&spi_obj_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&spi_obj_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&spi_obj_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&spi_obj_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&spi_obj_write_readinto_obj) },
};
static MP_DEFINE_CONST_DICT(spi_locals_dict, spi_locals_dict_table);

// SPI type definition
MP_DEFINE_CONST_OBJ_TYPE(
    ra_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, spi_obj_make_new,
    print, spi_obj_print,
    locals_dict, &spi_locals_dict
);
