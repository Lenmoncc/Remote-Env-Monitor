#include "stm32f4xx.h"
#include "AHT10.h"
#include "IIC_1.h"
#include "Delay.h"  
#include "UART_5.h"
#include "USART_1.h"
#include "sgp30.h"
#include "BH1750.h"
#include "BMP280.h"



int main(void)
{
	SystemInit();
	IIC1_Init();
	Delay_Init();
	UART5_Init();
	USART1_Init(115200);
	sgp30_data_show_init();
	BH1750_Init();
	BMP280_Init(&bmp_config);

	UART5_SendString("UART5 Communication Ready!\r\n");


	

    while(1)  
	{	
		//aht10_get_data(&data);
		sgp30_data_show();
		BH1750_ReadLight();  
		BMP280_GetData();

		USART1_SendString("Hello World!\r\n");
		Delay_ms(1000);
//		if(uart5_rx_flag) { // 检查是否有新数据
//        	UART5_SendChar(uart5_rx_buf); // 回发数据
//        	uart5_rx_flag = 0; // 清除标志
//		}
		//char received = UART5_ReceiveChar();
		//UART5_SendChar(received);

    }
}








