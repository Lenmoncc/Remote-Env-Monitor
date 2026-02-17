#ifndef __AHT10_OPTIMIZED_H__
#define __AHT10_OPTIMIZED_H__

#include <stdint.h>
#include "sensor_filter.h"

/**
 * @file    AHT10_optimized.h
 * @brief   AHT10 温湿度传感器 - 优化版
 *
 * 改进点：
 * 1. 添加校验和机制
 * 2. 3次重试机制
 * 3. 超时保护
 * 4. 数据滤波（可选）
 * 5. FreeRTOS 兼容
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

/* 错误码定义 */
typedef enum {
    AHT10_OK = 0,           /* 成功 */
    AHT10_CRC_ERR = 1,      /* 校验和错误 */
    AHT10_TIMEOUT = 2,      /* 超时 */
    AHT10_BUSY = 3,         /* 忙碌 */
    AHT10_INIT_FAILED = 4,  /* 初始化失败 */
    AHT10_I2C_ERR = 5       /* I2C 通信错误 */
} AHT10_Status_t;

/* 传感器数据结构 */
typedef struct {
    float temperature;  /* 温度，单位 ℃，精度 0.01℃ */
    float humidity;     /* 湿度，单位 %RH，精度 0.01%RH */
    uint32_t timestamp; /* 最后更新时间（FreeRTOS 滴答） */
} AHT10_Data_t;

/* 传感器管理结构 */
typedef struct {
    AHT10_Data_t data;
    MovingAverage_t temp_filter;
    MovingAverage_t hum_filter;
    uint8_t use_filter;
    uint32_t last_read_time;
    uint16_t error_count;
    uint16_t success_count;
} AHT10_Manager_t;

/*=================================================================================
 * 函数声明
 =================================================================================*/

/**
 * @brief  初始化 AHT10（带滤波）
 * @param  manager：传感器管理结构
 * @param  enable_filter：是否启用滤波（1=启用，0=禁用）
 * @return AHT10_OK 或其他错误码
 */
AHT10_Status_t AHT10_Init_Optimized(AHT10_Manager_t *manager, uint8_t enable_filter);

/**
 * @brief  读取传感器数据（带重试和校验）
 * @param  manager：传感器管理结构
 * @param  data：输出数据结构体
 * @return AHT10_OK 或其他错误码
 *
 * 功能：
 * - 触发测量
 * - 3次重试机制
 * - 校验和验证
 * - 数据滤波（如果启用）
 * - 错误计数
 */
AHT10_Status_t AHT10_ReadData_Optimized(AHT10_Manager_t *manager, AHT10_Data_t *data);

/**
 * @brief  获取滤波后的数据快照
 * @param  manager：传感器管理结构
 * @return 返回滤波后的数据
 */
AHT10_Data_t AHT10_GetFilteredData(AHT10_Manager_t *manager);

/**
 * @brief  获取传感器统计信息
 * @param  manager：传感器管理结构
 * @param  success_count：输出成功计数
 * @param  error_count：输出错误计数
 * @return 成功率（百分比）
 */
uint8_t AHT10_GetStatistics(AHT10_Manager_t *manager, uint16_t *success_count, uint16_t *error_count);

/**
 * @brief  重置传感器状态
 * @param  manager：传感器管理结构
 */
void AHT10_Reset(AHT10_Manager_t *manager);

#endif /* __AHT10_OPTIMIZED_H__ */
