#include "mb_user.h"

/* --------- ģ��Ĵ������� --------- */
//����Ĵ�����ʼ��ַ
#define REG_INPUT_START       0x0000
//����Ĵ�������
#define REG_INPUT_NREGS       8
//���ּĴ�����ʼ��ַ
#define REG_HOLDING_START     0x0000
//���ּĴ�������
#define REG_HOLDING_NREGS     8

//��Ȧ��ʼ��ַ
#define REG_COILS_START       0x0000
//��Ȧ����
#define REG_COILS_SIZE        16

//���ؼĴ�����ʵ��ַ
#define REG_DISCRETE_START    0x0000
//���ؼĴ�������
#define REG_DISCRETE_SIZE     16

/* Private variables ---------------------------------------------------------*/
//����Ĵ�������
uint16_t usRegInputBuf[REG_INPUT_NREGS] = {0x1000,0x1001,0x1002,0x1003,0x1004,0x1005,0x1006,0x1007};
//����Ĵ�����ʼ��ַ
uint16_t usRegInputStart = REG_INPUT_START;

//���ּĴ�������
uint16_t usRegHoldingBuf[REG_HOLDING_NREGS] = {0x147b,0x3f8e,0x147b,0x400e,0x1eb8,0x4055,0x147b,0x408e};
//���ּĴ�����ʼ��ַ
uint16_t usRegHoldingStart = REG_HOLDING_START;

//��Ȧ״̬
uint8_t ucRegCoilsBuf[REG_COILS_SIZE / 8] = {0x01,0x02};
//��������״̬
uint8_t ucRegDiscreteBuf[REG_DISCRETE_SIZE / 8] = {0x01,0x02};



/****************************************************************************
* ��	  �ƣ�eMBRegInputCB 
* ��    �ܣ���ȡ����Ĵ�������Ӧ�������� 04 eMBFuncReadInputRegister
* ��ڲ�����pucRegBuffer: ���ݻ�������������Ӧ����   
*						usAddress: �Ĵ�����ַ
*						usNRegs: Ҫ��ȡ�ļĴ�������
* ���ڲ�����
* ע	  �⣺��λ�������� ֡��ʽ��: SlaveAddr(1 Byte)+FuncCode(1 Byte)
*								+StartAddrHiByte(1 Byte)+StartAddrLoByte(1 Byte)
*								+LenAddrHiByte(1 Byte)+LenAddrLoByte(1 Byte)+
*								+CRCAddrHiByte(1 Byte)+CRCAddrLoByte(1 Byte)
*							3 ��
****************************************************************************/
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegInputStart );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = ( UCHAR )( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = ( UCHAR )( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

/****************************************************************************
* ��	  �ƣ�eMBRegHoldingCB 
* ��    �ܣ���Ӧ�������У�06 д���ּĴ��� eMBFuncWriteHoldingRegister 
*													16 д������ּĴ��� eMBFuncWriteMultipleHoldingRegister
*													03 �����ּĴ��� eMBFuncReadHoldingRegister
*													23 ��д������ּĴ��� eMBFuncReadWriteMultipleHoldingRegister
* ��ڲ�����pucRegBuffer: ���ݻ�������������Ӧ����   
*						usAddress: �Ĵ�����ַ
*						usNRegs: Ҫ��д�ļĴ�������
*						eMode: ������
* ���ڲ�����
* ע	  �⣺4 ��
****************************************************************************/
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
	eMBErrorCode    eStatus = MB_ENOERR;
	int             iRegIndex;


	if((usAddress >= REG_HOLDING_START)&&\
		((usAddress+usNRegs) <= (REG_HOLDING_START + REG_HOLDING_NREGS)))
	{
		iRegIndex = (int)(usAddress - usRegHoldingStart);
		switch(eMode)
		{                                       
			case MB_REG_READ://�� MB_REG_READ = 0
        while(usNRegs > 0)
				{
					*pucRegBuffer++ = (u8)(usRegHoldingBuf[iRegIndex] >> 8);            
					*pucRegBuffer++ = (u8)(usRegHoldingBuf[iRegIndex] & 0xFF); 
          iRegIndex++;
          usNRegs--;					
				}                            
        break;
			case MB_REG_WRITE://д MB_REG_WRITE = 0
				while(usNRegs > 0)
				{         
					usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
          usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
          iRegIndex++;
          usNRegs--;
        }				
			}
	}
	else//����
	{
		eStatus = MB_ENOREG;
	}	
	
	return eStatus;
}

/****************************************************************************
* ��	  �ƣ�eMBRegCoilsCB 
* ��    �ܣ���Ӧ�������У�01 ����Ȧ eMBFuncReadCoils
*													05 д��Ȧ eMBFuncWriteCoil
*													15 д�����Ȧ eMBFuncWriteMultipleCoils
* ��ڲ�����pucRegBuffer: ���ݻ�������������Ӧ����   
*						usAddress: ��Ȧ��ַ
*						usNCoils: Ҫ��д����Ȧ����
*						eMode: ������
* ���ڲ�����
* ע	  �⣺��̵��� 
*						0 ��
****************************************************************************/
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
	eMBErrorCode    eStatus = MB_ENOERR;
	int             iRegIndex;


	if((usAddress >= REG_HOLDING_START)&&\
		((usAddress+usNCoils) <= (REG_HOLDING_START + REG_HOLDING_NREGS)))
	{
		iRegIndex = (int)(usAddress - usRegHoldingStart);
		switch(eMode)
		{                                       
			case MB_REG_READ://�� MB_REG_READ = 0
        while(usNCoils > 0)
				{
// 					*pucRegBuffer++ = (u8)(usRegHoldingBuf[iRegIndex] >> 8);            
// 					*pucRegBuffer++ = (u8)(usRegHoldingBuf[iRegIndex] & 0xFF); 
          iRegIndex++;
          usNCoils--;					
				}                            
        break;
			case MB_REG_WRITE://д MB_REG_WRITE = 1
				while(usNCoils > 0)
				{         
// 					usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
//           usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
          iRegIndex++;
          usNCoils--;
        }				
			}
	}
	else//����
	{
		eStatus = MB_ENOREG;
	}	
	
	return eStatus;
}
/****************************************************************************
* ��	  �ƣ�eMBRegDiscreteCB 
* ��    �ܣ���ȡ��ɢ�Ĵ�������Ӧ�������У�02 ����ɢ�Ĵ��� eMBFuncReadDiscreteInputs
* ��ڲ�����pucRegBuffer: ���ݻ�������������Ӧ����   
*						usAddress: �Ĵ�����ַ
*						usNDiscrete: Ҫ��ȡ�ļĴ�������
* ���ڲ�����
* ע	  �⣺1 ��
****************************************************************************/
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    ( void )pucRegBuffer;
    ( void )usAddress;
    ( void )usNDiscrete;
    return MB_ENOREG;
}
