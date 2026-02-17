#include "task_manager.h"
#include "Delay.h"
#include "USART_1.h"
#include "UART_5.h"
#include "AHT10.h"
#include "BH1750.h"
#include "BMP280.h"
#include "sgp30.h"
#include "mb.h"
#include "string.h"

/*=================================================================================
 * 全局数据和同步原语定义
 =================================================================================*/

/* 传感器数据（受 g_sensorDataMutex 保护） */
static SensorData_t g_sensorData = {0};

/* 互斥量：保护传感器数据结构 */
static SemaphoreHandle_t g_sensorDataMutex = NULL;

/* 队列：Modbus 事件通知（如需要） */
static QueueHandle_t g_modbusEventQueue = NULL;

/*=================================================================================
 * 本地函数声明
 =================================================================================*/

/**
 * @brief  传感器采集任务
 * 周期性（1Hz）采集所有传感器数据，存储到受保护的全局结构体
 */
static void SensorTask(void *pvParameters);

/**
 * @brief  Modbus 通信任务
 * 高优先级处理 Modbus 协议栈
 */
static void ModbusTask(void *pvParameters);

/**
 * @brief  监控任务
 * 定期输出系统状态信息，用于调试
 */
static void MonitorTask(void *pvParameters);

/*=================================================================================
 * 初始化函数
 =================================================================================*/

void TaskManager_Init(void)
{
    BaseType_t ret;

    UART5_SendString("========== TaskManager_Init ==========\r\n");

    /* 1. 创建互斥量（保护传感器数据） */
    g_sensorDataMutex = xSemaphoreCreateMutex();
    if (g_sensorDataMutex == NULL) {
        UART5_SendString("ERROR: Failed to create sensor data mutex\r\n");
        return;
    }
    UART5_SendString("✓ Sensor data mutex created\r\n");

    /* 2. 创建队列（可选，用于任务间通信） */
    g_modbusEventQueue = xQueueCreate(10, sizeof(uint8_t));
    if (g_modbusEventQueue == NULL) {
        UART5_SendString("ERROR: Failed to create modbus event queue\r\n");
        return;
    }
    UART5_SendString("✓ Modbus event queue created\r\n");

    /* 3. 创建传感器采集任务 */
    ret = xTaskCreate(
        SensorTask,
        "SensorTask",
        STACK_SENSOR_TASK,
        NULL,
        PRIO_SENSOR_TASK,
        NULL
    );
    if (ret != pdPASS) {
        UART5_SendString("ERROR: Failed to create SensorTask\r\n");
        return;
    }
    UART5_SendString("✓ SensorTask created (Priority: 1)\r\n");

    /* 4. 创建 Modbus 通信任务 */
    ret = xTaskCreate(
        ModbusTask,
        "ModbusTask",
        STACK_MODBUS_TASK,
        NULL,
        PRIO_MODBUS_TASK,
        NULL
    );
    if (ret != pdPASS) {
        UART5_SendString("ERROR: Failed to create ModbusTask\r\n");
        return;
    }
    UART5_SendString("✓ ModbusTask created (Priority: 3)\r\n");

    /* 5. 创建监控任务 */
    ret = xTaskCreate(
        MonitorTask,
        "MonitorTask",
        STACK_MONITOR_TASK,
        NULL,
        PRIO_MONITOR_TASK,
        NULL
    );
    if (ret != pdPASS) {
        UART5_SendString("ERROR: Failed to create MonitorTask\r\n");
        return;
    }
    UART5_SendString("✓ MonitorTask created (Priority: 2)\r\n");

    UART5_SendString("========== TaskManager Ready ==========\r\n");
}

/*=================================================================================
 * 数据访问接口实现（线程安全）
 =================================================================================*/

SensorData_t SensorData_GetSnapshot(void)
{
    SensorData_t snapshot = {0};

    /* 获取互斥量，最多等待 portMAX_DELAY */
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        /* 复制数据 */
        memcpy(&snapshot, (void *)&g_sensorData, sizeof(SensorData_t));
        /* 释放互斥量 */
        xSemaphoreGive(g_sensorDataMutex);
    }

    return snapshot;
}

void SensorData_UpdateTemperature(float temp_aht10, float temp_bmp280)
{
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        g_sensorData.temperature_aht10 = temp_aht10;
        g_sensorData.temperature_bmp280 = temp_bmp280;
        g_sensorData.lastUpdateTime = xTaskGetTickCount();
        xSemaphoreGive(g_sensorDataMutex);
    }
}

void SensorData_UpdateHumidity(float humidity)
{
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        g_sensorData.humidity_aht10 = humidity;
        g_sensorData.lastUpdateTime = xTaskGetTickCount();
        xSemaphoreGive(g_sensorDataMutex);
    }
}

void SensorData_UpdateLight(int lux)
{
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        g_sensorData.lux = lux;
        g_sensorData.lastUpdateTime = xTaskGetTickCount();
        xSemaphoreGive(g_sensorDataMutex);
    }
}

void SensorData_UpdatePressure(float pressure)
{
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        g_sensorData.pressure_bmp280 = pressure;
        g_sensorData.lastUpdateTime = xTaskGetTickCount();
        xSemaphoreGive(g_sensorDataMutex);
    }
}

void SensorData_UpdateGas(int co2_eq, int tvoc)
{
    if (xSemaphoreTake(g_sensorDataMutex, portMAX_DELAY) == pdTRUE) {
        g_sensorData.co2_eq = co2_eq;
        g_sensorData.tvoc = tvoc;
        g_sensorData.lastUpdateTime = xTaskGetTickCount();
        xSemaphoreGive(g_sensorDataMutex);
    }
}

