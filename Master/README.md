# Master 文件夹 - 历史版本和说明

## 📋 概述

此文件夹包含项目的**初始版本**代码（master.c），用于演示和参考。

## 🔄 演进历史

### v1.0 - 初始版本（当前代码：master.c）

**架构特点：**
- 独立的数据获取程序
- 直接连接 Modbus 从机读取传感器数据
- 每 2 秒读取一次数据并打印到控制台
- 需要单独启动运行

**工作流程：**
```
[master.c] → Modbus RTU (/dev/ttymxc2) → 传感器从机
              ↓
           打印数据到控制台
```

**局限性：**
- ❌ 需要在单独的终端运行
- ❌ 数据仅显示在控制台，未与 UI 集成
- ❌ 如果同时运行 UI 程序，会产生两个独立的 Modbus 连接
- ❌ 资源浪费：CPU 和 I/O 占用较高

---

### v2.0 - 优化版本（当前推荐：LVGL/EnvMonitor）

**架构特点：**
- ✅ 数据获取集成到 UI 程序中
- ✅ 在 `custom.c` 中使用独立的 Modbus 读取线程
- ✅ 数据实时显示在 LVGL UI 界面
- ✅ 只需启动一个程序（UI）
- ✅ 数据同步自动化，无需进程间通信

**工作流程：**
```
[LVGL UI (custom.c)]
        ↓
  [Modbus 读取线程]
        ↓
Modbus RTU (/dev/ttymxc2) → 传感器从机
        ↓
  [UI 实时显示数据]
```

**关键改进：**
- 单一 Modbus 连接，减少资源占用
- 完整的数据验证机制（范围检查）
- 条件变量同步，避免忙轮询
- 数据变化检测，仅在必要时更新 UI
- 支持多屏幕：主屏、相册、天气详情页

---

## 📁 文件说明

| 文件 | 用途 | 状态 |
|------|------|------|
| `master.c` | 初始版本数据获取程序 | 🔒 保留（历史参考） |
| `README.md` | 本文档 | 📄 架构说明 |

---

## 🚀 部署建议

### 推荐方案（生产环境）
```bash
# 启动 UI 程序（自动处理数据获取）
cd LVGL/EnvMonitor
./ui_program
```

**优势：**
- 单一进程，部署简单
- 资源占用最低
- 数据一致性有保证

### 备选方案（开发/调试）
```bash
# 终端1：运行 master.c 验证硬件连接
./master

# 终端2：运行 UI 程序
cd LVGL/EnvMonitor
./ui_program
```

**说明：**
- 两个独立的 Modbus 连接
- 用于验证硬件是否正常工作
- 生产环境不推荐（占用资源）

---

## 🔧 技术细节

### v1.0 vs v2.0 对比

| 指标 | v1.0 (master.c) | v2.0 (LVGL/EnvMonitor) |
|------|-----------------|------------------------|
| Modbus 连接数 | 1（独立） | 1（集成） |
| 总进程数 | 2（需同时运行UI） | 1 |
| 数据更新频率 | 2 秒 | 1 秒 |
| 数据验证 | ❌ | ✅ |
| 条件变量同步 | ❌ | ✅ |
| UI 集成 | ❌ | ✅ |
| CPU 占用 | 中等 | 低 |
| 代码复杂度 | 低 | 中等 |

### v2.0 的关键实现（custom.c）

**1. Modbus 线程**
```c
// 在独立线程中读取数据，避免阻塞 UI
void *modbus_read_thread(void *arg);
```

**2. 数据验证**
```c
// 范围检查，确保数据有效性
bool validate_sensor_data(const sensor_data_t *data);
```

**3. 智能更新**
```c
// 仅当数据变化时才更新 UI
bool sensor_data_changed(const sensor_data_t *old_data, const sensor_data_t *new_data);
```

**4. 条件变量同步**
```c
// 避免忙轮询，降低 CPU 占用
pthread_cond_timedwait(&data_cond, &data_mutex, &timeout);
```

---

## 📚 参考资源

- **数据获取逻辑：** `LVGL/EnvMonitor/custom/custom.c`
- **高级显示功能：** `LVGL/EnvMonitor/custom/advanced_display.c`
- **资源优化：** `LVGL/EnvMonitor/custom/resource_optimizer.c`

---

## ✅ 迁移检查表

如果你是从 v1.0 升级到 v2.0：

- [ ] 确认硬件连接正常（串口设备 `/dev/ttymxc2`）
- [ ] 编译 LVGL UI 程序
- [ ] 启动 UI 程序验证数据获取
- [ ] 检查各传感器数据是否在有效范围
- [ ] 验证多屏幕切换功能
- [ ] 监控 CPU 和内存占用
- [ ] 可选：保留 master.c 用于硬件调试

---

## 💡 未来优化方向

1. **数据持久化** - 将历史数据存储到数据库
2. **远程传输** - 通过网络传输数据到云端
3. **告警机制** - 当数据超出范围时发送告警
4. **配置文件** - 外部配置 Modbus 参数和更新频率

---

**最后更新：2026年2月18日**
**版本：v2.0**
