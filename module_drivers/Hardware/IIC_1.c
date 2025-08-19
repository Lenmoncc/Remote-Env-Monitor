#include "IIC_1.h"
#include "UART_5.h"
#include "Delay.h"

void IIC1_Init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef  GPIO_InitStruct;  
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8|GPIO_Pin_9;           
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_OUT;        
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;        
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz; 
    GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;   
    GPIO_Init(GPIOB, &GPIO_InitStruct);                
    
    GPIO_SetBits(GPIOB,GPIO_Pin_8|GPIO_Pin_9); // SCL和SDA线拉高                                   
}


void MyIIC1_W_SCL(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)BitValue);		
	Delay_us(5);												
}

void MyIIC1_W_SDA(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)BitValue);		
	Delay_us(1);												
}


uint8_t MyIIC1_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);		
	Delay_us(1);												
	return BitValue;											
}


void MyIIC1_Start(void)
{
	MyIIC1_W_SDA(1);							
	MyIIC1_W_SCL(1);							
	MyIIC1_W_SDA(0);							
	MyIIC1_W_SCL(0);							
}


void MyIIC1_Stop(void)
{
	MyIIC1_W_SDA(0);							
	MyIIC1_W_SCL(1);							
	MyIIC1_W_SDA(1);							
}


void MyIIC1_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				
	{
		MyIIC1_W_SDA(Byte & (0x80 >> i));	
		MyIIC1_W_SCL(1);						
		MyIIC1_W_SCL(0);						
	}
}


uint8_t MyIIC1_ReceiveByte(void)
{
	uint8_t i, Byte = 0x00;					
	MyIIC1_W_SDA(1);							
	for (i = 0; i < 8; i ++)				
	{
		MyIIC1_W_SCL(1);						
		if (MyIIC1_R_SDA() == 1)				
			{Byte |= (0x80 >> i);}	
														
		MyIIC1_W_SCL(0);						
	}
	return Byte;							
}


void MyIIC1_Send_Ack(uint8_t AckBit)
{
	MyIIC1_W_SDA(AckBit);					
	MyIIC1_W_SCL(1);							
	MyIIC1_W_SCL(0);							
}


uint8_t MyIIC1_Receive_Ack(void)
{
	uint8_t AckBit;							
	MyIIC1_W_SDA(1);							
	MyIIC1_W_SCL(1);									
  AckBit = MyIIC1_R_SDA();					
	MyIIC1_W_SCL(0);							
	return AckBit;							
}

