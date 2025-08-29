#ifndef __USART_1_H
#define __USART_1_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include "Delay.h"
#include "port.h"
#include "mbport.h"
#include "mb.h"

#define SENDBUFF_SIZE   5000

void USART1_Init(uint32_t baudrate);
void USART1_SendChar(uint8_t ch);
void USART1_SendString(char *str);
void USART1_Printf(const char *fmt, ...);


#endif

