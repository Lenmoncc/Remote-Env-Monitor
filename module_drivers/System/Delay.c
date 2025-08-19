#include "Delay.h"


// 系统时钟频率（单位：Hz）
#define SYSTEM_CLOCK_FREQ 168000000
static __IO uint32_t uwTimingDelay;

void TimingDelay_Decrement(void)
{
  if (uwTimingDelay != 0x00)
  { 
    uwTimingDelay--;
  }
}

/**
  * @brief  初始化SysTick定时器
  * @param  无
  * @retval 无
  */
void Delay_Init(void) {
    // 配置SysTick时钟源为内核时钟（168MHz）
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
}

/**
  * @brief  微秒级延时
  * @param  us 延时时长（微秒），范围：1~798915
  * @retval 无
  */
void Delay_us(uint32_t us) {
    // 计算需要的时钟周期数（168MHz时1us=168周期）
    uint32_t ticks = us * (SYSTEM_CLOCK_FREQ / 1000000);
    
    // 设置重装载值（24位最大值0xFFFFFF）
    SysTick->LOAD = (ticks - 1) & 0xFFFFFF;
    
    // 清空当前值
    SysTick->VAL = 0;
    
    // 启动定时器
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    // 等待计数完成
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    
    // 关闭定时器
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief  毫秒级延时
  * @param  ms 延时时长（毫秒），范围：1~4294967295
  * @retval 无
  */
void Delay_ms(uint32_t ms) {
    // 对于小于798ms的延时直接使用us级延时
    if (ms <= 798) {
        Delay_us(ms * 1000);
    } 
    // 大于798ms时使用循环优化
    else {
        while (ms--) {
            // 每次延时798ms（最大不溢出值）
            uint32_t chunks = ms > 798 ? 798 : ms;
            Delay_us(chunks * 1000);
            ms -= chunks;
        }
    }
}

/**
  * @brief  秒级延时
  * @param  s 延时时长（秒），范围：1~4294967295
  * @retval 无
  */
void Delay_s(uint32_t s) {
    while (s--) {
        Delay_ms(1000);
    }
}