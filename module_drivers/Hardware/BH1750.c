#include "BH1750.h"

void BH1750_Init(void) {
    //IIC1_Init();               
    BH1750_SendCmd(POWER_ON);  
    Delay_ms(10);              
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
    float lux;


    BH1750_SendCmd(CONT_H_RES);
    Delay_ms(120);  

   
    MyIIC1_Start();                       
    MyIIC1_SendByte((BH1750_ADDR_L << 1) | 0x01);  
    MyIIC1_Receive_Ack();                 
    data_high = MyIIC1_ReceiveByte();     
    MyIIC1_Send_Ack(0);                   
    data_low = MyIIC1_ReceiveByte();      
    MyIIC1_Send_Ack(1);                   
    MyIIC1_Stop();                        


    raw_data = (data_high << 8) | data_low;  
    lux = raw_data / 1.2f;                   
	UART5_Printf("Light: %.1f lx\r\n", lux);
    return lux;
}

