# module_drivers FreeRTOS 集成与代码优化方案

## 执行概览

本方案将现有的裸机驱动转变为 FreeRTOS 多任务系统，并优化关键驱动代码以提升性能和可维护性。

---

## 一、现状问题诊断

### 1.1 阻塞式延时问题（严重）
**文件**：`System/Delay.c`
**问题代码**：
```c
void Delay_us(uint32_t us) {
    uint32_t start = TIM_GetCounter(TIM2);
    while ((TIM_GetCounter(TIM2) - start) < us);  // CPU空转，无法中断
}
```

**影响**：
- AHT10 读取：80ms 阻塞
- BH1750 读取：120ms 阻塞
- 总循环延迟：~200ms 以上
- Modbus 响应延迟不确定（被挤压在阻塞时间内）
- 无法充分利用 FreeRTOS 的抢占式调度

### 1.2 FreeRTOS 配置存在但未启用
**文件**：`User/main.c`
**问题**：
```c
// 这些代码被注释了
// xTaskCreate(SensorTask, "SensorTask", 128, NULL, 1, NULL);
// vTaskStartScheduler();
```

**影响**：
- FreeRTOS 占用 75KB 内存但不工作
- 错失多任务并发的机会
- 无法使用任务间通信、互斥量等机制

### 1.3 数据安全性问题
**文件**：`System/mb_user.c`、`User/main.c`
**问题**：
```c
// 主循环更新
aht10_get_data(&data);
// 同时 Modbus 回调读取
eMBRegInputCB() {
    *pucRegBuffer++ = (UCHAR)(data.temperature >> 8);
}
// 无互斥量保护，可能读到半更新的数据
```

### 1.4 驱动优化空间
- **I²C**：软件模拟，可改为硬件 I²C
- **SPI**：轮询式收发，可使用 DMA
- **UART**：同上，逐字符轮询发送

---

## 二、改进目标

### 2.1 架构改造
| 目标 | 现状 | 目标 |
|-----|------|------|
| 任务调度 | 无任务 | 3-4 个任务 + FreeRTOS 调度 |
| 延时方式 | CPU 空转 | vTaskDelay + 中断驱动 |
| 数据保护 | 无保护 | 互斥量保护共享数据 |
| Modbus 响应 | 不确定 | <50ms 级别 |
| CPU 利用率 | 低（大量空等） | 高（任务间切换） |

### 2.2 性能指标
- **Modbus 响应时间**：从不确定降至 10-50ms
- **传感器更新频率**：1Hz（每秒采集一次）
- **系统响应能力**：100% 抢占式响应
- **内存使用**：堆内存由 75KB 优化至 40-50KB
- **功耗**：空闲时系统进入低功耗等待

---

## 三、详细改进方案

### 3.1 第一步：创建任务管理模块

**文件**：`System/task_manager.c`、`System/task_manager.h`

**模块职责**：
- 定义所有应用任务
- 初始化任务和同步原语
- 提供任务间通信接口

**代码框架**：
```c
// System/task_manager.h
#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// 任务优先级定义（0=最低，4=最高）
#define PRIO_SENSOR_TASK    1
#define PRIO_MODBUS_TASK    3
#define PRIO_MONITOR_TASK   2

// 任务堆栈大小
#define STACK_SENSOR_TASK   256
#define STACK_MODBUS_TASK   384
#define STACK_MONITOR_TASK  256

// 传感器数据结构（受互斥量保护）
typedef struct {
    float temperature_aht10;
    float humidity_aht10;
    int lux;
    float temperature_bmp280;
    float pressure_bmp280;
    int co2_eq;
    int tvoc;
} SensorData_t;

// 全局数据和同步原语
extern SensorData_t g_sensorData;
extern SemaphoreHandle_t g_sensorDataMutex;
extern QueueHandle_t g_modbusEventQueue;

// 初始化函数
void TaskManager_Init(void);

// 任务函数
void SensorTask(void *pvParameters);
void ModbusTask(void *pvParameters);
void MonitorTask(void *pvParameters);

// 数据获取函数（带互斥量）
void SensorData_Lock(void);
void SensorData_Unlock(void);
SensorData_t SensorData_GetSnapshot(void);

#endif
```

### 3.2 第二步：重构 Delay 模块

**文件**：`System/Delay.c`（重构）

