#ifndef __UART_5_H
#define __UART_5_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>

extern volatile char uart5_rx_buf; // 接收缓存
extern volatile uint8_t uart5_rx_flag; // 接收完成标志

void SystemClock_Init(void);
void UART5_Init(void);
void UART5_SendChar(char c);
void UART5_SendString(char *str);
char UART5_ReceiveChar(void);
int fputc(int ch, FILE *f);
void UART5_Printf(char *format, ...);

#endif
