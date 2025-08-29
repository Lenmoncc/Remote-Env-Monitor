#include "stm32f4xx.h"
#include "AHT10.h"
#include "IIC_1.h"
#include "Delay.h"  
#include "UART_5.h"
#include "USART_1.h"
#include "sgp30.h"
#include "BH1750.h"
#include "BMP280.h"
#include "FreeRTOS.h"
#include "task.h"  
#include "mb.h"
#include "mbport.h"
#include "mb_user.h"


#define SENSOR_TASK_PRIORITY 1

#define SENSOR_TASK_STACK_SIZE 128

// 传感器数据采集任务
void SensorTask(void *pvParameters) {
    while(1) {
        // 循环读取各传感器数据（自带串口打印）
        aht10_get_data(&data);
        sgp30_data_show();
        BH1750_ReadLight();  
        BMP280_GetData();
        
        USART1_SendString("Hello World!\r\n");
        
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    // 硬件初始化（保持原有顺序）
    SystemInit();
    IIC1_Init();
    Delay_Init();
    UART5_Init();
    USART1_Init(115200);
    sgp30_data_show_init();
    BH1750_Init();
    BMP280_Init(&bmp_config);

    UART5_SendString("UART5 Communication Ready!\r\n");
    
    // 创建传感器任务
//    xTaskCreate(
//        SensorTask,           // 任务函数
//        "SensorTask",         // 任务名称（调试用）
//        SENSOR_TASK_STACK_SIZE, // 栈大小
//        NULL,                 // 传递给任务的参数
//        SENSOR_TASK_PRIORITY, // 优先级
//        NULL                  // 任务句柄（不需要可设为NULL）
//    );
    
    // 启动任务调度器
    //vTaskStartScheduler();
    
    // 如果调度器启动成功，不会执行到这里
	//USART1_SendString("Hello World!\r\n");
	printf("Hello World!\r\n");
    eMBInit(MB_RTU, 1, 1, 115200, MB_PAR_NONE);

    // 启动协议栈
    eMBEnable();
    while(1)
	{
		eMBPoll();
	}
}