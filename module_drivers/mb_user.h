#ifndef __MB_USER_H
#define __MB_USER_H

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdarg.h>
#include "Delay.h"
#include "port.h"
#include "mb.h"
#include "mbport.h"

eMBErrorCode eMBRegHoldingCB( UCHAR *pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode );
eMBErrorCode eMBRegInputCB( UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNRegs );
eMBErrorCode eMBRegCoilsCB( UCHAR *pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode );
eMBErrorCode eMBRegDiscreteCB( UCHAR *pucRegBuffer, USHORT usAddress,
                               USHORT usNDiscrete );

#endif