# FSP ICU Stack 配置说明

本文档说明如何在 e2studio FSP 配置中添加 ICU Stack 以支持 `machine.Pin.irq()` 功能。

## 前提条件

- 已安装 e2studio 和 FSP
- 项目已配置基本的 IOPORT Stack

## 配置步骤

### 1. 打开 FSP 配置视图

在 e2studio 中：
1. 右键点击项目
2. 选择 **FSP Configuration** 或点击 **FSP Configuration** 标签页

### 2. 添加 ICU Stack

1. 在 **Stacks** 面板中，点击 **New Stack** 或 **+** 按钮
2. 选择 **ICU** (Interrupt Controller Unit)
3. 在配置树中选择新添加的 ICU Stack

### 3. 配置 IRQ12（用于 P008 引脚）

1. 在 ICU Stack 的配置中，找到 **External IRQ** 或 **IRQ** 配置项
2. 启用 **IRQ12**（External IRQ12）
3. 配置以下参数：
   - **IRQ Channel**: IRQ12
   - **Pin**: P008 (或根据实际硬件选择)
   - **Trigger Mode**: 可以在代码中动态配置，这里可以先设置为 **Rising Edge** 或 **Falling Edge**
   - **Priority**: 建议设置为 12（中等优先级）
   - **Filter Enable**: 可选，启用可以减少噪声干扰

### 4. 配置引脚功能

1. 在 **Pins** 配置中，找到 **P008** 引脚
2. 将引脚功能设置为 **IRQ12** 或 **External IRQ12**
3. 确保引脚方向为 **Input**

### 5. 生成代码

1. 点击 **Generate Project Content** 按钮（或按 `Ctrl+G`）
2. FSP 会自动生成以下文件：
   - `ra_gen/vector_data.h` - 包含 IRQ12 的 IRQn_Type 定义
   - `ra_gen/vector_data.c` - 包含中断向量表

### 6. 更新代码中的 IRQ 定义

生成代码后，`ra_gen/vector_data.h` 中会包含类似以下的内容：

```c
#define VECTOR_NUMBER_IRQ12 ((IRQn_Type) 4)  // 假设这是第 5 个中断
#define IRQ12_IRQn          ((IRQn_Type) 4)
```

如果生成了 `IRQ12_IRQn` 定义，可以在 `machine_pin.c` 中使用它：

```c
// 在 machine_pin.c 的 pin_id_to_irq 函数中
#include "vector_data.h"  // 添加此包含

static IRQn_Type pin_id_to_irq(bsp_io_port_pin_t pin_id) {
    if (pin_id == BSP_IO_PORT_00_PIN_08) {
        return IRQ12_IRQn;  // 使用 FSP 生成的定义
    }
    return (IRQn_Type)-1;
}
```

### 7. 注册中断服务例程

在 `ra_gen/vector_data.c` 中，需要确保中断服务例程已注册。如果没有自动生成，可以手动添加：

```c
// 在 vector_data.c 中添加
void external_irq12_isr(void);
```

并在中断向量表中注册（通常 FSP 会自动处理）。

## 验证配置

配置完成后，可以通过以下 Python 代码测试：

```python
from machine import Pin

# 创建 Pin 对象（P008）
pin = Pin(0x0008, Pin.IN, Pin.PULL_UP)

# 定义中断回调函数
def irq_handler(pin_obj):
    print("Interrupt triggered on pin:", pin_obj)

# 配置中断：上升沿触发
pin.irq(handler=irq_handler, trigger=Pin.IRQ_RISING)

# 现在当 P008 引脚出现上升沿时，会触发中断并调用 irq_handler
```

## 故障排除

### 问题：中断不触发

如果按下按键后没有反应，请检查以下几点：

1. **确认引脚配置**
   - 在 FSP Pin Configuration 中，确保 P008 已配置为 IRQ12 功能
   - 确认引脚方向为 Input

2. **检查硬件连接**
   - 确认按键正确连接到 P008 引脚
   - 确认按键有上拉或下拉电阻（代码中使用了 `Pin.PULL_UP`）
   - 测试引脚电平：`pin.value()` 应该能读取到当前电平

3. **检查触发模式**
   - 如果使用 `IRQ_RISING`，确保按键按下时引脚从低电平变为高电平
   - 如果使用 `IRQ_FALLING`，确保按键按下时引脚从高电平变为低电平
   - 可以尝试切换触发模式测试

