#ifndef __WS2812_H
#define __WS2812_H

#include "stm32f4xx.h"
#include <math.h>

#define LED_NUM 12       
#define PWM_BUFFER_SIZE (LED_NUM * 24 + 50) 
extern uint16_t pwmBuffer[PWM_BUFFER_SIZE];  

typedef struct {
  uint8_t g; 
  uint8_t r;
  uint8_t b;
} LED_Color;
extern LED_Color leds [LED_NUM];


void WS2812_Init(void);
void WS2812_Encode(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Update(void);
LED_Color HSVtoRGB(float h, float s, float v);
void RainbowEffect(uint16_t offset);

#endif
