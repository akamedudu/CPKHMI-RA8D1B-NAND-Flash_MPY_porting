#ifndef BSP_LINKER_INFO_H
#define BSP_LINKER_INFO_H

/*
 * BSP Linker Information for RA8D1
 * This file defines the initialization structures used by the C runtime.
 */

#include <stdint.h>

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/** Type used to store whether an init section type is external or internal. */
typedef union
{
    uint32_t external : 1; ///< If 1, section is for external memory
    uint32_t reserved : 31;
} bsp_init_type_t;

/** Type used for initialization table of .zero sections. */
typedef struct
{
    void            * p_base;    ///< Start address of .zero section
    void            * p_limit;   ///< End address of .zero section
    bsp_init_type_t   type;      ///< Section type (external/internal)
} bsp_init_zero_info_t;

/** Type used for initialization table of .copy sections. */
typedef struct
{
    void const      * p_load;    ///< Source address of data in Flash
    void            * p_base;    ///< Start address of .data section in RAM
    void            * p_limit;   ///< End address of .data section in RAM
    bsp_init_type_t   type;      ///< Section type (external/internal)
} bsp_init_copy_info_t;

/** Type used for nocache table. */
typedef struct
{
    void * p_base;               ///< Start address of nocache section
    void * p_limit;              ///< End address of nocache section
} bsp_init_nocache_info_t;

/** C runtime initialization data structure. */
typedef struct
{
    uint32_t const               zero_count;      ///< Number of zero sections
    bsp_init_zero_info_t const * p_zero_list;     ///< Pointer to zero section list
    uint32_t const               copy_count;      ///< Number of copy sections
    bsp_init_copy_info_t const * p_copy_list;     ///< Pointer to copy section list
    uint32_t const               nocache_count;   ///< Number of nocache sections
    bsp_init_nocache_info_t const * p_nocache_list; ///< Pointer to nocache section list
} bsp_init_info_t;

/***********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/* Declare g_init_info as extern - actual definition is in bsp_linker.c */
extern const bsp_init_info_t g_init_info;

#endif /* BSP_LINKER_INFO_H */