#include "mb.h"
#include "mbport.h"

/* --------- 模拟寄存器区域 --------- */
#define REG_HOLDING_START   0x0001
#define REG_HOLDING_NREGS   10
static USHORT usRegHoldingBuf[REG_HOLDING_NREGS];

#define REG_INPUT_START     0x0001
#define REG_INPUT_NREGS     10
static USHORT usRegInputBuf[REG_INPUT_NREGS];

#define REG_COIL_START      0x0001
#define REG_COIL_NCOILS     16
static UCHAR ucRegCoilBuf[REG_COIL_NCOILS/8];

#define REG_DISC_START      0x0001
#define REG_DISC_NDISCRETES 16
static UCHAR ucRegDiscBuf[REG_DISC_NDISCRETES/8];

/* --------- 保持寄存器回调 --------- */
eMBErrorCode eMBRegHoldingCB( UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int i;

    if( (usAddress >= REG_HOLDING_START) &&
        (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS) )
    {
        USHORT iRegIndex = (USHORT)(usAddress - REG_HOLDING_START);

        if( eMode == MB_REG_READ )
        {
            for( i = 0; i < usNRegs; i++ )
            {
                *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf[iRegIndex + i] >> 8);
                *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf[iRegIndex + i] & 0xFF);
            }
        }
        else if( eMode == MB_REG_WRITE )
        {
            for( i = 0; i < usNRegs; i++ )
            {
                usRegHoldingBuf[iRegIndex + i] = 
                    (((USHORT)*pucRegBuffer++) << 8);
                usRegHoldingBuf[iRegIndex + i] |= 
                    ((USHORT)*pucRegBuffer++);
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/* --------- 输入寄存器回调 --------- */
eMBErrorCode eMBRegInputCB( UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNRegs )
{
    eMBErrorCode eStatus = MB_ENOERR;
    int i;

    if( (usAddress >= REG_INPUT_START) &&
        (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS) )
    {
        USHORT iRegIndex = (USHORT)(usAddress - REG_INPUT_START);
        for( i = 0; i < usNRegs; i++ )
        {
            *pucRegBuffer++ = (UCHAR)(usRegInputBuf[iRegIndex + i] >> 8);
            *pucRegBuffer++ = (UCHAR)(usRegInputBuf[iRegIndex + i] & 0xFF);
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/* --------- 线圈寄存器回调 --------- */
eMBErrorCode eMBRegCoilsCB( UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT i, bitIndex;

    if( (usAddress >= REG_COIL_START) &&
        (usAddress + usNCoils <= REG_COIL_START + REG_COIL_NCOILS) )
    {
        bitIndex = usAddress - REG_COIL_START;
        if( eMode == MB_REG_READ )
        {
            for( i = 0; i < usNCoils; i++, bitIndex++ )
            {
                UCHAR byte = ucRegCoilBuf[bitIndex/8];
                *pucRegBuffer |= ((byte >> (bitIndex%8)) & 0x01) << (i%8);

                if( (i % 8) == 7 )
                    pucRegBuffer++;
            }
        }
        else if( eMode == MB_REG_WRITE )
        {
            for( i = 0; i < usNCoils; i++, bitIndex++ )
            {
                if( (*pucRegBuffer >> (i%8)) & 0x01 )
                    ucRegCoilBuf[bitIndex/8] |= (1 << (bitIndex%8));
                else
                    ucRegCoilBuf[bitIndex/8] &= ~(1 << (bitIndex%8));

                if( (i % 8) == 7 )
                    pucRegBuffer++;
            }
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

/* --------- 离散输入回调 --------- */
eMBErrorCode eMBRegDiscreteCB( UCHAR *pucRegBuffer, USHORT usAddress,
                               USHORT usNDiscrete )
{
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT i, bitIndex;

    if( (usAddress >= REG_DISC_START) &&
        (usAddress + usNDiscrete <= REG_DISC_START + REG_DISC_NDISCRETES) )
    {
        bitIndex = usAddress - REG_DISC_START;
        for( i = 0; i < usNDiscrete; i++, bitIndex++ )
        {
            UCHAR byte = ucRegDiscBuf[bitIndex/8];
            *pucRegBuffer |= ((byte >> (bitIndex%8)) & 0x01) << (i%8);

            if( (i % 8) == 7 )
                pucRegBuffer++;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}
