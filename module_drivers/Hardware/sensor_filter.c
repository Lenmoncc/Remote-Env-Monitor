#include "sensor_filter.h"
#include <stdlib.h>
#include <math.h>

/*=================================================================================
 * 滑动平均滤波实现
 =================================================================================*/

int32_t MovingAverage_Init(MovingAverage_t *ma, uint8_t window_size)
{
    if (ma == NULL || window_size == 0) {
        return -1;
    }

    ma->window_size = window_size;
    ma->buffer = (float *)malloc(window_size * sizeof(float));

    if (ma->buffer == NULL) {
        return -1;
    }

    memset(ma->buffer, 0, window_size * sizeof(float));
    ma->index = 0;
    ma->count = 0;
    ma->sum = 0;

    return 0;
}

float MovingAverage_Update(MovingAverage_t *ma, float value)
{
    if (ma == NULL || ma->buffer == NULL) {
        return value;
    }

    /* 如果缓冲区满，减去最旧的值 */
    if (ma->count == ma->window_size) {
        ma->sum -= ma->buffer[ma->index];
    } else {
        ma->count++;
    }

    /* 添加新值 */
    ma->buffer[ma->index] = value;
    ma->sum += value;

    /* 移动指针（环形缓冲） */
    ma->index = (ma->index + 1) % ma->window_size;

    /* 返回平均值 */
    return ma->sum / ma->count;
}

void MovingAverage_Reset(MovingAverage_t *ma)
{
    if (ma == NULL || ma->buffer == NULL) {
        return;
    }

    memset(ma->buffer, 0, ma->window_size * sizeof(float));
    ma->index = 0;
    ma->count = 0;
    ma->sum = 0;
}

/*=================================================================================
 * 中值滤波实现
 =================================================================================*/

/* 快速中值排序（3个或5个元素） */
static float _median3(float a, float b, float c)
{
    if (a > b) {
        if (b > c) return b;      /* a > b > c */
        if (a > c) return c;      /* a > c >= b */
        return a;                 /* c >= a > b */
    } else {
        if (a > c) return a;      /* b >= a > c */
        if (b > c) return c;      /* b >= c >= a */
        return b;                 /* c >= b >= a */
    }
}

static float _median5(float *arr)
{
    /* 简单排序法（5个元素足够快） */
    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            if (arr[i] > arr[j]) {
                float tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
    return arr[2];  /* 返回中间值 */
}

int32_t MedianFilter_Init(MedianFilter_t *mf, uint8_t window_size)
{
    if (mf == NULL || window_size == 0 || window_size > 255) {
        return -1;
    }

    mf->window_size = window_size;
    mf->buffer = (float *)malloc(window_size * sizeof(float));

    if (mf->buffer == NULL) {
        return -1;
    }

    memset(mf->buffer, 0, window_size * sizeof(float));
    mf->index = 0;
    mf->count = 0;

    return 0;
}

float MedianFilter_Update(MedianFilter_t *mf, float value)
{
    if (mf == NULL || mf->buffer == NULL) {
        return value;
    }

    /* 添加新值 */
    mf->buffer[mf->index] = value;

    /* 更新计数和指针 */
    if (mf->count < mf->window_size) {
        mf->count++;
    }

    mf->index = (mf->index + 1) % mf->window_size;

    /* 计算中值 */
    if (mf->count < 3) {
        /* 数据不足3个，返回最后的值 */
        return value;
    }

    if (mf->count == 3) {
        return _median3(mf->buffer[0], mf->buffer[1], mf->buffer[2]);
    }

    if (mf->count == 5) {
        float tmp[5];
        memcpy(tmp, mf->buffer, 5 * sizeof(float));
        return _median5(tmp);
    }

    /* 通用排序法（对于大窗口） */
    float *tmp = (float *)malloc(mf->count * sizeof(float));
    if (tmp == NULL) {
        return value;  /* 内存分配失败，返回原值 */
    }

    memcpy(tmp, mf->buffer, mf->count * sizeof(float));

    /* 快速排序 */
    for (int i = 0; i < mf->count; i++) {
        for (int j = i + 1; j < mf->count; j++) {
            if (tmp[i] > tmp[j]) {
                float temp = tmp[i];
                tmp[i] = tmp[j];
                tmp[j] = temp;
            }
        }
    }

    float median = tmp[mf->count / 2];
    free(tmp);

    return median;
}

void MedianFilter_Reset(MedianFilter_t *mf)
{
    if (mf == NULL || mf->buffer == NULL) {
        return;
    }

    memset(mf->buffer, 0, mf->window_size * sizeof(float));
    mf->index = 0;
    mf->count = 0;
}

/*=================================================================================
 * 指数平滑实现
 =================================================================================*/

int32_t ExponentialSmoothing_Init(ExponentialSmoothing_t *es, float alpha)
{
    if (es == NULL) {
        return -1;
    }

    if (alpha < 0.0f || alpha > 1.0f) {
        alpha = 0.3f;  /* 默认值 */
    }

    es->alpha = alpha;
    es->last_value = 0.0f;
    es->initialized = 0;

    return 0;
}

float ExponentialSmoothing_Update(ExponentialSmoothing_t *es, float value)
{
    if (es == NULL) {
        return value;
    }

    /* 首次初始化 */
    if (es->initialized == 0) {
        es->last_value = value;
        es->initialized = 1;
        return value;
    }

    /* 计算平滑值：y = α*x + (1-α)*y_prev */
    es->last_value = es->alpha * value + (1.0f - es->alpha) * es->last_value;

    return es->last_value;
}

void ExponentialSmoothing_Reset(ExponentialSmoothing_t *es)
{
    if (es == NULL) {
        return;
    }

    es->last_value = 0.0f;
    es->initialized = 0;
}

/*=================================================================================
 * 卡尔曼滤波实现
 =================================================================================*/

int32_t KalmanFilter_Init(KalmanFilter_t *kf, float q, float r, float init_value)
{
    if (kf == NULL) {
        return -1;
    }

    if (q < 0.0f) q = 0.01f;  /* 默认值 */
    if (r < 0.0f) r = 10.0f;  /* 默认值 */

    kf->q = q;
    kf->r = r;
    kf->x = init_value;        /* 初始状态估计 */
    kf->p = 1.0f;              /* 初始估计协方差 */
    kf->k = 0.0f;              /* 初始卡尔曼增益 */

    return 0;
}

float KalmanFilter_Update(KalmanFilter_t *kf, float measurement)
{
    if (kf == NULL) {
        return measurement;
    }

    /* 预测阶段 */
    /* 状态预测：x_pred = x（假设匀速运动模型） */
    float x_pred = kf->x;

    /* 协方差预测：p_pred = p + q */
    float p_pred = kf->p + kf->q;

    /* 更新阶段 */
    /* 卡尔曼增益：k = p_pred / (p_pred + r) */
    kf->k = p_pred / (p_pred + kf->r);

    /* 状态更新：x = x_pred + k * (z - x_pred) */
    kf->x = x_pred + kf->k * (measurement - x_pred);

    /* 协方差更新：p = (1 - k) * p_pred */
    kf->p = (1.0f - kf->k) * p_pred;

    return kf->x;
}

void KalmanFilter_Reset(KalmanFilter_t *kf)
{
    if (kf == NULL) {
        return;
    }

    kf->x = 0.0f;
    kf->p = 1.0f;
    kf->k = 0.0f;
}