4. **调试步骤**
   ```python
   from machine import Pin
   
   # 测试引脚读取
   pin = Pin(0x0008, Pin.IN, Pin.PULL_UP)
   print("Pin value:", pin.value())  # 应该能读取到 0 或 1
   
   # 配置中断
   def irq_handler(pin_obj):
       print("IRQ triggered!")
   
   pin.irq(handler=irq_handler, trigger=Pin.IRQ_RISING)
   
   # 手动触发测试（如果支持）
   # 或者尝试不同的触发模式
   pin.irq(handler=irq_handler, trigger=Pin.IRQ_FALLING)
   ```

5. **检查 MicroPython 调度器**
   - 确保 `MICROPY_ENABLE_SCHEDULER` 已启用
   - 中断回调通过 `mp_sched_schedule` 调度，REPL 会自动处理

6. **检查 FSP 初始化**
   - 确认 External IRQ 实例已正确初始化
   - 代码会自动检查并打开 External IRQ（如果未打开）

7. **常见问题排查**
   - **问题**: 按下按键没有反应
     - **检查1**: 确认引脚配置正确
       ```python
       pin = Pin(0x0008, Pin.IN, Pin.PULL_UP)
       print("Pin value:", pin.value())  # 应该能读取到当前电平
       ```
     - **检查2**: 尝试不同的触发模式
       ```python
       # 如果 IRQ_RISING 不工作，尝试 IRQ_FALLING
       pin.irq(handler=irq_handler, trigger=Pin.IRQ_FALLING)
       ```
     - **检查3**: 确认按键硬件连接
       - 按键一端接 P008，另一端接 GND
       - 使用 `Pin.PULL_UP` 时，按键按下应该是低电平（0），释放是高电平（1）
       - 因此 `IRQ_RISING` 应该在按键释放时触发，`IRQ_FALLING` 在按下时触发
     - **检查4**: 测试引脚电平变化
       ```python
       # 循环读取引脚状态，手动按下按键观察变化
       import time
       for i in range(20):
           print("Pin:", pin.value())
           time.sleep_ms(100)
       ```

8. **调试技巧**
   - 可以在回调函数中添加更多输出：
     ```python
     def irq_handler(pin_obj):
         print("IRQ triggered!")
         print("Pin ID:", hex(pin_obj.pin_id))
     ```
   - 检查中断是否真的触发（可以在 C 代码中添加调试输出，但需要重新编译）

## 注意事项

1. **IRQ 通道限制**: RA8D1 支持多个外部 IRQ 通道（IRQ0-IRQ15），但每个引脚只能映射到一个 IRQ 通道。

2. **引脚映射**: 不是所有引脚都支持外部中断功能。需要查阅 RA8D1 硬件手册确认引脚是否支持 IRQ 功能。

3. **中断优先级**: 建议将外部中断优先级设置为中等优先级（例如 12），避免影响系统关键中断。

4. **滤波器**: 如果引脚连接的是机械开关等可能产生抖动的信号源，建议启用滤波器（Filter Enable）。

5. **向量表大小**: 添加新的中断后，`BSP_ICU_VECTOR_NUM_ENTRIES` 会自动更新。

## 扩展支持更多引脚

要支持更多引脚的中断功能：

1. 在 FSP 中为每个引脚配置对应的 IRQ 通道
2. 在 `machine_pin.c` 的 `pin_id_to_irq()` 函数中添加映射关系
3. 为每个 IRQ 创建对应的中断服务例程（或使用通用的回调机制）

## 故障排除

### 问题：编译错误 "R_ICU undeclared"

**解决方案**: 确保包含了正确的头文件。`R_ICU` 定义在 `R7FA8D1BH.h` 中，通常通过 `bsp_api.h` 间接包含。如果仍有问题，可以尝试：

```c
#include "R7FA8D1BH.h"
```

### 问题：中断不触发

**检查清单**:
1. 确认引脚已正确配置为 IRQ 功能
2. 确认 IRQCR 寄存器配置正确（触发模式）
3. 确认中断已启用（`R_BSP_IrqEnable`）
4. 确认中断服务例程已正确注册
5. 检查硬件连接和信号电平

### 问题：Python 回调函数不执行

**可能原因**:
1. `mp_sched_schedule` 返回 false（调度队列已满）
2. MicroPython 调度器未运行
3. 中断上下文中的内存分配问题

**解决方案**: 确保在中断回调中只调用 `mp_sched_schedule`，不要直接调用 Python 函数。