**新实现**：
```c
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"

// 初始化系统滴答定时器（由 FreeRTOS 自动配置）
void Delay_Init(void) {
    // FreeRTOS 会自动初始化 SysTick
    // 配置频率由 FreeRTOSConfig.h 中的 configTICK_RATE_HZ 决定
    // 默认 1000Hz（1ms 一次）
}

// 微秒级延时（小延时，保留为轮询式以避免任务切换开销）
void Delay_us(uint32_t us) {
    uint32_t start = TIM_GetCounter(TIM2);
    while ((TIM_GetCounter(TIM2) - start) < us);
    // 仅用于 <10us 的很小延时
}

// 毫秒级延时（改用 FreeRTOS，允许其他任务运行）
void Delay_ms(uint32_t ms) {
    if (ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

// 秒级延时
void Delay_s(uint32_t s) {
    Delay_ms(s * 1000);
}

// 用于中断上下文的非阻塞等待（返回 pdTRUE 如果时间已过）
BaseType_t Delay_IsExpired(uint32_t *pStartTick, uint32_t delayMs) {
    if ((xTaskGetTickCount() - *pStartTick) >= pdMS_TO_TICKS(delayMs)) {
        return pdTRUE;
    }
    return pdFALSE;
}
```

**关键点**：
- `Delay_us()` 保留为轮询式，仅用于 <10us 的硬件时序
- `Delay_ms()` 改用 `vTaskDelay()`，允许任务切换
- 新增 `Delay_IsExpired()` 用于状态机式延时

### 3.3 第三步：创建数据保护机制

**文件**：`System/task_manager.c`（关键部分）

```c
// 全局传感器数据（受保护）
static SensorData_t g_sensorData = {0};
static SemaphoreHandle_t g_sensorDataMutex = NULL;

// 初始化
void TaskManager_Init(void) {
    // 创建互斥量保护传感器数据
    g_sensorDataMutex = xSemaphoreCreateMutex();
    configASSERT(g_sensorDataMutex != NULL);

    // 创建队列用于 Modbus 事件
    g_modbusEventQueue = xQueueCreate(10, sizeof(uint8_t));
    configASSERT(g_modbusEventQueue != NULL);

    // 创建各任务
    BaseType_t ret;

    ret = xTaskCreate(
        SensorTask,
        "SensorTask",
        STACK_SENSOR_TASK,
        NULL,
        PRIO_SENSOR_TASK,
        NULL
    );
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(
        ModbusTask,
        "ModbusTask",
        STACK_MODBUS_TASK,
        NULL,
        PRIO_MODBUS_TASK,
        NULL
    );
    configASSERT(ret == pdPASS);
}

// 数据获取（加锁）
SensorData_t SensorData_GetSnapshot(void) {
    SensorData_t snapshot;
    xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY);
    memcpy(&snapshot, &g_sensorData, sizeof(SensorData_t));
    xSemaphoreGive(g_sensorDataMutex);
    return snapshot;
}

// 数据更新（加锁）
void SensorData_UpdateTemperature(float temp_aht10, float temp_bmp280) {
    xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY);
    g_sensorData.temperature_aht10 = temp_aht10;
    g_sensorData.temperature_bmp280 = temp_bmp280;
    xSemaphoreGive(g_sensorDataMutex);
}
```

### 3.4 第四步：重写主程序任务

**文件**：`User/main.c`（关键改变）

```c
#include "FreeRTOS.h"
#include "task.h"
#include "task_manager.h"

// 传感器采集任务
void SensorTask(void *pvParameters) {
    AHT10_Data aht10_data;
    BMP280_DataTypedef bmp280_data;
    int lux, co2_eq, tvoc;

    // 初始化传感器
    aht10_init();
    BH1750_Init();
    BMP280_Init(&bmp280_config);
    sgp30_data_show_init();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1Hz 采样

    while(1) {
        // 采集传感器数据（可能会因为 vTaskDelay 而让出 CPU）
        aht10_get_data(&aht10_data);  // 现在可以被 Modbus 任务中断

        lux = BH1750_ReadLight();     // 120ms，期间 Modbus 可运行

        BMP280_GetData(&bmp280_data);

        SGP30_MeasureIAQ(&co2_eq, &tvoc);

        // 更新全局数据（加锁保护）
        SensorData_UpdateTemperature(aht10_data.temperature, bmp280_data.temperature);
        SensorData_UpdateHumidity(aht10_data.humidity);
        SensorData_UpdateLight(lux);
        SensorData_UpdatePressure(bmp280_data.pressure);
        SensorData_UpdateGas(co2_eq, tvoc);

        // 任务延迟到下一个周期（1000ms）
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Modbus 通信任务
void ModbusTask(void *pvParameters) {
    // 初始化 Modbus
    eMBInit(MB_RTU, 1, 1, 115200, MB_PAR_NONE);
    eMBEnable();

    USART1_SendString("Modbus Started\r\n");

    while(1) {
        // 高优先级处理 Modbus，确保快速响应
        eMBPoll();

        // 给其他低优先级任务一点运行时间
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// 监控任务（可选，用于系统指示）
void MonitorTask(void *pvParameters) {
    while(1) {
        // 闪烁 LED 或输出调试信息
        UART5_SendString("System Running\r\n");

        vTaskDelay(pdMS_TO_TICKS(5000));  // 每 5 秒一次
    }
}

// 主程序
int main(void) {
    // 硬件初始化（在调度器启动前）
    SystemInit();
    IIC1_Init();
    UART5_Init();
    USART1_Init(115200);

    UART5_SendString("=== System Starting ===\r\n");

    // 初始化任务管理器（创建所有任务）
    TaskManager_Init();

    UART5_SendString("=== Scheduler Starting ===\r\n");

    // 启动 FreeRTOS 调度器
    vTaskStartScheduler();

    // 不应该到达这里
    while(1);

    return 0;
}
```

