/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_b_uart_rxi_isr, /* SCI3 RXI (Receive data full) */
            [1] = sci_b_uart_txi_isr, /* SCI3 TXI (Transmit data empty) */
            [2] = sci_b_uart_tei_isr, /* SCI3 TEI (Transmit end) */
            [3] = sci_b_uart_eri_isr, /* SCI3 ERI (Receive error) */
            [4] = r_icu_isr, /* ICU IRQ12 (External pin interrupt 12) */
            [5] = iic_master_rxi_isr, /* IIC1 RXI (Receive data full) */
            [6] = iic_master_txi_isr, /* IIC1 TXI (Transmit data empty) */
            [7] = iic_master_tei_isr, /* IIC1 TEI (Transmit end) */
            [8] = iic_master_eri_isr, /* IIC1 ERI (Transfer error) */
            [9] = spi_b_rxi_isr, /* SPI1 RXI (Receive buffer full) */
            [10] = spi_b_txi_isr, /* SPI1 TXI (Transmit buffer empty) */
            [11] = spi_b_tei_isr, /* SPI1 TEI (Transmission complete event) */
            [12] = spi_b_eri_isr, /* SPI1 ERI (Error) */
            [13] = sci_b_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [14] = sci_b_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [15] = sci_b_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [16] = sci_b_uart_eri_isr, /* SCI9 ERI (Receive error) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI3_RXI,GROUP0), /* SCI3 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TXI,GROUP1), /* SCI3 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TEI,GROUP2), /* SCI3 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI3_ERI,GROUP3), /* SCI3 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_ICU_IRQ12,GROUP4), /* ICU IRQ12 (External pin interrupt 12) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_IIC1_RXI,GROUP5), /* IIC1 RXI (Receive data full) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TXI,GROUP6), /* IIC1 TXI (Transmit data empty) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC1_TEI,GROUP7), /* IIC1 TEI (Transmit end) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC1_ERI,GROUP0), /* IIC1 ERI (Transfer error) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_SPI1_RXI,GROUP1), /* SPI1 RXI (Receive buffer full) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TXI,GROUP2), /* SPI1 TXI (Transmit buffer empty) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SPI1_TEI,GROUP3), /* SPI1 TEI (Transmission complete event) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SPI1_ERI,GROUP4), /* SPI1 ERI (Error) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP5), /* SCI9 RXI (Receive data full) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP6), /* SCI9 TXI (Transmit data empty) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP7), /* SCI9 TEI (Transmit end) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP0), /* SCI9 ERI (Receive error) */
        };
        #endif
        #endif
