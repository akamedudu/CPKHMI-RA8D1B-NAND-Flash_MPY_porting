#ifndef MICROPY_INCLUDED_RA8D1_MACHINE_SPI_H
#define MICROPY_INCLUDED_RA8D1_MACHINE_SPI_H

#include "py/obj.h"
#include "r_spi_api.h"

// SPI frequency constants (common MicroPython SPI baudrates)
#define MP_SPI_FREQ_100K   (100000)
#define MP_SPI_FREQ_500K   (500000)
#define MP_SPI_FREQ_1M     (1000000)
#define MP_SPI_FREQ_2M     (2000000)
#define MP_SPI_FREQ_4M     (4000000)
#define MP_SPI_FREQ_8M     (8000000)

// SPI bit order constants
#define MP_SPI_FIRSTBIT_MSB  (0)
#define MP_SPI_FIRSTBIT_LSB  (1)

// SPI polarity constants
#define MP_SPI_POLARITY_LOW   (0)
#define MP_SPI_POLARITY_HIGH  (1)

// SPI phase constants
#define MP_SPI_PHASE_1EDGE    (0)
#define MP_SPI_PHASE_2EDGE    (1)

// SPI object structure
typedef struct _ra_spi_obj_t {
    mp_obj_base_t base;
    const spi_instance_t *spi_instance;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    bool is_open;
} ra_spi_obj_t;

// Forward declaration of SPI type
extern const mp_obj_type_t ra_spi_type;

#endif // MICROPY_INCLUDED_RA8D1_MACHINE_SPI_H
