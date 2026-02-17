/**
 * @file    mb_user.c
 * @brief   FreeModbus 用户回调函数 - 传感器数据映射
 *
 * Modbus 寄存器映射（输入寄存器，只读）:
 *   0x0000-0x0001: AHT10 温度（int16_t，单位 0.01℃）
 *   0x0002-0x0003: AHT10 湿度（int16_t，单位 0.01%RH）
 *   0x0004-0x0005: BH1750 光照（int16_t，单位 1 lux）
 *   0x0006-0x0007: BMP280 气压（int16_t，单位 1 hPa）
 *
 * 数据来自 SensorTask 采集，通过 task_manager 中的数据访问接口获取
 * 所有数据访问都是线程安全的（使用互斥量保护）
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

#include "mb_user.h"
#include "task_manager.h"
#include "UART_5.h"

/*=================================================================================
 * Modbus 寄存器配置
 =================================================================================*/

/* 输入寄存器（只读） */
#define REG_INPUT_START       0x0000
#define REG_INPUT_NREGS       8

/* Holding 寄存器（可读可写） */
#define REG_HOLDING_START     0x0000
#define REG_HOLDING_NREGS     8

/* 线圈（可读可写） */
#define REG_COILS_START       0x0000
#define REG_COILS_SIZE        16

/* 离散输入（只读） */
#define REG_DISCRETE_START    0x0000
#define REG_DISCRETE_SIZE     16

/*=================================================================================
 * 私有缓冲区
 =================================================================================*/

/* 输入寄存器缓冲区 */
static uint16_t usRegInputBuf[REG_INPUT_NREGS] = {0};
static uint16_t usRegInputStart = REG_INPUT_START;

/* Holding 寄存器缓冲区 */
static uint16_t usRegHoldingBuf[REG_HOLDING_NREGS] = {0};
static uint16_t usRegHoldingStart = REG_HOLDING_START;

/* 线圈缓冲区 */
static uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8] = {0};

/* 离散输入缓冲区 */
static uint8_t ucRegDiscreteBuf[REG_DISCRETE_SIZE / 8] = {0};

/*=================================================================================
 * 回调函数实现
 =================================================================================*/

/**
 * @brief  输入寄存器读取回调（Modbus 功能码 04）
 *
 * 该函数在 Modbus 主机发起读输入寄存器请求时被调用
 * 负责将传感器数据编码为 Modbus 格式并返回
 *
 * @param  pucRegBuffer：输出缓冲区，存放寄存器数据
 * @param  usAddress：起始寄存器地址
 * @param  usNRegs：要读取的寄存器数量
 * @return MB_ENOERR 成功，MB_ENOREG 地址不存在
 */
