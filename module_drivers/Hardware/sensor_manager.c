#include "sensor_manager.h"
#include "UART_5.h"

/*=================================================================================
 * 全局传感器管理器实例
 =================================================================================*/

SensorManager_t g_sensorManager = {0};

/*=================================================================================
 * 初始化函数
 =================================================================================*/

int32_t SensorManager_Init(uint8_t enable_filtering, uint8_t enable_auto_gain)
{
    UART5_SendString("===== Sensor Manager Initialization =====\r\n");

    /* 1. 初始化 AHT10 */
    if (AHT10_Init_Optimized(&g_sensorManager.aht10, enable_filtering) != AHT10_OK) {
        UART5_SendString("ERROR: AHT10 init failed\r\n");
        return -1;
    }

    /* 创建 AHT10 互斥量 */
    g_sensorManager.aht10_mutex = xSemaphoreCreateMutex();
    if (g_sensorManager.aht10_mutex == NULL) {
        UART5_SendString("ERROR: AHT10 mutex creation failed\r\n");
        return -1;
    }

    /* 2. 初始化 BH1750 */
    if (BH1750_Init_Optimized(&g_sensorManager.bh1750, enable_filtering, enable_auto_gain)
        != BH1750_OK) {
        UART5_SendString("ERROR: BH1750 init failed\r\n");
        return -1;
    }

    /* 创建 BH1750 互斥量 */
    g_sensorManager.bh1750_mutex = xSemaphoreCreateMutex();
    if (g_sensorManager.bh1750_mutex == NULL) {
        UART5_SendString("ERROR: BH1750 mutex creation failed\r\n");
        return -1;
    }

    /* 3. 初始化全局统计 */
    g_sensorManager.total_readings = 0;
    g_sensorManager.total_errors = 0;
    g_sensorManager.last_update_time = xTaskGetTickCount();

    UART5_SendString("✓ Sensor Manager Initialized\r\n");
    UART5_Printf("  Filtering: %s, Auto-Gain: %s\r\n",
                 enable_filtering ? "ON" : "OFF",
                 enable_auto_gain ? "ON" : "OFF");

    return 0;
}

/*=================================================================================
 * 数据读取接口（线程安全）
 =================================================================================*/

int32_t SensorManager_ReadAHT10(AHT10_Data_t *data)
{
    if (data == NULL) {
        return -1;
    }

    /* 获取互斥量（最多等待 100ms） */
    if (xSemaphoreTake(g_sensorManager.aht10_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        UART5_SendString("[SensorMgr] AHT10 mutex timeout\r\n");
        return -1;
    }

    /* 读取数据 */
    AHT10_Status_t status = AHT10_ReadData_Optimized(&g_sensorManager.aht10, data);

    /* 更新统计 */
    g_sensorManager.total_readings++;
    if (status != AHT10_OK) {
        g_sensorManager.total_errors++;
    }

    /* 释放互斥量 */
    xSemaphoreGive(g_sensorManager.aht10_mutex);

    return (status == AHT10_OK) ? 0 : -1;
}

int32_t SensorManager_ReadBH1750(BH1750_Data_t *data)
{
    if (data == NULL) {
        return -1;
    }

    /* 获取互斥量（最多等待 150ms）*/
    if (xSemaphoreTake(g_sensorManager.bh1750_mutex, pdMS_TO_TICKS(150)) != pdTRUE) {
        UART5_SendString("[SensorMgr] BH1750 mutex timeout\r\n");
        return -1;
    }

    /* 读取数据 */
    BH1750_Status_t status = BH1750_ReadData_Optimized(&g_sensorManager.bh1750, data);

    /* 更新统计 */
    g_sensorManager.total_readings++;
    if (status != BH1750_OK) {
        g_sensorManager.total_errors++;
    }

    /* 释放互斥量 */
    xSemaphoreGive(g_sensorManager.bh1750_mutex);

    return (status == BH1750_OK) ? 0 : -1;
}

int32_t SensorManager_ReadAll(AHT10_Data_t *aht10_data, BH1750_Data_t *bh1750_data)
{
    int32_t ret = 0;

    if (aht10_data != NULL) {
        if (SensorManager_ReadAHT10(aht10_data) != 0) {
            ret |= 0x01;  /* AHT10 失败 */
        }
    }

    if (bh1750_data != NULL) {
        if (SensorManager_ReadBH1750(bh1750_data) != 0) {
            ret |= 0x02;  /* BH1750 失败 */
        }
    }

    g_sensorManager.last_update_time = xTaskGetTickCount();

    return ret;
}

/*=================================================================================
 * 诊断函数
 =================================================================================*/

void SensorManager_PrintStatistics(void)
{
    uint16_t aht10_success, aht10_error;
    uint16_t bh1750_success, bh1750_error, bh1750_saturation;

    uint8_t aht10_rate = AHT10_GetStatistics(&g_sensorManager.aht10, &aht10_success, &aht10_error);
    uint8_t bh1750_rate = BH1750_GetStatistics(&g_sensorManager.bh1750,
                                               &bh1750_success, &bh1750_error,
                                               &bh1750_saturation);

    UART5_SendString("\r\n===== Sensor Statistics =====\r\n");

    UART5_Printf("AHT10: %d%% success rate (%d reads, %d errors)\r\n",
                 aht10_rate, aht10_success, aht10_error);

    UART5_Printf("BH1750: %d%% success rate (%d reads, %d errors, %d saturations)\r\n",
                 bh1750_rate, bh1750_success, bh1750_error, bh1750_saturation);

    UART5_Printf("Overall: %d total readings, %d total errors\r\n",
                 g_sensorManager.total_readings, g_sensorManager.total_errors);

    uint8_t health = SensorManager_GetHealth();
    UART5_Printf("System Health: %d%%\r\n\r\n", health);
}

uint8_t SensorManager_GetHealth(void)
{
    if (g_sensorManager.total_readings == 0) {
        return 0;
    }

    uint32_t success = g_sensorManager.total_readings - g_sensorManager.total_errors;
    return (uint8_t)((success * 100) / g_sensorManager.total_readings);
}

void SensorManager_ResetStatistics(void)
{
    AHT10_Reset(&g_sensorManager.aht10);
    BH1750_Reset(&g_sensorManager.bh1750);

    g_sensorManager.total_readings = 0;
    g_sensorManager.total_errors = 0;
    g_sensorManager.last_update_time = xTaskGetTickCount();

    UART5_SendString("[SensorMgr] Statistics reset\r\n");
}
