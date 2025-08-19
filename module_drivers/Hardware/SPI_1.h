#ifndef __SPI_1_H
#define __SPI_1_H

#include "stm32f4xx.h"

void MySPI_Init(void);
void MySPI_Start(void);
void MySPI_Stop(void);
uint8_t MySPI_SwapByte(uint8_t ByteSend);

#endif