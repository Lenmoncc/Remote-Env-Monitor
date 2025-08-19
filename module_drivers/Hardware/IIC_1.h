#ifndef __IIC_1_H
#define __IIC_1_H

#include "stm32f4xx.h"

void IIC1_Init(void);
void MyIIC1_W_SCL(uint8_t BitValue);
void MyIIC1_W_SDA(uint8_t BitValue);
uint8_t MyIIC1_R_SDA(void);
void MyIIC1_Start(void);
void MyIIC1_Stop(void);
void MyIIC1_SendByte(uint8_t Byte);
uint8_t MyIIC1_ReceiveByte(void);
uint8_t MyIIC1_Receive_Ack(void);
void MyIIC1_Send_Ack(uint8_t AckBit);
uint8_t MyIIC1_Receive_Ack(void);

#endif