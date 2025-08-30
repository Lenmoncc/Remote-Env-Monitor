#ifndef __SGP30_H
#define __SGP30_H

#include "stm32f4xx.h"
#include "IIC_1.h"
#include "UART_5.h"
#include "FreeRTOS.h"
#include "task.h"
#include "USART_1.h"

extern int g_co2eq;
extern int g_tvoc;
extern uint8_t g_init_flag;

#define SGP30_ADDR_W 0xB0  
#define SGP30_ADDR_R 0xB1

// ����壨16λ��MSB��ǰ��
#define SGP30_CMD_INIT       0x2003  // ��ʼ��������������
#define SGP30_CMD_MEASURE_IAQ 0x2008 // ����TVOC��CO2eq
#define SGP30_CMD_GET_BASELINE 0x2015 // ��ȡ����ֵ
#define SGP30_CMD_SET_HUMIDITY 0x2061 // ���þ���ʪ�Ȳ���

void sgp30_data_show_init(void);
void sgp30_data_show(void);
void HandleInitPhaseData(uint16_t co2, uint16_t tvoc);
uint8_t SGP30_Init(void);
uint8_t SGP30_MeasureIAQ(int *co2eq, int *tvoc);
uint8_t SGP30_SetHumidity(uint32_t abs_humidity); // ����ʪ�ȣ�g/m3���̶����ʽ��


#endif