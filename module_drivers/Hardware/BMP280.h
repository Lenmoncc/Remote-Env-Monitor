#ifndef __BMP280_H
#define __BMP280_H

#include "stm32f4xx.h"
#include <math.h>
#include "SPI_1.h"
#include "Delay.h"  
#include "UART_5.h"
#include "FreeRTOS.h"
#include "task.h"
#include "USART_1.h"


// BMP280寄存器地址定义
#define BMP280_REG_ID        0xD0    // 芯片ID寄存器（固定值0x58）
#define BMP280_REG_RESET     0xE0    // 复位寄存器（写入0xB6触发复位）
#define BMP280_REG_STATUS    0xF3    // 状态寄存器
#define BMP280_REG_CTRL_MEAS 0xF4    // 测量控制寄存器
#define BMP280_REG_CONFIG    0xF5    // 配置寄存器
#define BMP280_REG_PRESS_MSB 0xF7    // 压力数据高位
#define BMP280_REG_TEMP_MSB  0xFA    // 温度数据高位
#define BMP280_REG_CALIB     0x88    // 校准参数起始地址（共24字节）

// 校准参数结构体（规格书3.11.2节）
typedef struct {
    uint16_t dig_T1;  // 温度校准参数
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;  // 压力校准参数
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibTypeDef;

// 原始数据结构体
typedef struct {
    uint32_t press;  // 20位压力原始值
    uint32_t temp;   // 20位温度原始值
} BMP280_RawDataTypedef;

// 测量数据结构体（补偿后）
typedef struct {
    int temp;  // 温度（℃）
    int press; // 压力（hPa）
    int altitude;
} BMP280_DataTypedef;

extern BMP280_DataTypedef bmp280_data;

// 配置参数结构体
typedef struct {
    uint8_t osrs_t;  // 温度过采样（0~5，对应x1~x16）
    uint8_t osrs_p;  // 压力过采样（0~5，对应x1~x16）
    uint8_t mode;    // 工作模式（0：睡眠，1/2：强制，3：正常）
    uint8_t filter;  // 滤波器系数（0~4，对应关/2/4/8/16）
    uint8_t t_sb;    // 待机时间（0~7，对应0.5~4000ms）
} BMP280_ConfigTypeDef;

extern BMP280_ConfigTypeDef bmp_config;

// 全局变量声明
extern BMP280_CalibTypeDef bmp280_calib;
extern int32_t t_fine;  // 温度补偿中间变量

// 函数声明
uint8_t BMP280_Init(BMP280_ConfigTypeDef *config);  // 初始化传感器
void BMP280_Reset(void);                            // 复位传感器
uint8_t BMP280_ReadID(void);                         // 读取芯片ID
uint8_t BMP280_ReadCalib(void);                      // 读取校准参数
void BMP280_WriteReg(uint8_t reg, uint8_t data);     // 写入寄存器
uint8_t BMP280_ReadReg(uint8_t reg);                 // 读取寄存器
void BMP280_ReadMultiReg(uint8_t reg, uint8_t *buf, uint8_t len);  // 连续读取寄存器
BMP280_DataTypedef BMP280_GetData(void);             // 获取补偿后的数据
float BMP280_CompensateTemp(int32_t raw_temp);       // 温度补偿
float BMP280_CompensatePress(int32_t raw_press);     // 压力补偿


#endif