### 3.5 第五步：更新 Modbus 回调

**文件**：`System/mb_user.c`（优化）

```c
#include "task_manager.h"

// 输入寄存器回调（读传感器数据）
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNRegs) {
    // 获取当前传感器数据的快照（线程安全）
    SensorData_t data = SensorData_GetSnapshot();

    if (usAddress == 0x0000 && usNRegs == 1) {
        // 温度（AHT10）
        int16_t temp = (int16_t)(data.temperature_aht10 * 100);
        pucRegBuffer[0] = (UCHAR)(temp >> 8);
        pucRegBuffer[1] = (UCHAR)(temp & 0xFF);
        return MB_ENOERR;
    }

    // ... 其他寄存器 ...

    return MB_ENOREG;  // 寄存器不存在
}
```

**优点**：
- 每次读取时获取数据快照
- 使用互斥量确保线程安全
- 避免读取半更新的数据

### 3.6 第六步：驱动优化（可选但推荐）

#### 3.6.1 硬件 I²C 替代软件 I²C
**文件**：`Hardware/I2C1_HW.c`（新建）

```c
// 使用硬件 I²C1（100kHz），替代软件模拟
void I2C1_HW_Init(void) {
    // 初始化 I²C1（PA8=SCL, PC9=SDA）或（PB6=SCL, PB7=SDA）
    // 配置频率：100kHz
    // 中断驱动而不是轮询
}
```

**优势**：
- CPU 占用从 ~50% 降至 <5%
- 自动处理时序，更稳定
- 可使用中断驱动

#### 3.6.2 SPI DMA 支持
**文件**：`Hardware/SPI1_DMA.c`（新建）

```c
// 为 BMP280 SPI 读取使用 DMA
// 代替轮询式 MySPI_SwapByte()
```

#### 3.6.3 UART 发送缓冲
**文件**：`Hardware/USART1_RingBuffer.c`（新建）

```c
// 使用环形缓冲 + DMA 发送
// 避免逐字符轮询等待
```

---

## 四、改进步骤顺序（推荐）

### 第 1 阶段：基础架构（必须）
1. ✅ 创建 `task_manager.c/h`（实现互斥量和任务创建）
2. ✅ 重构 `Delay.c`（改用 vTaskDelay）
3. ✅ 重写 `User/main.c`（启用任务调度器）
4. ✅ 更新 `mb_user.c`（数据获取加锁）

**预期时间**：1-2 小时
**测试方法**：
```bash
# 编译并烧录
# 观察 UART5 输出
# 验证 Modbus 响应时间（用示波器或上位机）
# 检查 USART1 通信正常
```

### 第 2 阶段：驱动优化（可选，提升性能）
1. 实现硬件 I²C（替换软件 I²C）
2. 添加 SPI DMA 支持
3. 添加 UART 环形缓冲

**预期时间**：2-3 小时
**性能提升**：CPU 占用 50% → 20%

### 第 3 阶段：增强功能（可选）
1. 添加看门狗（独立 IWDG）
2. 添加低功耗睡眠模式
3. 添加 OTA 固件更新

---

## 五、代码文件清单

