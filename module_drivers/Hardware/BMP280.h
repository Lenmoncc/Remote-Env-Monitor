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


// BMP280�Ĵ�����ַ����
#define BMP280_REG_ID        0xD0    // оƬID�Ĵ������̶�ֵ0x58��
#define BMP280_REG_RESET     0xE0    // ��λ�Ĵ�����д��0xB6������λ��
#define BMP280_REG_STATUS    0xF3    // ״̬�Ĵ���
#define BMP280_REG_CTRL_MEAS 0xF4    // �������ƼĴ���
#define BMP280_REG_CONFIG    0xF5    // ���üĴ���
#define BMP280_REG_PRESS_MSB 0xF7    // ѹ�����ݸ�λ
#define BMP280_REG_TEMP_MSB  0xFA    // �¶����ݸ�λ
#define BMP280_REG_CALIB     0x88    // У׼������ʼ��ַ����24�ֽڣ�

// У׼�����ṹ�壨�����3.11.2�ڣ�
typedef struct {
    uint16_t dig_T1;  // �¶�У׼����
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;  // ѹ��У׼����
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} BMP280_CalibTypeDef;

// ԭʼ���ݽṹ��
typedef struct {
    uint32_t press;  // 20λѹ��ԭʼֵ
    uint32_t temp;   // 20λ�¶�ԭʼֵ
} BMP280_RawDataTypedef;

// �������ݽṹ�壨������
typedef struct {
    int temp;  // �¶ȣ��棩
    int press; // ѹ����hPa��
    int altitude;
} BMP280_DataTypedef;

extern BMP280_DataTypedef bmp280_data;

// ���ò����ṹ��
typedef struct {
    uint8_t osrs_t;  // �¶ȹ�������0~5����Ӧx1~x16��
    uint8_t osrs_p;  // ѹ����������0~5����Ӧx1~x16��
    uint8_t mode;    // ����ģʽ��0��˯�ߣ�1/2��ǿ�ƣ�3��������
    uint8_t filter;  // �˲���ϵ����0~4����Ӧ��/2/4/8/16��
    uint8_t t_sb;    // ����ʱ�䣨0~7����Ӧ0.5~4000ms��
} BMP280_ConfigTypeDef;

extern BMP280_ConfigTypeDef bmp_config;

// ȫ�ֱ�������
extern BMP280_CalibTypeDef bmp280_calib;
extern int32_t t_fine;  // �¶Ȳ����м����

// ��������
uint8_t BMP280_Init(BMP280_ConfigTypeDef *config);  // ��ʼ��������
void BMP280_Reset(void);                            // ��λ������
uint8_t BMP280_ReadID(void);                         // ��ȡоƬID
uint8_t BMP280_ReadCalib(void);                      // ��ȡУ׼����
void BMP280_WriteReg(uint8_t reg, uint8_t data);     // д��Ĵ���
uint8_t BMP280_ReadReg(uint8_t reg);                 // ��ȡ�Ĵ���
void BMP280_ReadMultiReg(uint8_t reg, uint8_t *buf, uint8_t len);  // ������ȡ�Ĵ���
BMP280_DataTypedef BMP280_GetData(void);             // ��ȡ�����������
float BMP280_CompensateTemp(int32_t raw_temp);       // �¶Ȳ���
float BMP280_CompensatePress(int32_t raw_press);     // ѹ������


#endif