eMBErrorCode eMBRegInputCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;

    /* 检查地址范围 */
    if ((usAddress >= REG_INPUT_START) &&
        ((usAddress + usNRegs) <= (REG_INPUT_START + REG_INPUT_NREGS))) {

        /* 获取传感器数据快照（线程安全） */
        SensorData_t sensorData = SensorData_GetSnapshot();

        /* 清空缓冲区 */
        int i;
        for (i = 0; i < usNRegs; i++) {
            usRegInputBuf[i] = 0;
        }

        /* 根据地址填充寄存器数据 */
        for (i = 0; i < usNRegs; i++) {
            USHORT usTemp = usAddress + i;

            switch (usTemp) {
                /* 0x0000: AHT10 温度（℃ × 100） */
                case 0x0000:
                    usRegInputBuf[i] = (uint16_t)(sensorData.temperature_aht10 * 100);
                    break;

                /* 0x0001: AHT10 湿度（%RH × 100） */
                case 0x0001:
                    usRegInputBuf[i] = (uint16_t)(sensorData.humidity_aht10 * 100);
                    break;

                /* 0x0002: BH1750 光照（lux） */
                case 0x0002:
                    usRegInputBuf[i] = (uint16_t)sensorData.lux;
                    break;

                /* 0x0003: BMP280 温度（℃ × 100） */
                case 0x0003:
                    usRegInputBuf[i] = (uint16_t)(sensorData.temperature_bmp280 * 100);
                    break;

                /* 0x0004: BMP280 气压（hPa） */
                case 0x0004:
                    usRegInputBuf[i] = (uint16_t)sensorData.pressure_bmp280;
                    break;

                /* 0x0005: SGP30 CO₂ 等效值（ppm） */
                case 0x0005:
                    usRegInputBuf[i] = (uint16_t)sensorData.co2_eq;
                    break;

                /* 0x0006: SGP30 TVOC（ppb） */
                case 0x0006:
                    usRegInputBuf[i] = (uint16_t)sensorData.tvoc;
                    break;

                /* 0x0007: 数据更新时间戳（滴答数） */
                case 0x0007:
                    usRegInputBuf[i] = (uint16_t)(sensorData.lastUpdateTime & 0xFFFF);
                    break;

                default:
                    eStatus = MB_ENOREG;
                    break;
            }
        }

        /* 将数据复制到 Modbus 输出缓冲区（网络字节序：大端） */
        for (i = 0; i < usNRegs; i++) {
            pucRegBuffer[i * 2] = (UCHAR)(usRegInputBuf[i] >> 8);
            pucRegBuffer[i * 2 + 1] = (UCHAR)(usRegInputBuf[i] & 0xFF);
        }

    } else {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/**
 * @brief  Holding 寄存器读写回调（Modbus 功能码 03/06/16/23）
 *
 * 该函数处理 Holding 寄存器的读和写操作
 * 可用于远程配置或控制
 *
 * @param  pucRegBuffer：数据缓冲区（读时为输出，写时为输入）
 * @param  usAddress：起始寄存器地址
 * @param  usNRegs：寄存器数量
 * @param  eMode：操作模式（MB_REG_READ 或 MB_REG_WRITE）
 * @return MB_ENOERR 成功，MB_ENOREG 地址不存在
 */
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs,
                              eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ((usAddress >= REG_HOLDING_START) &&
        ((usAddress + usNRegs) <= (REG_HOLDING_START + REG_HOLDING_NREGS))) {

        iRegIndex = (int)(usAddress - usRegHoldingStart);

        switch (eMode) {
            /* 读操作 */
            case MB_REG_READ:
                for (iRegIndex = 0; iRegIndex < usNRegs; iRegIndex++) {
                    pucRegBuffer[iRegIndex * 2] = (UCHAR)(usRegHoldingBuf[iRegIndex] >> 8);
                    pucRegBuffer[iRegIndex * 2 + 1] = (UCHAR)(usRegHoldingBuf[iRegIndex] & 0xFF);
                }
                break;

            /* 写操作 */
            case MB_REG_WRITE:
                for (iRegIndex = 0; iRegIndex < usNRegs; iRegIndex++) {
                    usRegHoldingBuf[iRegIndex] = (USHORT)((pucRegBuffer[iRegIndex * 2] << 8) |
                                                           pucRegBuffer[iRegIndex * 2 + 1]);
                }
                break;
        }

    } else {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/**
 * @brief  线圈读写回调（Modbus 功能码 01/05/15）
 *
 * @param  pucRegBuffer：数据缓冲区
 * @param  usAddress：线圈地址
 * @param  usNCoils：线圈数量
 * @param  eMode：操作模式
 * @return MB_ENOERR 或 MB_ENOREG
 */
eMBErrorCode eMBRegCoilsCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils,
                            eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if ((usAddress >= REG_COILS_START) &&
        ((usAddress + usNCoils) <= (REG_COILS_START + REG_COILS_SIZE))) {

        iRegIndex = (int)(usAddress - REG_COILS_START);

        switch (eMode) {
            case MB_REG_READ:
                for (iRegIndex = 0; iRegIndex < usNCoils; iRegIndex++) {
                    UCHAR ucByte = iRegIndex / 8;
                    UCHAR ucBit = iRegIndex % 8;

                    if (ucByte < sizeof(ucRegCoilsBuf)) {
                        if (ucRegCoilsBuf[ucByte] & (1 << ucBit)) {
                            pucRegBuffer[iRegIndex] = 0xFF;
                        } else {
                            pucRegBuffer[iRegIndex] = 0x00;
                        }
                    }
                }
                break;

            case MB_REG_WRITE:
                for (iRegIndex = 0; iRegIndex < usNCoils; iRegIndex++) {
                    UCHAR ucByte = iRegIndex / 8;
                    UCHAR ucBit = iRegIndex % 8;

                    if (ucByte < sizeof(ucRegCoilsBuf)) {
                        if (pucRegBuffer[iRegIndex] != 0) {
                            ucRegCoilsBuf[ucByte] |= (1 << ucBit);
                        } else {
                            ucRegCoilsBuf[ucByte] &= ~(1 << ucBit);
                        }
                    }
                }
                break;
        }

    } else {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/**
 * @brief  离散输入读取回调（Modbus 功能码 02）
 *
 * @param  pucRegBuffer：输出缓冲区
 * @param  usAddress：起始地址
 * @param  usNDiscretes：离散输入数量
 * @return MB_ENOERR 或 MB_ENOREG
 */
eMBErrorCode eMBRegDiscreteCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscretes)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex = 0;

    if ((usAddress >= REG_DISCRETE_START) &&
        ((usAddress + usNDiscretes) <= (REG_DISCRETE_START + REG_DISCRETE_SIZE))) {

        iRegIndex = (int)(usAddress - REG_DISCRETE_START);

        while (usNDiscretes > 0) {
            UCHAR ucByte = iRegIndex / 8;
            UCHAR ucBit = iRegIndex % 8;

            if (ucByte < sizeof(ucRegDiscreteBuf)) {
                if (ucRegDiscreteBuf[ucByte] & (1 << ucBit)) {
                    pucRegBuffer[iRegIndex] = 0xFF;
                } else {
                    pucRegBuffer[iRegIndex] = 0x00;
                }
            }

            iRegIndex++;
            usNDiscretes--;
        }

    } else {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}
