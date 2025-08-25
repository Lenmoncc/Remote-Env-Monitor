///*
//#include "Delay.h"


//// ???????????λ??Hz??
//#define SYSTEM_CLOCK_FREQ 168000000
//static __IO uint32_t uwTimingDelay;

//void TimingDelay_Decrement(void)
//{
//  if (uwTimingDelay != 0x00)
//  { 
//    uwTimingDelay--;
//  }
//}

///**
//  * @brief  ?????SysTick?????
//  * @param  ??
//  * @retval ??
//  */
//void Delay_Init(void) {
//    // ????SysTick????????????168MHz??
//    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
//}

///**
//  * @brief  ??????
//  * @param  us ???????????????Χ??1~798915
//  * @retval ??
//  */
//void Delay_us(uint32_t us) {
//    // ????????????????????168MHz?1us=168?????
//    uint32_t ticks = us * (SYSTEM_CLOCK_FREQ / 1000000);
//    
//    // ????????????24λ????0xFFFFFF??
//    SysTick->LOAD = (ticks - 1) & 0xFFFFFF;
//    
//    // ??????
//    SysTick->VAL = 0;
//    
//    // ?????????
//    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
//    
//    // ??????????
//    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
//    
//    // ???????
//    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
//}

///**
//  * @brief  ???????
//  * @param  ms ????????????????Χ??1~4294967295
//  * @retval ??
//  */
//void Delay_ms(uint32_t ms) {
//    // ????С??798ms???????????us?????
//    if (ms <= 798) {
//        Delay_us(ms * 1000);
//    } 
//    // ????798ms??????????
//    else {
//        while (ms--) {
//            // ??????798ms???????????
//            uint32_t chunks = ms > 798 ? 798 : ms;
//            Delay_us(chunks * 1000);
//            ms -= chunks;
//        }
//    }
//}

///**
//  * @brief  ?????
//  * @param  s ??????????????Χ??1~4294967295
//  * @retval ??
//  */
//void Delay_s(uint32_t s) {
//    while (s--) {
//        Delay_ms(1000);
//    }
//}
//g
//*/




#include "Delay.h"
#include "stm32f4xx.h"

#define SYSTEM_CORE_CLOCK 168000000UL  // 168 MHz

/**
  * @brief  使用 TIM2 初始化为 1MHz 计数器（1us 递增）
  * @note   确保在 RCC 时钟中已经开启了 TIM2 时钟
  */
void Delay_Init(void)
{
    // 打开 TIM2 时钟
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 复位 TIM2
    TIM2->CR1 = 0;
    TIM2->PSC = (SYSTEM_CORE_CLOCK / 1000000) - 1;  // 分频到 1MHz (1us per tick)
    TIM2->ARR = 0xFFFFFFFF;  // 自动重装载最大值
    TIM2->EGR = TIM_EGR_UG;  // 更新寄存器
    TIM2->CR1 |= TIM_CR1_CEN;  // 启动 TIM2
}

/**
  * @brief  微秒延时
  * @param  us: 延时时间，单位: 微秒
  */
void Delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us);
}

/**
  * @brief  毫秒延时
  * @param  ms: 延时时间，单位: 毫秒
  */
void Delay_ms(uint32_t ms)
{
    while (ms--) {
        Delay_us(1000);
    }
}

/**
  * @brief  秒延时
  * @param  s: 延时时间，单位: 秒
  */
void Delay_s(uint32_t s)
{
    while (s--) {
        Delay_ms(1000);
    }
}
