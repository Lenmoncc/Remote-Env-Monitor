#include "BH1750.h"
#include "USART_1.h"

int lux = 0.0f;

void BH1750_Init(void) {
    //IIC1_Init();               
    BH1750_SendCmd(POWER_ON);  
    Delay_ms(10);  
	//vTaskDelay(pdMS_TO_TICKS(10));
}

void BH1750_SendCmd(uint8_t cmd) {
    MyIIC1_Start();                       
    MyIIC1_SendByte(BH1750_ADDR_L << 1);  
    MyIIC1_Receive_Ack();                 
    MyIIC1_SendByte(cmd);                 
    MyIIC1_Receive_Ack();                 
    MyIIC1_Stop();                        
}


float BH1750_ReadLight(void) {
    uint8_t data_high, data_low;
    uint16_t raw_data;
    BH1750_SendCmd(CONT_H_RES);
    Delay_ms(120);  
	//vTaskDelay(pdMS_TO_TICKS(120));

   
    MyIIC1_Start();                       
    MyIIC1_SendByte((BH1750_ADDR_L << 1) | 0x01);  
    MyIIC1_Receive_Ack();                 
    data_high = MyIIC1_ReceiveByte();     
    MyIIC1_Send_Ack(0);                   
    data_low = MyIIC1_ReceiveByte();      
    MyIIC1_Send_Ack(1);                   
    MyIIC1_Stop();                        


    raw_data = (data_high << 8) | data_low;  
    lux = (int)(raw_data / 1.2f);                   
	//printf("Light: %d lx\r\n", (int)lux);
    return lux;
}

