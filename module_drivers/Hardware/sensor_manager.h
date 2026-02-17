#ifndef __SENSOR_MANAGER_H__
#define __SENSOR_MANAGER_H__

#include "FreeRTOS.h"
#include "semphr.h"

#include "AHT10_optimized.h"
#include "BH1750_optimized.h"

/**
 * @file    sensor_manager.h
 * @brief   FreeRTOS 兼容的传感器管理框架
 *
 * 该框架统一管理所有传感器，提供：
 * - 集中的传感器初始化
 * - 统一的数据读取接口
 * - 中断安全的数据共享
 * - 性能统计和故障监测
 * - FreeRTOS 互斥量保护
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

/*=================================================================================
 * 传感器管理结构
 =================================================================================*/

typedef struct {
    /* AHT10 温湿度传感器 */
    AHT10_Manager_t aht10;
    SemaphoreHandle_t aht10_mutex;

    /* BH1750 光照传感器 */
    BH1750_Manager_t bh1750;
    SemaphoreHandle_t bh1750_mutex;

    /* BMP280 气压传感器（待优化） */
    // BMP280_Manager_t bmp280;
    // SemaphoreHandle_t bmp280_mutex;

    /* SGP30 空气质量传感器（待优化） */
    // SGP30_Manager_t sgp30;
    // SemaphoreHandle_t sgp30_mutex;

    /* 全局统计 */
    uint32_t total_readings;
    uint32_t total_errors;
    uint32_t last_update_time;
} SensorManager_t;

/*=================================================================================
 * 全局实例
 =================================================================================*/

extern SensorManager_t g_sensorManager;

/*=================================================================================
 * 初始化函数
 =================================================================================*/

/**
 * @brief  初始化传感器管理器
 * @param  enable_filtering：是否启用所有滤波器
 * @param  enable_auto_gain：是否启用自适应增益
 * @return 0 成功，-1 失败
 */
int32_t SensorManager_Init(uint8_t enable_filtering, uint8_t enable_auto_gain);

/*=================================================================================
 * 数据读取接口（线程安全）
 =================================================================================*/

/**
 * @brief  读取 AHT10 数据（线程安全）
 * @param  data：输出数据
 * @return 0 成功，-1 失败
 */
int32_t SensorManager_ReadAHT10(AHT10_Data_t *data);

/**
 * @brief  读取 BH1750 数据（线程安全）
 * @param  data：输出数据
 * @return 0 成功，-1 失败
 */
int32_t SensorManager_ReadBH1750(BH1750_Data_t *data);

/**
 * @brief  读取所有传感器数据（线程安全）
 * @param  aht10_data：AHT10 输出
 * @param  bh1750_data：BH1750 输出
 * @return 0 全部成功，其他表示部分失败
 */
int32_t SensorManager_ReadAll(AHT10_Data_t *aht10_data, BH1750_Data_t *bh1750_data);

/*=================================================================================
 * 诊断函数
 =================================================================================*/

/**
 * @brief  打印传感器统计信息
 */
void SensorManager_PrintStatistics(void);

/**
 * @brief  检查传感器健康状态
 * @return 健康度（百分比）
 */
uint8_t SensorManager_GetHealth(void);

/**
 * @brief  重置所有传感器统计
 */
void SensorManager_ResetStatistics(void);

#endif /* __SENSOR_MANAGER_H__ */
