#include "ws2812.h"

uint16_t pwmIdx = 0;
__attribute__((aligned(4))) uint16_t pwmBuffer[PWM_BUFFER_SIZE];
LED_Color leds[LED_NUM]; 

void WS2812_Init()
{
	//GPIO配置
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed =GPIO_High_Speed;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	//定时器配置
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Period = 41;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = 0;
	
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);
	
	 //通道配置
	TIM_OCInitTypeDef TIM_OCInitStruct;
	TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStruct.TIM_Pulse = 0;
	TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCPolarity_High;
	 
	TIM_OC1Init(TIM1, &TIM_OCInitStruct);
	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
	TIM_Cmd(TIM1, ENABLE);
	TIM_CtrlPWMOutputs(TIM1, ENABLE);
	
	
	//DMA配置
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	DMA_InitTypeDef DMA_InitStruct = {
    .DMA_Channel = DMA_Channel_6,
    .DMA_PeripheralBaseAddr = (uint32_t)&TIM1->CCR1,
    .DMA_Memory0BaseAddr = (uint32_t)pwmBuffer,
    .DMA_DIR = DMA_DIR_MemoryToPeripheral,
    .DMA_BufferSize = PWM_BUFFER_SIZE,
    .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
    .DMA_MemoryInc = DMA_MemoryInc_Enable,
    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
    .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
    .DMA_Mode = DMA_Mode_Normal,
    .DMA_Priority = DMA_Priority_High,
    .DMA_FIFOMode = DMA_FIFOMode_Disable
	};
	DMA_Init(DMA2_Stream5, &DMA_InitStruct);
	TIM_DMACmd(TIM1, TIM_DMA_Update, ENABLE);
	DMA_Cmd(DMA2_Stream5, ENABLE);
	
	// 配置DMA中断优先级（高于FreeRTOS内核中断优先级）
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel = DMA2_Stream5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 6;  // 高优先级
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	
	DMA_ITConfig(DMA2_Stream5, DMA_IT_TC, ENABLE);
}

// DMA中断处理函数
void DMA2_Stream5_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5))
    {
        DMA_ClearITPendingBit(DMA2_Stream5, DMA_IT_TCIF5);
 
    }
}

void WS2812_Encode(uint8_t r, uint8_t g, uint8_t b) 
{
  uint32_t color = (g << 16) | (r << 8) | b;  
  
  for(int i = 23; i >= 0; i--) {
    // 1码: PWM高电平21/30 (≈0.9μs), 0码: PWM高电平8/30 (≈0.35μs)
    pwmBuffer[pwmIdx++] = (color & (1 << i)) ? 29 : 14;
  }
}

void WS2812_Update(void) 
{
  
  pwmIdx = 0; 
  
  // 1. 编码所有LED数据
  for(int i = 0; i < LED_NUM; i++) {
    WS2812_Encode(leds[i].r, leds[i].g, leds[i].b);
  }
  
  // 2. 添加RESET信号（>80μs低电平）
  for(int i = 0; i < 134; i++) { 
    pwmBuffer[pwmIdx++] = 0;
  }
  
  // 3. 启动DMA传输
  DMA_Cmd(DMA2_Stream5, DISABLE);
  DMA2_Stream5->NDTR = pwmIdx; // 设置传输数据量
  DMA2_Stream5->CR |= DMA_SxCR_EN; // 启动DMA
  
  // 临时禁用DMA中断，防止标志被中断清除
  DMA_ITConfig(DMA2_Stream5, DMA_IT_TC, DISABLE);
  DMA_Cmd(DMA2_Stream5, ENABLE);
  // 4. 等待传输完成
  while(DMA_GetFlagStatus(DMA2_Stream5, DMA_FLAG_TCIF5) == RESET);
  DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);

}


/**
  * @brief  HSV转RGB函数
  * @param  h: 色相 (0-360度)
  * @param  s: 饱和度 (0.0-1.0)
  * @param  v: 明度 (0.0-1.0)
  * @retval LED_Color结构体（GRB顺序）
  */
LED_Color HSVtoRGB(float h, float s, float v) {
    LED_Color rgb; // 改为返回LED_Color类型
    
    if(h >= 360.0f) h = 0.0f;
    
    if(s < 0.001f) {
        uint8_t val = (uint8_t)(v * 255);
        rgb.g = val;
        rgb.r = val;
        rgb.b = val;
        return rgb;
    }
    
    float h60 = h / 60.0f;
    int hi = (int)h60;
    float f = h60 - hi;
    
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    
    switch(hi) {
        case 0:
            rgb.g = (uint8_t)(v * 255); // g对应标准RGB的r
            rgb.r = (uint8_t)(t * 255); // r对应标准RGB的g
            rgb.b = (uint8_t)(p * 255); // b对应标准RGB的b
            break;
        case 1:
            rgb.g = (uint8_t)(q * 255);
            rgb.r = (uint8_t)(v * 255);
            rgb.b = (uint8_t)(p * 255);
            break;
        case 2:
            rgb.g = (uint8_t)(p * 255);
            rgb.r = (uint8_t)(v * 255);
            rgb.b = (uint8_t)(t * 255);
            break;
        case 3:
            rgb.g = (uint8_t)(p * 255);
            rgb.r = (uint8_t)(q * 255);
            rgb.b = (uint8_t)(v * 255);
            break;
        case 4:
            rgb.g = (uint8_t)(t * 255);
            rgb.r = (uint8_t)(p * 255);
            rgb.b = (uint8_t)(v * 255);
            break;
        default:
            rgb.g = (uint8_t)(v * 255);
            rgb.r = (uint8_t)(p * 255);
            rgb.b = (uint8_t)(q * 255);
            break;
    }
    
    return rgb;
}

// 彩虹渐变效果
void RainbowEffect(uint16_t offset) {
    for(uint16_t i = 0; i < LED_NUM; i++) {
        float hue = ((i + offset) % LED_NUM) * 360.0f / LED_NUM;
        leds[i] = HSVtoRGB(hue, 1.0f, 1.0f); // 直接赋值
    }
}

