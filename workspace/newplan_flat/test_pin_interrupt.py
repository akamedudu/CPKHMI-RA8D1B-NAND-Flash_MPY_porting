#!/usr/bin/env python3
"""
测试引脚中断功能的脚本
验证重构后的 machine_pin.c 是否正常工作
"""

import machine
import time

def test_pin_interrupt():
    print("Testing Pin interrupt functionality...")

    # 测试 1: 创建支持中断的引脚 (P008)
    try:
        pin = machine.Pin(0x0008, machine.Pin.IN, machine.Pin.PULL_UP)
        print("✓ Successfully created Pin(0x0008)".format())
    except Exception as e:
        print("✗ Failed to create Pin(0x0008): {}".format(e))
        return False

    # 测试 2: 配置中断
    def interrupt_handler(pin_obj):
        print("Interrupt triggered on pin: 0x{:04X}".format(pin_obj.pin_id))

    try:
        pin.irq(handler=interrupt_handler, trigger=machine.Pin.IRQ_RISING)
        print("✓ Successfully configured interrupt handler")
    except Exception as e:
        print("✗ Failed to configure interrupt: {}".format(e))
        return False

    # 测试 3: 测试不支持中断的引脚
    try:
        pin_invalid = machine.Pin(0x0001, machine.Pin.IN)  # P001 不支持中断
        pin_invalid.irq(handler=interrupt_handler, trigger=machine.Pin.IRQ_RISING)
        print("✗ Should have failed for unsupported pin")
        return False
    except ValueError as e:
        print("✓ Correctly rejected unsupported pin: {}".format(e))
    except Exception as e:
        print("✗ Unexpected error for unsupported pin: {}".format(e))
        return False

    print("All tests passed!")
    return True

if __name__ == "__main__":
    test_pin_interrupt()
