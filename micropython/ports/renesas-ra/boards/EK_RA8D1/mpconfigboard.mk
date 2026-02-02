# 1. 定义芯片系列和具体型号
MCU_SERIES = m85
MCU_VARIANT = R7FA8D1BHECBD
CMSIS_MCU = RA8D1

# 2. 指定链接脚本 (确保目录下有 ra8d1_ek.ld)
LD_FILES = EK_RA8D1/ra8d1_ek.ld

# 3. 指定引脚复用功能定义文件
AF_FILE = boards/ra8d1_af.csv

# 4. 使用板级 FSP 库而不是全局 lib/fsp
# RA8D1 需要使用自己的 FSP 实现
# Note: Makefile uses $(HAL_DIR)/ra/fsp/src/..., so HAL_DIR should be $(BOARD_DIR)
HAL_DIR = $(BOARD_DIR)
STARTUP_FILE = $(BOARD_DIR)/ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/startup.o
SYSTEM_FILE = $(BOARD_DIR)/ra/fsp/src/bsp/cmsis/Device/RENESAS/Source/system.o

# 5. 包含 FSP 头文件路径
# (FSP 6.x 的生成目录结构)
INC += -I$(BOARD_DIR)/ra/fsp/inc
INC += -I$(BOARD_DIR)/ra/fsp/inc/api
INC += -I$(BOARD_DIR)/ra/fsp/inc/instances
INC += -I$(BOARD_DIR)/ra/fsp/src/bsp/cmsis/Device/RENESAS/Include
INC += -I$(BOARD_DIR)/ra/fsp/src/bsp/mcu/all
INC += -I$(BOARD_DIR)/ra_cfg/fsp_cfg
INC += -I$(BOARD_DIR)/ra_cfg/fsp_cfg/bsp
INC += -I$(BOARD_DIR)/ra_gen

# 添加全局 lib/fsp 头文件路径（用于Flash HP等共享API）
INC += -Ilib/fsp/ra/fsp/inc
INC += -Ilib/fsp/ra/fsp/inc/api
INC += -Ilib/fsp/ra/fsp/inc/instances

# 6. 核心编译参数 & 宏定义 (这是解决报错的关键!)
# ---------------------------------------------------
# 架构参数: 强制指定 M85 和 浮点单元
CFLAGS += -mcpu=cortex-m85 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16

# 芯片识别宏:
# -D_RA_ : 基础宏
# -D_RA8D1_ : FSP 内部使用
# -DRA8D1 : MicroPython mpconfigboard_common.h 使用 (解决 I2C redefined 问题)
# -DBSP_MCU_GROUP_RA8D1 : FSP renesas.h 使用 (解决 Unsupported MCU 问题)
CFLAGS += -D_RA8D1_ -DRA8D1

# 架构补丁宏:
# 强制告诉 FSP 这是 Armv8.1-M 架构 (解决 Unsupported Architecture 问题)
CFLAGS += -D__ARM_ARCH_8_1M_MAIN__=1 
CFLAGS += -D__ARM_FEATURE_DSP=1

# 忽略部分 FSP 头文件的告警 (可选，防止 Warning 变 Error)
CFLAGS += -Wno-error=cpp

# MicroPython settings
MICROPY_VFS_FAT = 1

# Enable FSP Flash HP support for RA8D1
USE_FSP_FLASH = 1

# Debug channel (SCI3 for REPL)
CFLAGS += -DDEFAULT_DBG_CH=0

# 7. 编译 FSP 生成的核心配置代码 (关键修复！)
# 必须包含这些文件，否则中断表为空，引脚不工作
SRC_C += \
    $(BOARD_DIR)/ra_gen/common_data.c \
    $(BOARD_DIR)/ra_gen/hal_data.c \
    $(BOARD_DIR)/ra_gen/pin_data.c \
    $(BOARD_DIR)/ra_gen/vector_data.c