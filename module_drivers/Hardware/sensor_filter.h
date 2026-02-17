#ifndef __SENSOR_FILTER_H__
#define __SENSOR_FILTER_H__

#include <stdint.h>
#include <string.h>

/**
 * @file    sensor_filter.h
 * @brief   传感器数据滤波库
 *
 * 提供多种滤波算法供所有传感器使用：
 * - 滑动平均（Moving Average）
 * - 中值滤波（Median Filter）
 * - 卡尔曼滤波（Kalman Filter）
 * - 指数平滑（Exponential Smoothing）
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

/*=================================================================================
 * 滑动平均滤波 (Moving Average Filter)
 *
 * 优点：简单、低计算量、有效消除随机噪声
 * 缺点：无法跟踪快速变化
 * 用途：温度、湿度、气压等变化缓慢的传感器
 =================================================================================*/

typedef struct {
    float *buffer;
    uint8_t window_size;
    uint8_t index;
    uint8_t count;
    float sum;
} MovingAverage_t;

/**
 * @brief  初始化滑动平均滤波器
 * @param  ma：滤波器结构体
 * @param  window_size：窗口大小（通常 3-10）
 * @return 0 成功，-1 失败
 */
int32_t MovingAverage_Init(MovingAverage_t *ma, uint8_t window_size);

/**
 * @brief  更新滤波器并获取平均值
 * @param  ma：滤波器结构体
 * @param  value：新采样值
 * @return 平均值
 */
float MovingAverage_Update(MovingAverage_t *ma, float value);

/**
 * @brief  清空滤波器状态
 * @param  ma：滤波器结构体
 */
void MovingAverage_Reset(MovingAverage_t *ma);

/*=================================================================================
 * 中值滤波 (Median Filter)
 *
 * 优点：有效消除脉冲干扰，保留边界信息
 * 缺点：计算量较大
 * 用途：光照、ADC 等容易出现毛刺的数据
 =================================================================================*/

typedef struct {
    float *buffer;
    uint8_t window_size;
    uint8_t index;
    uint8_t count;
} MedianFilter_t;

/**
 * @brief  初始化中值滤波器
 * @param  mf：滤波器结构体
 * @param  window_size：窗口大小（通常 3, 5, 7）
 * @return 0 成功，-1 失败
 */
int32_t MedianFilter_Init(MedianFilter_t *mf, uint8_t window_size);

/**
 * @brief  更新滤波器并获取中值
 * @param  mf：滤波器结构体
 * @param  value：新采样值
 * @return 中值
 */
float MedianFilter_Update(MedianFilter_t *mf, float value);

/**
 * @brief  清空滤波器状态
 * @param  mf：滤波器结构体
 */
void MedianFilter_Reset(MedianFilter_t *mf);

/*=================================================================================
 * 指数平滑 (Exponential Smoothing)
 *
 * 优点：快速响应，计算量小，实时性好
 * 缺点：需要手动调整平滑系数
 * 用途：实时性要求高的传感器数据（光照、加速度等）
 *
 * 公式：y_n = α * x_n + (1 - α) * y_(n-1)
 * α 范围：0.1-0.5（α 越大，响应越快但噪声越多）
 =================================================================================*/

typedef struct {
    float alpha;      /* 平滑系数（0-1） */
    float last_value; /* 上一个平滑值 */
    uint8_t initialized;
} ExponentialSmoothing_t;

/**
 * @brief  初始化指数平滑滤波器
 * @param  es：滤波器结构体
 * @param  alpha：平滑系数 (0.1-0.5 推荐)
 * @return 0 成功，-1 失败
 */
int32_t ExponentialSmoothing_Init(ExponentialSmoothing_t *es, float alpha);

/**
 * @brief  更新滤波器并获取平滑值
 * @param  es：滤波器结构体
 * @param  value：新采样值
 * @return 平滑值
 */
float ExponentialSmoothing_Update(ExponentialSmoothing_t *es, float value);

/**
 * @brief  清空滤波器状态
 * @param  es：滤波器结构体
 */
void ExponentialSmoothing_Reset(ExponentialSmoothing_t *es);

/*=================================================================================
 * 卡尔曼滤波 (Kalman Filter)
 *
 * 优点：最优估计，能跟踪快速变化，噪声抑制最好
 * 缺点：计算复杂，需要调参
 * 用途：精度要求最高的应用（高度计算、运动跟踪等）
 *
 * 参数说明：
 * - q：过程噪声协方差（系统信任度，越小越信任模型）
 * - r：测量噪声协方差（传感器信任度，越小越信任传感器）
 =================================================================================*/

typedef struct {
    float q;              /* 过程噪声协方差 */
    float r;              /* 测量噪声协方差 */
    float x;              /* 状态估计 */
    float p;              /* 估计协方差 */
    float k;              /* 卡尔曼增益 */
} KalmanFilter_t;

/**
 * @brief  初始化卡尔曼滤波器
 * @param  kf：滤波器结构体
 * @param  q：过程噪声协方差 (0.001-0.1 推荐)
 * @param  r：测量噪声协方差 (1.0-100 推荐)
 * @param  init_value：初始值
 * @return 0 成功，-1 失败
 */
int32_t KalmanFilter_Init(KalmanFilter_t *kf, float q, float r, float init_value);

/**
 * @brief  更新滤波器并获取估计值
 * @param  kf：滤波器结构体
 * @param  measurement：测量值
 * @return 估计值
 */
float KalmanFilter_Update(KalmanFilter_t *kf, float measurement);

/**
 * @brief  清空滤波器状态
 * @param  kf：滤波器结构体
 */
void KalmanFilter_Reset(KalmanFilter_t *kf);

/*=================================================================================
 * 使用示例
 =================================================================================*/

/*
 * 1. 滑动平均（推荐用于温度、湿度）
 *
 * MovingAverage_t temp_filter;
 * MovingAverage_Init(&temp_filter, 5);  // 5个样本窗口
 *
 * float raw_temp = aht10_get_temp();
 * float filtered_temp = MovingAverage_Update(&temp_filter, raw_temp);
 *
 * ---
 *
 * 2. 中值滤波（推荐用于光照）
 *
 * MedianFilter_t lux_filter;
 * MedianFilter_Init(&lux_filter, 3);  // 3个样本中值
 *
 * int raw_lux = bh1750_get_lux();
 * int filtered_lux = (int)MedianFilter_Update(&lux_filter, (float)raw_lux);
 *
 * ---
 *
 * 3. 指数平滑（推荐用于快速变化的量）
 *
 * ExponentialSmoothing_t co2_filter;
 * ExponentialSmoothing_Init(&co2_filter, 0.3);  // α=0.3
 *
 * int raw_co2 = sgp30_get_co2();
 * int filtered_co2 = (int)ExponentialSmoothing_Update(&co2_filter, (float)raw_co2);
 *
 * ---
 *
 * 4. 卡尔曼滤波（用于最精密的应用）
 *
 * KalmanFilter_t altitude_filter;
 * KalmanFilter_Init(&altitude_filter, 0.01, 10, 100);  // q=0.01, r=10, init=100m
 *
 * float raw_alt = bmp280_calculate_altitude();
 * float filtered_alt = KalmanFilter_Update(&altitude_filter, raw_alt);
 */

#endif /* __SENSOR_FILTER_H__ */
