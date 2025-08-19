#include "stm32f4xx.h"
#include "ws2812.h"
#include "Delay.h"


int main(void)
{
	SystemInit();
	Delay_Init();
	WS2812_Init();
	WS2812_Update();
	Delay_ms(500);
    
	uint16_t offset = 0;
    while(1) {
//		 leds[0] = (LED_Color){0, 255, 0}; // GRB˳����ɫ=0, ��ɫ=255, ��ɫ=0 �� ��ɫ
//        WS2812_Update();
//        Delay_ms(500);
        RainbowEffect(offset++);
        WS2812_Update();
        Delay_ms(1000);
    }
}








