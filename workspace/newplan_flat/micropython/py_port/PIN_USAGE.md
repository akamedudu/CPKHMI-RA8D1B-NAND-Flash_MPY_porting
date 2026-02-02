# MicroPython RA8D1 Port - Pin 类使用说明

## 概述

本端口为瑞萨 RA8D1 (Cortex-M85) 开发板实现了标准的 MicroPython `machine.Pin` 类，支持 GPIO 引脚的基本输入输出控制。

## 快速开始

### 1. REPL 中使用

将固件烧录到开发板后，通过串口（默认 115200 波特率）连接到 REPL，即可开始使用：

>>> from machine import Pin
>>> led = Pin(0x0A01, Pin.OUT)  # P10_01 (USER LED)
>>> led.on()
>>> led.off()### 2. 引脚编号

引脚 ID 使用 FSP 的 `BSP_IO_PORT_XX_PIN_XX` 枚举值，格式为十六进制：
- `0x0A01` = Port 10, Pin 1 = P10_01
- `0x0008` = Port 0, Pin 8 = P0_08
- `0x0100` = Port 1, Pin 0 = P1_00

**常用引脚：**
- `0x0A01` (P10_01) - USER LED（低电平点亮）

### 3. 基本用法

#### 输出引脚（控制 LED）

from machine import Pin

# 创建输出引脚
led = Pin(0x0A01, Pin.OUT)  # P10_01

# 点亮 LED（低电平有效）
led.off()   # 输出低电平 = 点亮
led.on()    # 输出高电平 = 熄灭

# 使用 value() 方法
led.value(0)  # 低电平
led.value(1)  # 高电平#### 输入引脚（读取按钮）
on
from machine import Pin

# 创建输入引脚，启用上拉
button = Pin(0x0008, Pin.IN, Pin.PULL_UP)  # P0_08 (USER_SW)

# 读取引脚状态
state = button.value()
if state == 0:
    print("Button pressed")
else:
    print("Button released")
#### 翻转引脚状态

led = Pin(0x0A01, Pin.OUT)
led.value(not led.value())  # 翻转状态## API 参考

### 构造函数

Pin(id, mode=-1, pull=-1)**参数：**
- `id` (必需): 引脚 ID，FSP 枚举值（如 `0x0A01`）
- `mode` (可选): 引脚模式
  - `Pin.IN` - 输入模式（默认）
  - `Pin.OUT` - 输出模式
- `pull` (可选): 上拉/下拉配置
  - `Pin.PULL_NONE` - 无上拉下拉（默认）
  - `Pin.PULL_UP` - 内部上拉
  - `Pin.PULL_DOWN` - 下拉（注意：FSP 仅支持硬件上拉）

**示例：**n
# 输入引脚，无上拉
pin1 = Pin(0x0001, Pin.IN)

# 输入引脚，上拉
pin2 = Pin(0x0002, Pin.IN, Pin.PULL_UP)

# 输出引脚
pin3 = Pin(0x0003, Pin.OUT)### 方法

#### `value([val])`

获取或设置引脚电平值。

**参数：**
- `val` (可选): 如果提供，设置引脚电平（0=低, 1=高）
- 如果不提供，返回当前引脚电平

**返回值：**
- 读取时：`0` (低电平) 或 `1` (高电平)
- 设置时：`None`

**示例：**
pin = Pin(0x0A01, Pin.OUT)

# 设置引脚为高电平
pin.value(1)

# 设置引脚为低电平
pin.value(0)

# 读取引脚状态
state = pin.value()
print(f"Pin state: {state}")#### `on()`

设置引脚为高电平（1）。

pin = Pin(0x0A01, Pin.OUT)
pin.on()  # 等同于 pin.value(1)#### `off()`

设置引脚为低电平（0）。
ython
pin = Pin(0x0A01, Pin.OUT)
pin.off()  # 等同于 pin.value(0)#### `irq(handler=None, trigger=IRQ_RISING)`

配置引脚中断（当前未实现，会抛出 `NotImplementedError`）。

# 当前不支持
pin.irq()  # NotImplementedError## 常量

### 引脚模式

- `Pin.IN` - 输入模式
- `Pin.OUT` - 输出模式

### 上拉/下拉

- `Pin.PULL_NONE` - 无上拉下拉
- `Pin.PULL_UP` - 内部上拉
- `Pin.PULL_DOWN` - 下拉（硬件限制）

## 完整示例

### 示例1：LED 闪烁

from machine import Pin
import time

led = Pin(0x0A01, Pin.OUT)  # USER LED (低电平点亮)

while True:
    led.off()      # 点亮
    time.sleep_ms(500)
    led.on()       # 熄灭
    time.sleep_ms(500)### 示例2：按钮控制 LED

from machine import Pin
import time

led = Pin(0x0A01, Pin.OUT)       # LED 输出
button = Pin(0x0008, Pin.IN, Pin.PULL_UP)  # 按钮输入

while True:
    if button.value() == 0:  # 按钮按下（低电平）
        led.off()            # 点亮 LED
    else:
        led.on()             # 熄灭 LED
    time.sleep_ms(10)        # 防抖### 示例3：使用 value() 方法

from machine import Pin

pin = Pin(0x0A01, Pin.OUT)

# 翻转状态
pin.value(not pin.value())

# 读取后设置
current = pin.value()
pin.value(current ^ 1)  # 异或翻转## 注意事项

1. **引脚 ID 格式**：必须使用 FSP 枚举值（十六进制），不是字符串或端口号
2. **LED 极性**：开发板上的 USER LED (P10_01) 是**低电平有效**，`off()` 点亮，`on()` 熄灭
3. **上拉/下拉**：FSP 仅支持硬件上拉，下拉需要通过外部硬件实现
4. **引脚冲突**：确保使用的引脚未配置为外设功能（如 UART、SPI 等）
5. **REPL 使用**：所有示例代码都可以直接在 REPL 中逐行输入执行

## 错误处理

from machine import Pin

# 无效引脚 ID
try:
    pin = Pin(0xFFFF, Pin.OUT)
except ValueError as e:
    print(f"Error: {e}")  # Invalid pin ID: 0xFFFF

# 无效模式
try:
    pin = Pin(0x0A01, 99)  # 无效模式
except ValueError as e:
    print(f"Error: {e}")  # mode must be Pin.IN or Pin.OUT## 引脚查找参考

常用引脚映射（RA8D1 CPKHMI 开发板）：
- `0x0A01` - P10_01 - USER_LED
- `0x0008` - P0_08 - USER_SW

更多引脚定义请参考：
- `ra/fsp/src/bsp/mcu/all/bsp_io.h`
- `ra_cfg/fsp_cfg/bsp/bsp_pin_cfg.h`
- `ra_gen/pin_data.c`

## 故障排除

**问题：引脚无响应**
- 检查引脚是否被其他外设占用
- 确认引脚 ID 是否正确
- 验证 FSP 配置是否正确初始化

**问题：按钮读取不稳定**
- 添加软件防抖
- 检查硬件连接
- 确认上拉/下拉配置

**问题：LED 不亮**
- 确认 LED 极性（低电平有效）
- 检查是否使用了正确的引脚
- 使用 `value()` 读取当前状态验证

## 相关文档

- MicroPython 官方文档：https://docs.micropython.org/
- 瑞萨 FSP API 参考：Renesas FSP 用户手册
- 开发板原理图：RA8D1 CPKHMI 硬件手册