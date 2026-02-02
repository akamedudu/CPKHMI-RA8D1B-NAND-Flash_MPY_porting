"""
测试 DWT 高精度微秒计时功能
Test DWT high precision microsecond timing
"""

import utime
from utime import ticks_diff  # 导入 ticks_diff 函数用于计算时间差

def test_dwt_timing():
    """
    测试 DWT 高精度微秒计时功能的主函数
    测试包括：ticks_ms(), ticks_us(), ticks_cpu() 以及高精度延时功能
    """
    print("Test DWT high precision microsecond timing")
    print("=" * 40)

    # 首先检查 utime 模块中可用的方法
    print("Available methods in utime module:")
    try:
        # 获取 utime 模块中所有不以 '_' 开头的属性（即公共方法）
        methods = [attr for attr in dir(utime) if not attr.startswith('_')]
        print("  Methods: {}".format(methods))
    except:
        print("  Could not list methods")

    # 测试基本功能
    print("\nBasic functionality test:")

    # 测试 ticks_ms() - 毫秒级计时，应该始终可用
    try:
        print("  Testing ticks_ms()...")
        start_ms = utime.ticks_ms()  # 记录开始时间（毫秒）
        utime.sleep_ms(100)  # 延时 100 毫秒
        end_ms = utime.ticks_ms()  # 记录结束时间（毫秒）
        elapsed_ms = ticks_diff(end_ms, start_ms)  # 计算时间差，自动处理溢出
        print("  ticks_ms() works: {} ms elapsed".format(elapsed_ms))
    except AttributeError as e:
        print("  ERROR: ticks_ms() not available: {}".format(e))
        return
    except Exception as e:
        print("  ERROR in ticks_ms(): {}".format(e))
        return

    # 测试 ticks_us() - 微秒级计时，这是我们实现的高精度功能
    print("  Testing ticks_us()...")
    try:
        start_us = utime.ticks_us()  # 记录开始时间（微秒）
        utime.sleep_ms(100)  # 延时 100 毫秒
        end_us = utime.ticks_us()  # 记录结束时间（微秒）
        elapsed_us = ticks_diff(end_us, start_us)  # 计算时间差
        print("  ticks_us() works: {} microseconds elapsed".format(elapsed_us))
        # 计算误差：预期是 100000 微秒（100ms），计算实际误差
        print("  Error from 100ms: {:.2f} ms".format(abs(elapsed_us - 100000) / 1000))
    except AttributeError as e:
        print("  ERROR: ticks_us() not available: {}".format(e))
        return
    except Exception as e:
        print("  ERROR in ticks_us(): {}".format(e))
        return

    # 测试 ticks_cpu() - CPU 周期计数，使用 DWT 周期计数器
    print("  Testing ticks_cpu()...")
    try:
        start_cpu = utime.ticks_cpu()  # 记录开始时的 CPU 周期数
        utime.sleep_ms(100)  # 延时 100 毫秒
        end_cpu = utime.ticks_cpu()  # 记录结束时的 CPU 周期数
        elapsed_cpu = ticks_diff(end_cpu, start_cpu)  # 计算 CPU 周期差
        print("  ticks_cpu() works: {} CPU ticks elapsed".format(elapsed_cpu))

        # 验证 CPU ticks 与微秒的关系
        # CPU 频率为 480MHz，所以每微秒 = 480 个 CPU 周期
        expected_cpu_ticks = elapsed_us * 480
        print("  Expected CPU ticks (us * 480): {}".format(expected_cpu_ticks))
        if expected_cpu_ticks > 0:
            # 计算精度误差百分比
            error_percent = abs(elapsed_cpu - expected_cpu_ticks) / expected_cpu_ticks * 100
            print("  Precision error: {:.2f}%".format(error_percent))
    except AttributeError as e:
        print("  ERROR: ticks_cpu() not available: {}".format(e))
        return
    except Exception as e:
        print("  ERROR in ticks_cpu(): {}".format(e))
        return

    # 测试高精度微秒级延时
    print("\nTest high precision microsecond delay:")
    delays = [1, 10, 50, 100, 500]  # 要测试的延时时间（微秒）

    for delay_us in delays:
        try:
            start = utime.ticks_us()  # 记录开始时间
            utime.sleep_us(delay_us)  # 执行指定时长的微秒级延时
            end = utime.ticks_us()  # 记录结束时间
            actual_delay = ticks_diff(end, start)  # 计算实际延时时间
            print("    {:3d}us delay: actual {:3d}us".format(delay_us, actual_delay))
        except Exception as e:
            print("    {:3d}us delay: ERROR - {}".format(delay_us, e))

    print("\nDWT timing test completed!")

if __name__ == "__main__":
    # 如果直接运行此脚本，执行测试函数
    test_dwt_timing()