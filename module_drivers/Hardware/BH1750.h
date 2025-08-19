#ifndef __BH1750_H
#define __BH1750_H

#include "stm32f4xx.h"
#include <stdint.h> 
#include "IIC_1.h"
#include "UART_5.h"
#include "Delay.h"


#define BH1750_ADDR_H 0x46  
#define BH1750_ADDR_L 0x23  // 默认使用

#define POWER_DOWN     0x00  // 掉电模式
#define POWER_ON       0x01  // 上电模式（等待测量指令）
#define RESET          0x07  // 重置数据寄存器（仅上电模式有效）

// 连续测量模式
#define CONT_H_RES     0x10  // 连续高分辨率模式（1lx，典型120ms）
#define CONT_H_RES2    0x11  // 连续高分辨率模式2（0.5lx，典型120ms）
#define CONT_L_RES     0x13  // 连续低分辨率模式（4lx，典型16ms）

// 单次测量模式（测量后自动进入掉电模式）
#define ONCE_H_RES     0x20  // 单次高分辨率模式
#define ONCE_H_RES2    0x21  // 单次高分辨率模式2
#define ONCE_L_RES     0x23  // 单次低分辨率模式

void BH1750_Init(void);                  
void BH1750_SendCmd(uint8_t cmd);        
float BH1750_ReadLight(void);            // 单位：lx

#endif
