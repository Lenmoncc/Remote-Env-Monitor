#include "SPI_1.h"


void MySPI_W_SS(uint8_t BitValue)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);		
}


void MySPI_Init(void)
{
	/*����ʱ��*/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);	
	
	/*GPIO��ʼ��*/
	//SS
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);					//��PA4���ų�ʼ��Ϊ�������
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;//PA5~7���ù������	
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
  	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* ���Ÿ���ӳ�� */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);  // SCK
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);  // MISO
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);  // MOSI

	/*SPI��ʼ��*/
	SPI_InitTypeDef SPI_InitStructure;						
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;			
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;	
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;		
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;	
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;				//ģʽ0
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;			
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;				//NSS��ѡ�����������
	SPI_InitStructure.SPI_CRCPolynomial = 7;				
	SPI_Init(SPI1, &SPI_InitStructure);						

	SPI_Cmd(SPI1, ENABLE);									
	
	/*����Ĭ�ϵ�ƽ*/
	MySPI_W_SS(1);											
}


void MySPI_Start(void)
{
	MySPI_W_SS(0);				
}

void MySPI_Stop(void)
{
	MySPI_W_SS(1);				
}


uint8_t MySPI_SwapByte(uint8_t ByteSend)
{
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);	
	
	SPI_I2S_SendData(SPI1, ByteSend);								
	
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);	
	
	return SPI_I2S_ReceiveData(SPI1);								
}
