#ifndef __AHT10_H
#define __AHT10_H

#include "stm32f4xx.h"
#include <stdint.h> 

#include "IIC_1.h"
#include "Delay.h"  
#include "UART_5.h"
#include "FreeRTOS.h"
#include "task.h"

#define AHT10_ADDRESS 0x38
#define AHT10_CMD_INIT 0XE1
#define AHT10_CMD_TRIGGER 0XAC
#define AHT10_CMD_READ 0X71
#define AHT10_CMD_WRITE 0X70
#define AHT10_INIT_PRAM1 0X08
#define AHT10_INIT_PRAM2 0X00
#define AHT10_TRIGGER_PRAM1 0X33
#define AHT10_TRIGGER_PRAM2 0X00
#define AHT10_CMD_SOFTRESET 0XBA

typedef struct {
    float humidity;    
    float temperature; 
}AHT10_Data;

extern AHT10_Data data;

uint8_t aht10_init(void);
void aht10_soft_reset(void);
uint8_t aht10_get_data(AHT10_Data* data);


#endif