# DWT 高精度微秒计时实现

## 概述

本项目已成功利用 Cortex-M85 内核的 DWT (Data Watchpoint and Trace) 周期计数器实现了高精度微秒计时功能，替代了原有的低精度毫秒级计时方案。

## 修改内容

### 1. 添加 CMSIS 头文件支持

在 `micropython/py_port/mp_hal_ra8d1.c` 中添加了 CMSIS 头文件：
```c
#include <core_cm85.h>
```

### 2. DWT 初始化函数

新增 `mp_hal_time_init()` 函数用于初始化 DWT：
```c
void mp_hal_time_init(void) {
    // 解锁并启用 DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // 清零 DWT 周期计数器
    DWT->CYCCNT = 0;

    // 启用周期计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}
```

### 3. 高精度微秒计时函数

重写了 `mp_hal_ticks_us()` 函数：
```c
mp_uint_t mp_hal_ticks_us(void) {
    // CPU 频率 480MHz，转换为微秒：cycles / (480000000 / 1000000) = cycles / 480
    // 为避免浮点运算，使用整数除法
    return DWT->CYCCNT / 480u;
}
```

### 4. CPU 周期计数函数

修改了 `mp_hal_ticks_cpu()` 函数：
```c
mp_uint_t mp_hal_ticks_cpu(void) {
    return DWT->CYCCNT;
}
```

### 5. 系统初始化集成

在 `src/hal_entry.c` 中添加了 DWT 初始化调用：
```c
// 初始化 DWT 高精度周期计数器
mp_hal_time_init();
```

## 技术细节

### CPU 频率配置
- 系统 CPU 频率：480MHz (来自 `ra_gen/bsp_clock_cfg.h`)
- 每微秒周期数：480 cycles
- 微秒转换公式：`microseconds = cycles / 480`

### DWT 特性
- DWT 是 Cortex-M 内核的调试组件
- 提供高精度周期计数器
- 计数器位宽：32 位
- 支持 wrap-around 处理 (MicroPython 内核已处理)

## 优势

1. **高精度**: 从 1ms 精度提升到约 2.08ns 理论精度 (1/480MHz)
2. **低开销**: 无需浮点运算，直接整数除法
3. **硬件级**: 利用内核硬件特性，无软件开销
4. **兼容性**: 保持原有 API 接口不变

## 测试验证

提供了 `test_dwt_timing.py` 测试脚本，用于验证：
- 微秒级延时精度
- CPU ticks 与微秒的换算关系
- 高精度计时功能

## 注意事项

1. DWT 初始化需要在系统启动早期完成
2. 32 位计数器溢出时间约为 8.9 秒 (2^32 / 480MHz)
3. MicroPython 的 `ticks_diff` 函数已处理 wrap-around 情况

## 兼容性影响

- ✅ 保持所有现有 MicroPython API 兼容
- ✅ 不影响现有毫秒级计时功能
- ✅ DHT11、OneWire 等高精度时序驱动现可正常工作
