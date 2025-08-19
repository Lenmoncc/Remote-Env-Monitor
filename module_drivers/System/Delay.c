#include "Delay.h"


// ϵͳʱ��Ƶ�ʣ���λ��Hz��
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
  * @brief  ��ʼ��SysTick��ʱ��
  * @param  ��
  * @retval ��
  */
void Delay_Init(void) {
    // ����SysTickʱ��ԴΪ�ں�ʱ�ӣ�168MHz��
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
}

/**
  * @brief  ΢�뼶��ʱ
  * @param  us ��ʱʱ����΢�룩����Χ��1~798915
  * @retval ��
  */
void Delay_us(uint32_t us) {
    // ������Ҫ��ʱ����������168MHzʱ1us=168���ڣ�
    uint32_t ticks = us * (SYSTEM_CLOCK_FREQ / 1000000);
    
    // ������װ��ֵ��24λ���ֵ0xFFFFFF��
    SysTick->LOAD = (ticks - 1) & 0xFFFFFF;
    
    // ��յ�ǰֵ
    SysTick->VAL = 0;
    
    // ������ʱ��
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    
    // �ȴ��������
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    
    // �رն�ʱ��
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/**
  * @brief  ���뼶��ʱ
  * @param  ms ��ʱʱ�������룩����Χ��1~4294967295
  * @retval ��
  */
void Delay_ms(uint32_t ms) {
    // ����С��798ms����ʱֱ��ʹ��us����ʱ
    if (ms <= 798) {
        Delay_us(ms * 1000);
    } 
    // ����798msʱʹ��ѭ���Ż�
    else {
        while (ms--) {
            // ÿ����ʱ798ms��������ֵ��
            uint32_t chunks = ms > 798 ? 798 : ms;
            Delay_us(chunks * 1000);
            ms -= chunks;
        }
    }
}

/**
  * @brief  �뼶��ʱ
  * @param  s ��ʱʱ�����룩����Χ��1~4294967295
  * @retval ��
  */
void Delay_s(uint32_t s) {
    while (s--) {
        Delay_ms(1000);
    }
}