/*=================================================================================
 * 任务实现
 =================================================================================*/

/**
 * @brief  传感器采集任务
 *
 * 周期：1Hz（每 1000ms 采集一次）
 * 优先级：1（低，可被高优先级任务中断）
 * 功能：
 *   1. 采集所有传感器数据
 *   2. 通过数据访问接口更新全局结构体
 *   3. 使用 vTaskDelayUntil 确保周期性
 */
static void SensorTask(void *pvParameters)
{
    AHT10_Data aht10_data = {0};
    BMP280_DataTypedef bmp280_data = {0};
    int lux = 0;
    int co2_eq = 0;
    int tvoc = 0;

    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(1000);  /* 1000ms 周期 */

    /* 初始化传感器 */
    UART5_SendString("[SensorTask] Initializing sensors...\r\n");

    aht10_init();
    Delay_ms(100);

    BH1750_Init();
    Delay_ms(50);

    BMP280_Init(&bmp_config);
    Delay_ms(50);

    sgp30_data_show_init();
    Delay_ms(100);

    UART5_SendString("[SensorTask] All sensors initialized\r\n");

    /* 获取任务启动时的滴答时间 */
    xLastWakeTime = xTaskGetTickCount();

    /* 任务主循环 */
    while (1) {
        /* 采集传感器数据 */
        aht10_get_data(&aht10_data);
        /* 注意：aht10_get_data() 内部可能有延时，期间其他高优先级任务可以运行 */

        lux = BH1750_ReadLight();
        /* 注意：BH1750_ReadLight() 有 ~120ms 阻塞延时，Modbus 任务可以抢占 */

        BMP280_GetData(&bmp280_data);

        SGP30_MeasureIAQ(&co2_eq, &tvoc);

        /* 更新全局数据结构（线程安全） */
        SensorData_UpdateTemperature(aht10_data.temperature, bmp280_data.temperature);
        SensorData_UpdateHumidity(aht10_data.humidity);
        SensorData_UpdateLight(lux);
        SensorData_UpdatePressure(bmp280_data.pressure);
        SensorData_UpdateGas(co2_eq, tvoc);

        /* 定期输出调试信息 */
        UART5_Printf("[Sensor] T_AHT=%.1f°C H=%.1f%% Lux=%d P=%.1fhPa\r\n",
                     aht10_data.temperature, aht10_data.humidity, lux, bmp280_data.pressure);

        /* 延迟到下一个周期
         * vTaskDelayUntil 会自动计算需要等待的时间，确保周期性
         * 如果采集耗时 > 1000ms，任务会立即执行下一个周期（不会跳过）
         */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

/**
 * @brief  Modbus 通信任务
 *
 * 周期：尽可能快（主要是 eMBPoll() 的速度）
 * 优先级：3（高，用于快速响应 Modbus 请求）
 * 功能：
 *   1. 初始化 Modbus 协议栈
 *   2. 循环处理 Modbus 消息
 *   3. 偶尔让出 CPU 给其他任务
 */
static void ModbusTask(void *pvParameters)
{
    eMBErrorCode eStatus;

    UART5_SendString("[ModbusTask] Initializing Modbus RTU...\r\n");

    /* 初始化 Modbus 协议栈
     * 参数：
     * - MB_RTU：RTU 模式
     * - 1：从站 ID
     * - 1：端口 1（USART1）
     * - 115200：波特率
     * - MB_PAR_NONE：无奇偶校验
     */
    eStatus = eMBInit(MB_RTU, 1, 1, 115200, MB_PAR_NONE);
    if (eStatus != MB_ENOERR) {
        UART5_Printf("[ModbusTask] Init failed: %d\r\n", eStatus);
        vTaskDelete(NULL);  /* 删除任务 */
        return;
    }

    /* 启用 Modbus */
    eStatus = eMBEnable();
    if (eStatus != MB_ENOERR) {
        UART5_Printf("[ModbusTask] Enable failed: %d\r\n", eStatus);
        vTaskDelete(NULL);
        return;
    }

    UART5_SendString("[ModbusTask] Modbus started successfully\r\n");

    /* Modbus 主循环 */
    while (1) {
        /* 处理 Modbus 协议
         * eMBPoll() 是非阻塞的，如果没有待处理的数据会立即返回
         */
        (void)eMBPoll();

        /* 给其他任务留出 CPU 时间
         * 即使是高优先级任务，也应该偶尔让出 CPU
         * 这个延时很短（10ms），Modbus 响应时间仍然很快
         */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief  监控任务
 *
 * 周期：5000ms（5 秒）
 * 优先级：2（中等）
 * 功能：
 *   1. 定期输出系统状态
 *   2. 监控任务堆栈使用情况（可选）
 *   3. LED 闪烁或其他指示（可选）
 */
static void MonitorTask(void *pvParameters)
{
    uint32_t freeHeap;
    uint32_t runCount = 0;

    UART5_SendString("[MonitorTask] Started\r\n");

    while (1) {
        /* 获取当前可用堆内存 */
        freeHeap = xPortGetFreeHeapSize();

        /* 输出系统状态 */
        runCount++;
        UART5_Printf("[Monitor] Tick=%lu, FreeHeap=%lu bytes, Run#=%lu\r\n",
                     xTaskGetTickCount(), freeHeap, runCount);

        /* 每 5 秒执行一次 */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
