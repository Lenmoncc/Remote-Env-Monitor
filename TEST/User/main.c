#include "stm32f4xx.h"
#include "ws2812.h"
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"


// 任务堆栈大小
#define LED_TASK_STACK_SIZE 512
// 任务优先级
#define LED_TASK_PRIORITY 0

// LED任务函数声明
void vLEDTask(void *pvParameters);


int main(void)
{
	
	Delay_Init();
	WS2812_Init();
	WS2812_Update();
	
    // 创建LED任务
    xTaskCreate(vLEDTask, "LED Task", LED_TASK_STACK_SIZE, 
                NULL, LED_TASK_PRIORITY, NULL);
	
    // 启动任务调度器
    vTaskStartScheduler();


    while(1) 
    {
    }

}

// LED任务实现
void vLEDTask(void *pvParameters)
{
    uint16_t offset = 0;
    
    // 任务循环
    for(;;)
    {
        RainbowEffect(offset++);
        WS2812_Update();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}






