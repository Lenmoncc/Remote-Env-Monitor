#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"
#include "FreeRTOS.h"

/**
 * @brief  初始化延时模块
 * @note   初始化 TIM2 用于微秒级延时
 */
void Delay_Init(void);

/**
 * @brief  微秒级延时（阻塞式，轮询）
 * @param  us：微秒数
 * @note   仅用于 <10us 的硬件时序，不会触发任务切换
 */
void Delay_us(uint32_t us);

/**
 * @brief  毫秒级延时（FreeRTOS 兼容）
 * @param  ms：毫秒数
 * @note   会让出 CPU 给其他任务（如果调度器已启动）
 *         在调度器启动前会使用轮询延时
 */
void Delay_ms(uint32_t ms);

/**
 * @brief  秒级延时（FreeRTOS 兼容）
 * @param  s：秒数
 */
void Delay_s(uint32_t s);

/**
 * @brief  非阻塞式延时检查
 * @param  pStartTick：起始滴答时间指针
 * @param  delayMs：延时毫秒数
 * @retval pdTRUE 如果时间已过，pdFALSE 否则
 */
BaseType_t Delay_IsExpired(uint32_t *pStartTick, uint32_t delayMs);

#endif
