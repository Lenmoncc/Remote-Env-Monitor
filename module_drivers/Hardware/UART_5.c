#include "UART_5.h"



void UART5_Init()
{
	//SystemClock_Init();
	
	//ʹ������ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD, ENABLE);
	
	    
	//���Ÿ���ӳ��
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_UART5);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_UART5);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	    
	//����UART5 TX (PC12) ����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	//����UART5 RX (PD2) ����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	USART_OverSampling8Cmd(UART5, DISABLE);
	
	USART_InitTypeDef USART_InitStructure;
	
	//����UART5����
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(UART5, &USART_InitStructure);
	
	USART_Cmd(UART5, ENABLE);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(UART5, USART_IT_RXNE, ENABLE);
}

void UART5_SendChar(char c) {
    
    while(USART_GetFlagStatus(UART5, USART_FLAG_TXE) == RESET); // �ȴ��������ݼĴ���Ϊ��
    USART_SendData(UART5, c);
}

void UART5_SendString(char *str) {
    while (*str != '\0') {        
		UART5_SendChar(*str++);
    }
}

char UART5_ReceiveChar(void) {
    
    while(USART_GetFlagStatus(UART5, USART_FLAG_RXNE) == RESET); // �ȴ����յ�����
    return USART_ReceiveData(UART5);
}

volatile char uart5_rx_buf = 0;
volatile uint8_t uart5_rx_flag = 0;

void UART5_IRQHandler(void) {
    if(USART_GetITStatus(UART5, USART_IT_RXNE) != RESET) {
        uart5_rx_buf = USART_ReceiveData(UART5); // ��������
        uart5_rx_flag = 1; // ��λ��־
        //char received = USART_ReceiveData(UART5);
        //UART5_SendChar(received);// ���Խ��յ����ַ�
        USART_ClearITPendingBit(UART5, USART_IT_RXNE);// ����жϱ�־
    }
}

// int fputc(int ch, FILE *f)
// {
//     UART5_SendChar(ch);
//     return ch;
// }

void UART5_Printf(char *format, ...)
{
    char String[100];
    va_list arg;
    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);
    UART5_SendString(String);
}