### 新增/修改文件
```
新增：
  System/task_manager.h          # 任务管理头文件
  System/task_manager.c          # 任务管理实现
  System/sensor_data.h           # 传感器数据结构定义
  System/sensor_data.c           # 传感器数据保护函数

修改：
  System/Delay.c                 # 改用 vTaskDelay
  System/Delay.h                 # 新增 Delay_IsExpired()
  System/mb_user.c               # 数据获取加锁
  User/main.c                    # 多任务改造，启用调度器

可选：
  Hardware/I2C1_HW.c/h           # 硬件 I²C 实现
  Hardware/SPI1_DMA.c/h          # SPI DMA 实现
  Hardware/USART1_RingBuffer.c/h # UART 环形缓冲
```

---

## 六、测试验证清单

### 6.1 编译测试
- [ ] 无编译错误
- [ ] 无警告
- [ ] 可执行文件大小合理（<256KB）

### 6.2 功能测试
- [ ] UART5 调试输出正常
- [ ] USART1 Modbus 通信正常
- [ ] 传感器数据采集正确
- [ ] Modbus 寄存器读取返回正确数据

### 6.3 性能测试
- [ ] Modbus 响应时间 <50ms
- [ ] 系统可以立即响应 Modbus 请求（不等待传感器读取）
- [ ] 没有数据撕裂（传感器数据一致性）

### 6.4 稳定性测试
- [ ] 连续运行 24 小时无异常
- [ ] 堆内存不溢出（用 FreeRTOS 提供的内存监控 API）
- [ ] 栈溢出检查通过

---

## 七、优化效果对比

| 指标 | 改造前 | 改造后 | 改进 |
|-----|--------|--------|------|
| **Modbus 响应时间** | 80-200ms（不确定） | 10-50ms | 4-20x 更快 |
| **CPU 空闲率** | ~0%（CPU 空转） | 50-70% | 能更好处理其他任务 |
| **传感器采集延迟** | 同步，不可中断 | 1Hz 定时，可抢占 | 更规则，更易控制 |
| **代码可维护性** | 混乱，全局变量 | 清晰，任务分离 | 易于扩展和调试 |
| **线程安全性** | 无保护 | 互斥量保护 | 数据一致性有保证 |
| **堆内存使用** | 75KB 未用 | 40-50KB 有效使用 | 更高效 |

---

## 八、参考资源

### FreeRTOS 相关
- 官方文档：https://www.freertos.org/
- API 参考：`FreeRTOS.h`, `task.h`, `semphr.h`
- 示例：`FreeRTOS_CORE/` 中的示例代码

### STM32F4 相关
- STM32F4 参考手册
- HAL 库文档

### Modbus 相关
- FreeModbus 文档：https://github.com/armink/FreeModbus_Slave-Master-RTU-over-TCP-English-
- mb.h 中的函数声明

---

## 九、常见问题解答

### Q1: 启用 FreeRTOS 会增加多少代码量？
**A**: 主要增加在 `task_manager.c` 和重构的模块中，总共约 500-700 行。FreeRTOS 库本身已经包含在编译中，不会额外增加。

### Q2: 堆内存不足怎么办？
**A**: FreeRTOS 配置中的 `configTOTAL_HEAP_SIZE` 可调整。当前 75KB，如果不足：
```c
// FreeRTOSConfig.h
#define configTOTAL_HEAP_SIZE ((size_t)(100*1024))  // 改为 100KB
```

### Q3: 如何监测每个任务的堆栈使用情况？
**A**: 使用 FreeRTOS 的 `uxTaskGetStackHighWaterMark()`:
```c
UBaseType_t stackUsage = uxTaskGetStackHighWaterMark(taskHandle);
UART5_Printf("Task stack high water: %d\r\n", stackUsage);
```

### Q4: 任务优先级如何设置？
**A**: 根据实时性要求：
- **Modbus 任务**：优先级 3（需要快速响应）
- **传感器任务**：优先级 1（不需要立即响应，可延迟）
- **监控任务**：优先级 2（介于两者）

---

## 十、验收标准

该改进方案的成功标志：

1. ✅ **系统能够启动** - 调度器成功启动，不会立即崩溃
2. ✅ **Modbus 通信正常** - 能接收并响应 Modbus 请求
3. ✅ **传感器数据正确** - 采集的数据与直接读取一致
4. ✅ **性能显著提升** - Modbus 响应时间缩短至 <50ms
5. ✅ **代码可维护性提高** - 模块清晰，易于扩展

---

**作者**：Claude Code
**版本**：1.0
**最后更新**：2026年2月
**状态**：待实施
