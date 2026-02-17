#ifndef __BH1750_OPTIMIZED_H__
#define __BH1750_OPTIMIZED_H__

#include <stdint.h>
#include "sensor_filter.h"

/**
 * @file    BH1750_optimized.h
 * @brief   BH1750 光照传感器 - 优化版
 *
 * 改进点：
 * 1. 范围检查（上溢出、下溢出）
 * 2. 自适应增益调整
 * 3. 中值滤波
 * 4. FreeRTOS 兼容
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

/* 错误码定义 */
typedef enum {
    BH1750_OK = 0,           /* 成功 */
    BH1750_UNDERFLOW = 1,    /* 光照过弱 */
    BH1750_OVERFLOW = 2,     /* 光照过强 */
    BH1750_COMM_ERR = 3,     /* 通信错误 */
    BH1750_INIT_FAILED = 4   /* 初始化失败 */
} BH1750_Status_t;

/* 传感器分辨率模式 */
typedef enum {
    BH1750_RESOLUTION_HIGH = 0x10,    /* 1 lux 分辨率 */
    BH1750_RESOLUTION_HIGH2 = 0x11,   /* 0.5 lux 分辨率 */
    BH1750_RESOLUTION_LOW = 0x13      /* 4 lux 分辨率 */
} BH1750_Resolution_t;

/* 传感器数据结构 */
typedef struct {
    uint16_t lux;           /* 光照值（lux） */
    BH1750_Resolution_t mode; /* 当前工作模式 */
    uint8_t is_saturated;   /* 是否饱和 */
    uint32_t timestamp;     /* 最后更新时间 */
} BH1750_Data_t;

/* 传感器管理结构 */
typedef struct {
    BH1750_Data_t data;
    MedianFilter_t filter;
    uint8_t use_filter;
    uint8_t auto_gain_enabled;
    uint8_t adjustment_counter;
    uint16_t error_count;
    uint16_t success_count;
    uint16_t saturation_count;
} BH1750_Manager_t;

/*=================================================================================
 * 函数声明
 =================================================================================*/

/**
 * @brief  初始化 BH1750
 * @param  manager：传感器管理结构
 * @param  enable_filter：是否启用中值滤波
 * @param  enable_auto_gain：是否启用自适应增益
 * @return BH1750_OK 或其他错误码
 */
BH1750_Status_t BH1750_Init_Optimized(BH1750_Manager_t *manager,
                                      uint8_t enable_filter,
                                      uint8_t enable_auto_gain);

/**
 * @brief  读取光照值
 * @param  manager：传感器管理结构
 * @param  data：输出数据结构体
 * @return BH1750_OK 或其他错误码
 *
 * 功能：
 * - 读取原始数据
 * - 范围检查（上下溢出）
 * - 中值滤波
 * - 自适应增益调整
 */
BH1750_Status_t BH1750_ReadData_Optimized(BH1750_Manager_t *manager, BH1750_Data_t *data);

/**
 * @brief  手动切换分辨率模式
 * @param  manager：传感器管理结构
 * @param  mode：目标分辨率模式
 * @return BH1750_OK 或其他错误码
 */
BH1750_Status_t BH1750_SetResolution(BH1750_Manager_t *manager, BH1750_Resolution_t mode);

/**
 * @brief  获取传感器统计信息
 * @param  manager：传感器管理结构
 * @param  success_count：输出成功计数
 * @param  error_count：输出错误计数
 * @param  saturation_count：输出饱和计数
 * @return 成功率（百分比）
 */
uint8_t BH1750_GetStatistics(BH1750_Manager_t *manager,
                             uint16_t *success_count,
                             uint16_t *error_count,
                             uint16_t *saturation_count);

/**
 * @brief  重置传感器状态
 * @param  manager：传感器管理结构
 */
void BH1750_Reset(BH1750_Manager_t *manager);

#endif /* __BH1750_OPTIMIZED_H__ */
