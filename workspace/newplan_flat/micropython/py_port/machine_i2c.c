/*
 * machine_i2c.c - RA8D1 I2C Master Implementation using FSP r_iic_master driver
 *
 * Created on: 2025年12月3日
 * Author: AI Assistant
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "hal_data.h"
#include "bsp_api.h"
#include "machine_i2c.h"
#include "common_data.h"  // Contains g_i2c_master0 instance

// Define STATIC macro
#ifndef STATIC
#define STATIC static
#endif

// ========== Synchronous I2C Transfer Context ==========
// Note: Synchronous state is now encapsulated in ra_i2c_obj_t structure

// ========== FSP I2C Callback Implementation ==========

// I2C Master callback function for FSP driver
void i2c_master_callback(i2c_master_callback_args_t *p_args) {
    // Get I2C object from context
    ra_i2c_obj_t *self = (ra_i2c_obj_t *)p_args->p_context;
    
    // Defensive check (FSP should guarantee non-NULL, but better safe than sorry)
    if (self == NULL) {
        return;
    }
    
    // Store the event and mark transfer as complete
    self->last_event = p_args->event;
    self->transfer_result = FSP_SUCCESS;

    // Mark transfer as complete based on event type
    if (p_args->event == I2C_MASTER_EVENT_RX_COMPLETE ||
        p_args->event == I2C_MASTER_EVENT_TX_COMPLETE) {
        self->transfer_complete = true;
    } else if (p_args->event == I2C_MASTER_EVENT_ABORTED) {
        self->transfer_result = FSP_ERR_ABORTED;
        self->transfer_complete = true;
    }
}

// ========== Synchronous I2C Helper Functions ==========

// Initialize sync context for a new transfer
static void i2c_sync_init(ra_i2c_obj_t *self) {
    self->transfer_complete = false;
    self->transfer_result = FSP_SUCCESS;
    self->last_event = (i2c_master_event_t)0;
}

// Wait for transfer completion with timeout
static fsp_err_t i2c_sync_wait(ra_i2c_obj_t *self, uint32_t timeout_ms) {
    uint32_t start_time = mp_hal_ticks_ms();

    while (!self->transfer_complete) {
        // Check for timeout
        if ((mp_hal_ticks_ms() - start_time) > timeout_ms) {
            return FSP_ERR_TIMEOUT;
        }

        // Allow other MicroPython tasks to run
        MICROPY_EVENT_POLL_HOOK

        // Small delay to prevent busy waiting
        mp_hal_delay_us(100);
    }

    return self->transfer_result;
}

// ========== I2C Object Implementation ==========

// Print I2C object
static void i2c_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2C(%u, freq=%u)", self->i2c_instance->p_cfg->channel, self->freq);
}

// Constructor: I2C(id, freq=100000)
static mp_obj_t i2c_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Parse arguments
    enum { ARG_id, ARG_freq };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_freq, MP_ARG_INT, {.u_int = MP_I2C_FREQ_100K} },
    };

    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

    // Get I2C ID (only support I2C1 for now)
    mp_int_t i2c_id = vals[ARG_id].u_int;
    if (i2c_id != 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Only I2C(1) is supported (IIC1 channel)"));
    }

    // Get frequency
    mp_int_t freq = vals[ARG_freq].u_int;
    if (freq != MP_I2C_FREQ_100K && freq != MP_I2C_FREQ_400K) {
        mp_raise_ValueError(MP_ERROR_TEXT("freq must be 100000 or 400000"));
    }

    // Create I2C object
    ra_i2c_obj_t *self = m_new_obj(ra_i2c_obj_t);
    self->base.type = type;
    self->i2c_instance = &g_i2c_master0;
    self->freq = freq;
    self->is_open = false;
    // Initialize synchronous transfer state
    self->transfer_complete = false;
    self->transfer_result = FSP_SUCCESS;
    self->last_event = (i2c_master_event_t)0;

    // Initialize the I2C driver
    fsp_err_t err = self->i2c_instance->p_api->open(self->i2c_instance->p_ctrl, self->i2c_instance->p_cfg);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to open I2C driver: %d"), err);
    }

    self->is_open = true;

    // Set the callback context to point to self (must pass self as context)
    // This allows the callback to know which I2C object triggered the interrupt
    err = self->i2c_instance->p_api->callbackSet(self->i2c_instance->p_ctrl,
                                               i2c_master_callback,
                                               self, NULL);
    if (err != FSP_SUCCESS) {
        // Close the driver if callback setup failed
        self->i2c_instance->p_api->close(self->i2c_instance->p_ctrl);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to set I2C callback: %d"), err);
    }

    return MP_OBJ_FROM_PTR(self);
}

// Deinitialize I2C (called when object is deleted)
static mp_obj_t i2c_obj_deinit(mp_obj_t self_in) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->is_open) {
        self->i2c_instance->p_api->close(self->i2c_instance->p_ctrl);
        self->is_open = false;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(i2c_obj_deinit_obj, i2c_obj_deinit);

static mp_obj_t i2c_obj_scan(mp_obj_t self_in) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C not initialized"));
    }

    mp_obj_t addr_list = mp_obj_new_list(0, NULL);

    // Scan addresses 0x08 to 0x77
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {

        // [新增] 必须先告诉驱动我们要探测哪个地址！
        fsp_err_t err = self->i2c_instance->p_api->slaveAddressSet(
            self->i2c_instance->p_ctrl, addr, I2C_MASTER_ADDR_MODE_7BIT);

        if (err != FSP_SUCCESS) continue;

        // Try to write 0 bytes
        i2c_sync_init(self);

        // [修改] 最后一个参数改为 false (发送 STOP)，防止总线挂死
        err = self->i2c_instance->p_api->write(self->i2c_instance->p_ctrl, NULL, 0, false);

        if (err == FSP_SUCCESS) {
            // Wait for completion
            err = i2c_sync_wait(self, 10);
            if (err == FSP_SUCCESS) {
                mp_obj_list_append(addr_list, mp_obj_new_int(addr));
            }
        }
    }

    return addr_list;
}

static MP_DEFINE_CONST_FUN_OBJ_1(i2c_obj_scan_obj, i2c_obj_scan);

// Read from I2C device: readfrom(addr, nbytes)
static mp_obj_t i2c_obj_readfrom(size_t n_args, const mp_obj_t *args) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_int_t nbytes = mp_obj_get_int(args[2]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C not initialized"));
    }

    if (nbytes <= 0) {
        return mp_obj_new_bytes(NULL, 0);
    }

    // Allocate buffer for received data
    uint8_t *buf = m_new(uint8_t, nbytes);

    // Set slave address
    fsp_err_t err = self->i2c_instance->p_api->slaveAddressSet(self->i2c_instance->p_ctrl,
                                                             addr,
                                                             I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to set slave address: %d"), err);
    }

    // Start read operation
    i2c_sync_init(self);
    err = self->i2c_instance->p_api->read(self->i2c_instance->p_ctrl, buf, nbytes, true);

    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start I2C read: %d"), err);
    }

    // Wait for completion
    err = i2c_sync_wait(self, 100); // 100ms timeout
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("I2C read timeout or error: %d"), err);
    }

    // Create bytes object from received data
    mp_obj_t result = mp_obj_new_bytes(buf, nbytes);
    m_del(uint8_t, buf, nbytes);

    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_obj_readfrom_obj, 3, 3, i2c_obj_readfrom);

// Write to I2C device: writeto(addr, buf)
static mp_obj_t i2c_obj_writeto(size_t n_args, const mp_obj_t *args) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C not initialized"));
    }

    // Get buffer data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len == 0) {
        return mp_const_none;
    }

    // Set slave address
    fsp_err_t err = self->i2c_instance->p_api->slaveAddressSet(self->i2c_instance->p_ctrl,
                                                             addr,
                                                             I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to set slave address: %d"), err);
    }

    // Start write operation
    i2c_sync_init(self);
    err = self->i2c_instance->p_api->write(self->i2c_instance->p_ctrl,
                                         (uint8_t *)bufinfo.buf,
                                         bufinfo.len, true);

    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start I2C write: %d"), err);
    }

    // Wait for completion
    err = i2c_sync_wait(self, 100); // 100ms timeout
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("I2C write timeout or error: %d"), err);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_obj_writeto_obj, 3, 3, i2c_obj_writeto);

// Read from then write to I2C device: readfrom_mem(addr, memaddr, nbytes, *, addrsize=8)
static mp_obj_t i2c_obj_readfrom_mem(size_t n_args, const mp_obj_t *args) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_int_t memaddr = mp_obj_get_int(args[2]);
    mp_int_t nbytes = mp_obj_get_int(args[3]);
    mp_int_t addrsize = (n_args >= 5) ? mp_obj_get_int(args[4]) : 8;

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C not initialized"));
    }

    if (addrsize != 8 && addrsize != 16) {
        mp_raise_ValueError(MP_ERROR_TEXT("addrsize must be 8 or 16"));
    }

    if (nbytes <= 0) {
        return mp_obj_new_bytes(NULL, 0);
    }

    // Allocate buffer for received data
    uint8_t *buf = m_new(uint8_t, nbytes);

    // Set slave address
    fsp_err_t err = self->i2c_instance->p_api->slaveAddressSet(self->i2c_instance->p_ctrl,
                                                             addr,
                                                             I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to set slave address: %d"), err);
    }

    // For memory read operations, we need to:
    // 1. Write the memory address
    // 2. Restart and read the data

    // Prepare memory address buffer
    uint8_t addr_buf[2];
    if (addrsize == 8) {
        addr_buf[0] = memaddr & 0xFF;
        // Write memory address (no restart)
        i2c_sync_init(self);
        err = self->i2c_instance->p_api->write(self->i2c_instance->p_ctrl, addr_buf, 1, false);
    } else {
        addr_buf[0] = (memaddr >> 8) & 0xFF;
        addr_buf[1] = memaddr & 0xFF;
        // Write memory address (no restart)
        i2c_sync_init(self);
        err = self->i2c_instance->p_api->write(self->i2c_instance->p_ctrl, addr_buf, 2, false);
    }

    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to write memory address: %d"), err);
    }

    // Wait for address write completion
    err = i2c_sync_wait(self, 50);
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Memory address write timeout: %d"), err);
    }

    // Now read the data with restart
    i2c_sync_init(self);
    err = self->i2c_instance->p_api->read(self->i2c_instance->p_ctrl, buf, nbytes, true);

    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start memory read: %d"), err);
    }

    // Wait for read completion
    err = i2c_sync_wait(self, 100);
    if (err != FSP_SUCCESS) {
        m_del(uint8_t, buf, nbytes);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Memory read timeout or error: %d"), err);
    }

    // Create bytes object from received data
    mp_obj_t result = mp_obj_new_bytes(buf, nbytes);
    m_del(uint8_t, buf, nbytes);

    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_obj_readfrom_mem_obj, 4, 5, i2c_obj_readfrom_mem);

// Write to memory of I2C device: writeto_mem(addr, memaddr, buf, *, addrsize=8)
static mp_obj_t i2c_obj_writeto_mem(size_t n_args, const mp_obj_t *args) {
    ra_i2c_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_int_t memaddr = mp_obj_get_int(args[2]);
    mp_int_t addrsize = (n_args >= 5) ? mp_obj_get_int(args[4]) : 8;

    if (!self->is_open) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("I2C not initialized"));
    }

    if (addrsize != 8 && addrsize != 16) {
        mp_raise_ValueError(MP_ERROR_TEXT("addrsize must be 8 or 16"));
    }

    // Get buffer data
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[3], &bufinfo, MP_BUFFER_READ);

    // Set slave address
    fsp_err_t err = self->i2c_instance->p_api->slaveAddressSet(self->i2c_instance->p_ctrl,
                                                             addr,
                                                             I2C_MASTER_ADDR_MODE_7BIT);
    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to set slave address: %d"), err);
    }

    // Calculate total buffer size (address + data)
    size_t total_len = bufinfo.len + (addrsize / 8);
    uint8_t *tx_buf = m_new(uint8_t, total_len);

    // Prepare memory address + data buffer
    if (addrsize == 8) {
        tx_buf[0] = memaddr & 0xFF;
        memcpy(&tx_buf[1], bufinfo.buf, bufinfo.len);
    } else {
        tx_buf[0] = (memaddr >> 8) & 0xFF;
        tx_buf[1] = memaddr & 0xFF;
        memcpy(&tx_buf[2], bufinfo.buf, bufinfo.len);
    }

    // Start write operation
    i2c_sync_init(self);
    err = self->i2c_instance->p_api->write(self->i2c_instance->p_ctrl, tx_buf, total_len, false);

    if (err != FSP_SUCCESS) {
    	m_del(uint8_t, tx_buf, total_len);
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Failed to start memory write: %d"), err);
    }

    // Wait for completion
    err = i2c_sync_wait(self, 100);
    m_del(uint8_t, tx_buf, total_len);


    if (err != FSP_SUCCESS) {
        mp_raise_msg_varg(&mp_type_RuntimeError,
                         MP_ERROR_TEXT("Memory write timeout or error: %d"), err);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_obj_writeto_mem_obj, 4, 5, i2c_obj_writeto_mem);

// ========== I2C Type Definition ==========

// I2C class methods dictionary
static const mp_rom_map_elem_t i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&i2c_obj_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&i2c_obj_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&i2c_obj_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&i2c_obj_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&i2c_obj_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem), MP_ROM_PTR(&i2c_obj_writeto_mem_obj) },
};
static MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);

// I2C type definition
MP_DEFINE_CONST_OBJ_TYPE(
    ra_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, i2c_obj_make_new,
    print, i2c_obj_print,
    locals_dict, &i2c_locals_dict
);
