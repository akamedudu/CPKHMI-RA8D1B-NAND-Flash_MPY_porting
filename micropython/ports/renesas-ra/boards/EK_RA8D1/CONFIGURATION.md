# EK-RA8D1 板级配置说明

本文档记录了基于新FSP工程配置的EK-RA8D1板级适配更新。

## 芯片信息
- **MCU**: R7FA8D1BHECBD
- **架构**: ARM Cortex-M85
- **封装**: BGA176

## 时钟配置

基于 `bsp_clock_cfg.h` 的配置：

| 时钟源 | 频率 | 说明 |
|--------|------|------|
| XTAL | 24 MHz | 外部晶振 |
| PLL1 | 960 MHz | 主PLL (24MHz × 160 / 4) |
| PLL1P | 480 MHz | CPU时钟源 |
| PLL1Q | 480 MHz | |
| PLL1R | 240 MHz | USB时钟源 |
| PLL2 | 800 MHz | (24MHz × 100 / 3) |
| PLL2P | 200 MHz | OCTA时钟源 |
| CPUCLK | 480 MHz | CPU时钟 (PLL1P / 1) |
| ICLK | 240 MHz | 系统时钟 (PLL1P / 2) |
| PCLKA | 120 MHz | 外设时钟A (PLL1P / 4) |
| PCLKB | 60 MHz | 外设时钟B (PLL1P / 8) |
| PCLKC | 60 MHz | 外设时钟C (PLL1P / 8) |
| PCLKD | 120 MHz | 外设时钟D (PLL1P / 4) |
| PCLKE | 240 MHz | 外设时钟E (PLL1P / 2) |
| BCLK | 120 MHz | 总线时钟 (PLL1P / 4) |
| FCLK | 60 MHz | Flash时钟 (PLL1P / 8) |
| SCICLK | 60 MHz | SCI时钟 (PLL1P / 8) |
| UCLK | 48 MHz | USB时钟 (PLL1R / 5) |
| OCTACLK | 200 MHz | OSPI时钟 (PLL2P / 1) |

## 内存配置

基于 `ra8d1_ek.ld` 的配置：

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| Code Flash | 0x02000000 | 2 MB | 非安全代码Flash |
| File System | 0x12200000 | 128 KB | MicroPython文件系统 |
| SRAM | 0x22000000 | 896 KB | 非安全用户SRAM |
| Data Flash | 0x40100000 | 12 KB | 数据Flash存储 |
| ID Code | 0x0100A150 | 16 bytes | 芯片ID代码区 |

## 引脚配置

### 主要外设引脚

#### UART (SCI3)
- **RXD**: P408 (SCI3_RXD)
- **TXD**: P409 (SCI3_TXD)
- **用途**: MicroPython REPL控制台
- **波特率**: 115200 bps
- **配置**: hal_data.c中g_uart0使用channel 3

#### SPI0
- **MISO**: P100 (SPI0_MISO)
- **MOSI**: P101 (SPI0_MOSI)
- **RSPCK**: P102 (SPI0_RSPCK)
- **SSL0**: P103 (SPI0_SSL0)

#### SPI1
- **MISO**: P700 (SPI1_MISO)
- **MOSI**: P701 (SPI1_MOSI)
- **RSPCK**: P702 (SPI1_RSPCK)
- **SSL0**: P703 (SPI1_SSL0)

#### I2C2
- **SCL**: P512 (IIC2_SCL)
- **SDA**: P511 (IIC2_SDA)

#### UART (SCI0)
- **RXD**: P410 (SCI0_RXD_MISO)
- **TXD**: P411 (SCI0_TXD_MOSI)

#### UART (SCI7)
- **RXD**: P402 (SCI7_RXD_MISO)
- **TXD**: P401 (SCI7_TXD_MOSI)

#### UART (SCI9)
- **RXD**: P601 (SCI9_RXD_MISO)
- **TXD**: P602 (SCI9_TXD_MOSI)

#### USB FS
- **VBUS**: P407 (USBFS0_VBUS)
- **USB D-**: 专用引脚
- **USB D+**: 专用引脚

#### 调试接口
- **SWDIO**: P210 (DEBUG0_SWDIO)
- **SWCLK**: P211 (DEBUG0_SWCLK)
- **SWO**: P109 (DEBUG0_SWO/TDO)
- **TDI**: P110 (DEBUG0_TDI)
- **TCK**: P300 (DEBUG0_TCK)
- **TMS**: P108 (DEBUG0_TMS)

### 板载资源

#### 用户LED
- **LED1**: PA01 (GPIO Output High)
- **宏定义**: USER_LED
- **控制**: MICROPY_HW_LED_ON/OFF/TOGGLE

#### 用户按键
- **SW1**: P008 (GPIO Input with IRQ12)
- **宏定义**: USER_SW
- **中断**: IRQ12

