# 引脚中断扩展指南

本文档说明如何在重构后的 `machine_pin.c` 中添加对更多引脚的中断支持。

## 当前状态

重构后的代码已经支持：
- **映射表驱动**：使用 `pin_irq_map` 数组管理引脚到IRQ的映射
- **实例管理**：使用 `m_irq_instances` 数组管理FSP External IRQ实例
- **动态查找**：`pin_id_to_irq()` 和 `pin_id_to_external_irq_instance()` 函数自动查找映射
- **错误处理**：不支持中断的引脚会抛出清晰的错误信息

## 添加新引脚中断支持的步骤

### 步骤1：在FSP配置中添加External IRQ Stack

1. 打开e2studio的FSP Configuration视图
2. 添加新的**ICU** Stack（如果还没有）
3. 在ICU配置中启用对应的IRQ通道（例如IRQ0、IRQ1等）
4. 配置引脚映射：
   - 选择对应的物理引脚
   - 设置为External IRQ功能
   - 配置触发模式、优先级等参数

### 步骤2：更新映射表

在 `machine_pin.c` 的 `pin_irq_map` 数组中添加新的映射：

```c
static const pin_irq_map_t pin_irq_map[] = {
    // 当前支持的映射
    {BSP_IO_PORT_00_PIN_08, ICU_IRQ12_IRQn},

    // 新增映射示例
    {BSP_IO_PORT_01_PIN_05, ICU_IRQ0_IRQn},   // P105 -> IRQ0
    {BSP_IO_PORT_04_PIN_00, ICU_IRQ1_IRQn},   // P400 -> IRQ1
    {BSP_IO_PORT_02_PIN_03, ICU_IRQ2_IRQn},   // P203 -> IRQ2
};
```

### 步骤3：添加FSP External IRQ实例

当FSP生成新的External IRQ实例时（例如`g_external_irq0`、`g_external_irq1`等），在 `m_irq_instances` 数组中添加对应的指针：

```c
static const external_irq_instance_t * const m_irq_instances[16] = {
    &g_external_irq0,    // IRQ0 (新增)
    &g_external_irq1,    // IRQ1 (新增)
    &g_external_irq2,    // IRQ2 (新增)
    NULL,                // IRQ3
    // ... 其他保持不变
    &g_external_irq_s2,  // IRQ12 (原有)
    // ... 其他
};
```

### 步骤4：处理缺失的实例

如果某些IRQ通道的FSP实例还未生成，可以：
1. 暂时设为 `NULL`（代码会自动报错）
2. 使用条件编译处理：

```c
static const external_irq_instance_t * const m_irq_instances[16] = {
#ifdef G_EXTERNAL_IRQ0
    &g_external_irq0,    // IRQ0
#else
    NULL,                // IRQ0 (未配置)
#endif
    // ...
};
```

## RA8D1常用中断引脚映射

以下是RA8D1芯片常见的中断引脚映射示例：

| 引脚 | IRQ通道 | 描述 | 状态 |
|------|---------|------|------|
| P008 | IRQ12 | 当前支持 | ✓ |
| P105 | IRQ0 | 常用中断引脚 | 示例 |
| P400 | IRQ1 | 常用中断引脚 | 示例 |
| P203 | IRQ2 | 常用中断引脚 | 示例 |
| P014 | IRQ3 | 备用 | - |
| P015 | IRQ4 | 备用 | - |

## 注意事项

1. **硬件限制**：并非所有引脚都支持外部中断功能，请参考RA8D1数据手册
2. **FSP配置**：必须先在FSP中正确配置External IRQ，才能在代码中使用
3. **实例命名**：FSP生成的实例名称通常为`g_external_irq{N}`，其中N为IRQ通道号
4. **优先级**：合理设置中断优先级，避免影响系统关键功能
5. **测试**：添加新映射后，务必测试中断功能是否正常工作

## 故障排除

### 问题：编译错误 "undefined reference to g_external_irqX"

**原因**：FSP中未生成对应的External IRQ实例

**解决**：
1. 检查FSP配置中是否添加了对应的External IRQ Stack
2. 重新生成项目内容（Generate Project Content）
3. 如果仍未生成，检查ICU配置是否正确

### 问题：中断不触发

**原因**：引脚配置或硬件连接问题

**解决**：
1. 检查引脚是否正确配置为IRQ功能
2. 验证硬件连接和信号电平
3. 测试不同的触发模式
4. 检查中断优先级设置

### 问题：Python报错 "Pin XXXX does not support interrupts"

**原因**：引脚未在映射表中注册

**解决**：
1. 在 `pin_irq_map` 数组中添加引脚映射
2. 确保对应的FSP External IRQ实例已配置
3. 在 `m_irq_instances` 数组中添加实例指针
