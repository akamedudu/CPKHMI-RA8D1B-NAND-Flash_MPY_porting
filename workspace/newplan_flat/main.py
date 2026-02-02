"""
MicroPython RA8D1 - Pin OPEN_DRAIN 模式测试代码

本测试用于验证 P410 引脚 (Arduino D12, BSP_IO_PORT_4_PIN_10 = 0x040A)
在推挽输出和开漏输出模式下的区别。

使用方法：
1. 将本文件上传到开发板或通过 REPL 执行
2. 使用万用表测量 P410 引脚的电压
3. 观察推挽模式和开漏模式下的行为差异

硬件连接建议：
- P410 引脚：连接到万用表的正极
- GND：连接到万用表的负极
- 如需外部上拉：在 P410 和 3.3V 之间连接 10kΩ 电阻（可选，用于观察开漏特性）

测试原理：
- 推挽模式 (Pin.OUT)：引脚可以主动输出高电平（3.3V）或低电平（0V）
- 开漏模式 (Pin.OPEN_DRAIN)：引脚只能主动拉低，高电平状态时引脚呈现高阻态
  需要外部上拉电阻才能输出高电平
"""

from machine import Pin
import time

# P410 引脚定义 (BSP_IO_PORT_4_PIN_10 = 0x040A)
# Port 4 = 0x04, Pin 10 = 0x0A, 组合 = 0x040A
PIN_P410 = 0x040A

print("=" * 60)
print("MicroPython RA8D1 - Pin OPEN_DRAIN 模式测试")
print("=" * 60)
print()
print(f"测试引脚: P410 (引脚 ID: 0x{PIN_P410:04X})")
print()
print("测试步骤：")
print("1. 请使用万用表测量 P410 引脚的电压")
print("2. 引脚对地：黑色表笔接 GND，红色表笔接 P410")
print("3. 观察电压变化")
print()
print("-" * 60)

# ========== 测试 1: 推挽输出模式 ==========
print()
print("【测试 1】推挽输出模式 (Pin.OUT)")
print("-" * 60)
print("创建推挽输出引脚...")
pin_pp = Pin(PIN_P410, Pin.OUT)
print(f"引脚对象: {pin_pp}")

print()
print(">>> 推挽模式 - 输出高电平 (应测量到约 3.3V)")
pin_pp.value(1)
time.sleep(2)
voltage = "约 3.3V" if True else ""  # 实际测量值
print(f"   当前状态: 高电平")
print(f"   预期电压: 约 3.3V")
print(f"   实际测量: [请用万用表测量并记录]")

print()
print(">>> 推挽模式 - 输出低电平 (应测量到约 0V)")
pin_pp.value(0)
time.sleep(2)
print(f"   当前状态: 低电平")
print(f"   预期电压: 约 0V")
print(f"   实际测量: [请用万用表测量并记录]")

print()
print(">>> 推挽模式 - 再次输出高电平")
pin_pp.value(1)
time.sleep(2)
print(f"   当前状态: 高电平")
print(f"   说明: 推挽模式可以主动输出高电平")

time.sleep(3)

# ========== 测试 2: 开漏输出模式（无外部上拉）==========
print()
print("【测试 2】开漏输出模式 (Pin.OPEN_DRAIN) - 无外部上拉")
print("-" * 60)
print("创建开漏输出引脚...")
pin_od = Pin(PIN_P410, Pin.OPEN_DRAIN)
print(f"引脚对象: {pin_od}")

print()
print(">>> 开漏模式 - 输出低电平 (应测量到约 0V)")
pin_od.value(0)
time.sleep(2)
print(f"   当前状态: 低电平")
print(f"   预期电压: 约 0V")
print(f"   实际测量: [请用万用表测量并记录]")

print()
print(">>> 开漏模式 - 输出高电平 (无外部上拉时，应测量到约 0V 或浮空)")
pin_od.value(1)
time.sleep(2)
print(f"   当前状态: 高电平（但实际上引脚为高阻态）")
print(f"   预期电压: 约 0V 或浮空（因为没有外部上拉电阻）")
print(f"   实际测量: [请用万用表测量并记录]")
print(f"   说明: 开漏模式在高电平时引脚为高阻态，无法主动输出高电平")

time.sleep(3)

# ========== 测试 3: 开漏输出模式（带内部上拉）==========
print()
print("【测试 3】开漏输出模式 (Pin.OPEN_DRAIN) - 带内部上拉")
print("-" * 60)
print("创建开漏输出引脚（启用内部上拉）...")
pin_od_pullup = Pin(PIN_P410, Pin.OPEN_DRAIN, Pin.PULL_UP)
print(f"引脚对象: {pin_od_pullup}")

print()
print(">>> 开漏模式 - 输出低电平 (应测量到约 0V，内部上拉被覆盖)")
pin_od_pullup.value(0)
time.sleep(2)
print(f"   当前状态: 低电平")
print(f"   预期电压: 约 0V")
print(f"   实际测量: [请用万用表测量并记录]")

print()
print(">>> 开漏模式 - 输出高电平 (内部上拉，应测量到约 3.3V)")
pin_od_pullup.value(1)
time.sleep(2)
print(f"   当前状态: 高电平（引脚为高阻态，但内部上拉生效）")
print(f"   预期电压: 约 3.3V（由内部上拉电阻提供）")
print(f"   实际测量: [请用万用表测量并记录]")
print(f"   说明: 开漏模式 + 内部上拉 = 类似于推挽输出，但更安全")

time.sleep(3)

# ========== 对比总结 ==========
print()
print("=" * 60)
print("【测试总结】")
print("=" * 60)
print()
print("推挽模式 (Pin.OUT) vs 开漏模式 (Pin.OPEN_DRAIN):")
print()
print("1. 推挽模式:")
print("   - 可以主动输出高电平（3.3V）")
print("   - 可以主动输出低电平（0V）")
print("   - 输出电流能力较强")
print("   - 适用于大多数数字输出场景")
print()
print("2. 开漏模式:")
print("   - 只能主动输出低电平（0V）")
print("   - 高电平状态时为高阻态（无法主动输出高）")
print("   - 需要外部或内部上拉电阻才能输出高电平")
print("   - 适用于 I2C、多主机总线等场景")
print("   - 可以实现线与（wire-AND）功能")
print()
print("3. 实际应用:")
print("   - I2C 总线：必须使用开漏模式")
print("   - 多主机通信：开漏模式更安全")
print("   - LED 驱动：推挽模式更直接")
print("   - 需要线与逻辑：必须使用开漏模式")
print()
print("=" * 60)
print("测试完成！")
print("=" * 60)