### SDRAM接口
使用多个引脚连接外部SDRAM，主要引脚包括：
- **地址线**: P301-P313 (A2-A13)
- **数据线**: P601-P615, PA00-PA15, P505-P509, P813 (DQ0-DQ31)
- **控制信号**: 
  - SDCLK: PA09
  - CKE: P113
  - WE: P114
  - RAS: P908
  - CAS: P909
  - SDCS: P115
  - DQM0-DQM3: PA10, P112, PA11, P300

### OSPI (Octo-SPI) Flash
用于连接外部Octo-SPI Flash：
- **数据线**: P100-P106, P800-P804 (SIO0-SIO7)
- **时钟**: P808/P809 (SCLK/SCLKN)
- **片选**: P104 (CS1)
- **其他**: P801 (DQS), P106 (RESET), P600 (RSTO1)

### 以太网 (RMII)
- **MDC**: P401
- **MDIO**: P402
- **TXD_EN**: P405
- **TXD0**: P700
- **TXD1**: P406
- **RXD0**: P702
- **RXD1**: P703
- **RX_ER**: P704
- **REF50CK**: P701

### SD卡 (SDHI1)
- **CLK**: P810
- **CMD**: P811
- **DAT0**: P812
- **DAT1**: P500
- **DAT2**: P501
- **DAT3**: P502
- **CD**: P503 (卡检测)

## 文件更新清单

### 1. pins.csv
重新生成，包含147个引脚定义：
- 标准GPIO引脚 (P000-PB01)
- 板载资源别名 (SW1, USER_LED, LED1, TX, RX)

### 2. ra_gen/RA8D1-EK.csv
从FSP工程导出的完整引脚配置表，包含95行配置。

### 3. mpconfigboard.h
更新了以下配置：
- REPL UART配置（SCI3 on P408/P409）
- LED配置（PA01）
- 时钟频率定义

### 4. ra8d1_ek.ld
更新了链接脚本注释，包含：
- 详细的时钟配置信息
- 内存布局说明
- 地址范围注释

### 5. ra_cfg/fsp_cfg/bsp/bsp_pin_cfg.h
添加了详细的引脚定义注释：
- USER_SW: P008 (IRQ12)
- USER_LED: PA01 (GPIO Output)

### 6. ra_gen/hal_data.h 和 hal_data.c
**重要更新**：添加了Flash HP驱动配置
- 添加了r_flash_hp.h和r_flash_api.h头文件包含
- 定义了g_flash0实例（Flash HP）
- 定义了g_flash0_ctrl和g_flash0_cfg
- 添加了callback_flash回调函数声明

Flash配置详情：
- 驱动类型：Flash HP (High Performance)
- 实例名：g_flash0
- 数据Flash BGO：禁用（阻塞模式）
- 回调函数：callback_flash
- 中断：FCU_FRDYI 和 FCU_FIFERR（当前禁用，使用轮询模式）

### 7. mpconfigboard.mk
添加了全局lib/fsp头文件路径：
- lib/fsp/ra/fsp/inc
- lib/fsp/ra/fsp/inc/api
- lib/fsp/ra/fsp/inc/instances

这样可以访问Flash HP API头文件（r_flash_api.h, r_flash_hp.h）

### 8. common_data.c/h 和 vector_data.c/h
FSP生成的公共数据和中断向量表配置（已从新工程导入）

## MicroPython配置

### 启用的功能
```c
#define MICROPY_PY_MACHINE_UART     (1)
#define MICROPY_PY_MACHINE_SPI      (1)
// I2C会根据引脚定义自动启用
```

### REPL配置
```c
#define MICROPY_HW_UART3_TX         (pin_P409)  // SCI3_TXD
#define MICROPY_HW_UART3_RX         (pin_P408)  // SCI3_RXD
#define MICROPY_HW_UART_REPL        (HW_UART_3)
#define MICROPY_HW_UART_REPL_BAUD   (115200)
```

### LED配置
```c
#define MICROPY_HW_USER_LED         (pin_PA01)
#define MICROPY_HW_LED1             MICROPY_HW_USER_LED
#define MICROPY_HW_LED_ON(pin)      mp_hal_pin_high(pin)
#define MICROPY_HW_LED_OFF(pin)     mp_hal_pin_low(pin)
```

## 编译和烧录

### 编译
```bash
cd ports/renesas-ra
make BOARD=EK_RA8D1
```

### 烧录
使用J-Link、SEGGER或e2studio将生成的固件烧录到板卡。

## 注意事项

1. **SDRAM和OSPI Flash**: 新配置启用了外部SDRAM和Octo-SPI Flash，请确保硬件连接正确。

2. **USB**: USB FS外设已配置，但需要在软件中启用USB支持。

3. **以太网**: RMII接口已配置，可用于网络功能。

4. **中断优先级**: SCI3 UART中断优先级设置为12。

5. **时钟**: CPU运行在480MHz，外设时钟根据功能分配不同频率。

## 版本信息
- FSP版本: 5.x
- 配置日期: 2025-11
- 板卡: EK-RA8D1 (CPKHMI-RA8D1B)

