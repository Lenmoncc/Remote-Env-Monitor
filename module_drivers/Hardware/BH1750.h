#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f4xx.h"
#include <stdint.h> 
#include "IIC_1.h"
#include "UART_5.h"
#include "Delay.h"
#include "FreeRTOS.h"
#include "task.h"

extern int lux;

#define BH1750_ADDR_H 0x46  
#define BH1750_ADDR_L 0x23  // Ĭ��ʹ��

#define POWER_DOWN     0x00  // ����ģʽ
#define POWER_ON       0x01  // �ϵ�ģʽ���ȴ�����ָ�
#define RESET          0x07  // �������ݼĴ��������ϵ�ģʽ��Ч��

// ��������ģʽ
#define CONT_H_RES     0x10  // �����߷ֱ���ģʽ��1lx������120ms��
#define CONT_H_RES2    0x11  // �����߷ֱ���ģʽ2��0.5lx������120ms��
#define CONT_L_RES     0x13  // �����ͷֱ���ģʽ��4lx������16ms��

// ���β���ģʽ���������Զ��������ģʽ��
#define ONCE_H_RES     0x20  // ���θ߷ֱ���ģʽ
#define ONCE_H_RES2    0x21  // ���θ߷ֱ���ģʽ2
#define ONCE_L_RES     0x23  // ���εͷֱ���ģʽ

void BH1750_Init(void);                  
void BH1750_SendCmd(uint8_t cmd);        
float BH1750_ReadLight(void);            // ��λ��lx

#endif
