///*
//#include "Delay.h"


//// ???????????��??Hz??
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
//  * @param  us ???????????????��??1~798915
//  * @retval ??
//  */
//void Delay_us(uint32_t us) {
//    // ????????????????????168MHz?1us=168?????
//    uint32_t ticks = us * (SYSTEM_CLOCK_FREQ / 1000000);
//    
//    // ????????????24��????0xFFFFFF??
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
//  * @param  ms ????????????????��??1~4294967295
//  * @retval ??
//  */
//void Delay_ms(uint32_t ms) {
//    // ????��??798ms???????????us?????
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
//  * @param  s ??????????????��??1~4294967295
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
  * @brief  ʹ�� TIM2 ��ʼ��Ϊ 1MHz ��������1us ������
  * @note   ȷ���� RCC ʱ�����Ѿ������� TIM2 ʱ��
  */
void Delay_Init(void)
{
    // �� TIM2 ʱ��
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // ��λ TIM2
    TIM2->CR1 = 0;
    TIM2->PSC = (SYSTEM_CORE_CLOCK / 1000000) - 1;  // ��Ƶ�� 1MHz (1us per tick)
    TIM2->ARR = 0xFFFFFFFF;  // �Զ���װ�����ֵ
    TIM2->EGR = TIM_EGR_UG;  // ���¼Ĵ���
    TIM2->CR1 |= TIM_CR1_CEN;  // ���� TIM2
}

/**
  * @brief  ΢����ʱ
  * @param  us: ��ʱʱ�䣬��λ: ΢��
  */
void Delay_us(uint32_t us)
{
    uint32_t start = TIM2->CNT;
    while ((TIM2->CNT - start) < us);
}

/**
  * @brief  ������ʱ
  * @param  ms: ��ʱʱ�䣬��λ: ����
  */
void Delay_ms(uint32_t ms)
{
    while (ms--) {
        Delay_us(1000);
    }
}

/**
  * @brief  ����ʱ
  * @param  s: ��ʱʱ�䣬��λ: ��
  */
void Delay_s(uint32_t s)
{
    while (s--) {
        Delay_ms(1000);
    }
}
