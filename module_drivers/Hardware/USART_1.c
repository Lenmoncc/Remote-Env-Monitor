#include "USART_1.h"


// 接收缓冲区及相关变量
static uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

// 初始化USART1
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 使能时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // 配置PA9(TX)和PA10(RX)为复用功能
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    // GPIO配置
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // USART配置
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStruct);

    // 使能中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 使能接收中断

    // NVIC配置
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // 使能USART1
    USART_Cmd(USART1, ENABLE);
}

// 发送一个字节
void USART1_SendChar(uint8_t ch)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, ch);
}

// 发送字符串
void USART1_SendString(char *str)
{
    while (*str) {
        USART1_SendChar(*str++);
    }
}

// 格式化输出（类似printf）
void USART1_Printf(const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    USART1_SendString(buffer);
}

// 从缓冲区读取一个字节
uint8_t USART1_ReadByte(void)
{
    uint8_t data = 0;
    
    if (rx_head != rx_tail) {
        data = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    }
    
    return data;
}

// 获取缓冲区中可用的字节数
uint16_t USART1_Available(void)
{
    return (rx_head >= rx_tail) ? (rx_head - rx_tail) : (RX_BUFFER_SIZE - rx_tail + rx_head);
}

// 清空接收缓冲区
void USART1_Flush(void)
{
    rx_head = 0;
    rx_tail = 0;
}

// USART1中断处理函数
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);

        // ===== 通知 Modbus 协议栈有新数据到达 =====
        extern BOOL rx_enabled;  // 在 portserial.c 定义
        if (rx_enabled) {
            pxMBFrameCBByteReceived();
        }

        // ===== 原有环形缓冲区逻辑 =====
        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = data;
            rx_head = next_head;
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }

    if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
        // ===== 通知 Modbus 协议栈可以发送下一个字节 =====
        extern BOOL tx_enabled;  // 在 portserial.c 定义
        if (tx_enabled) {
            pxMBFrameCBTransmitterEmpty();
        }

        USART_ClearITPendingBit(USART1, USART_IT_TXE);
    }

    if (USART_GetITStatus(USART1, USART_IT_ORE) != RESET) {
        USART_ReceiveData(USART1); // 清除 ORE 标志
        USART_ClearITPendingBit(USART1, USART_IT_ORE);
    }
}

