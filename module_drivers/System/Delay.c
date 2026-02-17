#include "Delay.h"
#include "stm32f4xx.h"

/* FreeRTOS support */
#include "FreeRTOS.h"
#include "task.h"

/*
 * TIM2 用于微秒级延时（轮询式，仅用于 <10us 的硬件时序）
 * 毫秒及以上延时使用 FreeRTOS vTaskDelay（允许任务切换）
 */

void Delay_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_Init;
    TIM_Init.TIM_Prescaler = (SystemCoreClock / 1000000) - 1;  // 1MHz -> 1μs 分辨率
    TIM_Init.TIM_Period = 0xFFFFFFFF;  // 不溢出，通过轮询实现
    TIM_Init.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_Init.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_Init);

    TIM_Cmd(TIM2, ENABLE);

    /* FreeRTOS 系统滴答已由 FreeRTOS 初始化，无需额外配置 */
}

/**
 * @brief  微秒级延时（轮询式，阻塞）
 * @note   仅用于小于 10μs 的硬件时序延时，不会触发任务切换
 * @param  us：微秒数（建议不超过 10000）
 */
void Delay_us(uint32_t us)
{
    uint32_t start = TIM_GetCounter(TIM2);
    while ((TIM_GetCounter(TIM2) - start) < us);
}

/**
 * @brief  毫秒级延时（使用 FreeRTOS）
 * @note   会让出 CPU 给其他任务，是否立即返回取决于调度器
 * @param  ms：毫秒数
 */
void Delay_ms(uint32_t ms)
{
    if (ms > 0) {
        /* 检查调度器是否已启动 */
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            /* 使用 FreeRTOS 延时（允许任务切换） */
            vTaskDelay(pdMS_TO_TICKS(ms));
        } else {
            /* 调度器未启动，使用轮询延时 */
            while (ms--) {
                Delay_us(1000);
            }
        }
    }
}

/**
 * @brief  秒级延时（使用 FreeRTOS）
 * @param  s：秒数
 */
void Delay_s(uint32_t s)
{
    Delay_ms(s * 1000);
}

/**
 * @brief  非阻塞式延时检查（用于状态机）
 * @param  pStartTick：指向保存的起始滴答时间指针
 * @param  delayMs：延时毫秒数
 * @retval pdTRUE 如果指定时间已过，pdFALSE 否则
 *
 * 使用示例：
 * @code
 * static uint32_t startTick;
 *
 * // 首次调用时记录起始时间
 * if (condition) {
 *     startTick = xTaskGetTickCount();
 * }
 *
 * // 检查 500ms 是否已过
 * if (Delay_IsExpired(&startTick, 500)) {
 *     // 执行延时后的操作
 * }
 * @endcode
 */
BaseType_t Delay_IsExpired(uint32_t *pStartTick, uint32_t delayMs)
{
    if ((xTaskGetTickCount() - *pStartTick) >= pdMS_TO_TICKS(delayMs)) {
        return pdTRUE;
    }
    return pdFALSE;
}
