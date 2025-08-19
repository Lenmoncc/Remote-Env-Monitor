#ifndef __SGP30_H
#define __SGP30_H

#include "stm32f4xx.h"
#include "IIC_1.h"
#include "UART_5.h"


#define SGP30_ADDR_W 0xB0  
#define SGP30_ADDR_R 0xB1

// 命令定义（16位，MSB在前）
#define SGP30_CMD_INIT       0x2003  // 初始化空气质量测量
#define SGP30_CMD_MEASURE_IAQ 0x2008 // 测量TVOC和CO2eq
#define SGP30_CMD_GET_BASELINE 0x2015 // 读取基线值
#define SGP30_CMD_SET_HUMIDITY 0x2061 // 设置绝对湿度补偿

void sgp30_data_show_init(void);
void sgp30_data_show(void);
void HandleInitPhaseData(uint16_t co2, uint16_t tvoc);
uint8_t SGP30_Init(void);
uint8_t SGP30_MeasureIAQ(uint16_t *co2eq, uint16_t *tvoc);
uint8_t SGP30_SetHumidity(uint32_t abs_humidity); // 绝对湿度（g/m3，固定点格式）


#endif