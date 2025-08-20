#ifndef __USART_1_H
#define __USART_1_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include "Delay.h"

// 接收缓冲区大小
#define RX_BUFFER_SIZE 256

void USART1_Init(uint32_t baudrate);
void USART1_SendChar(uint8_t ch);
void USART1_SendString(char *str);
void USART1_Printf(const char *fmt, ...);
uint8_t USART1_ReadByte(void);
uint16_t USART1_Available(void);
void USART1_Flush(void);

// 中断处理函数声明
void USART1_IRQHandler(void);

#endif

