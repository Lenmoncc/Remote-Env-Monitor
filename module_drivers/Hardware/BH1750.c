#include "BH1750.h"

void BH1750_Init(void) {
    //IIC1_Init();               // 初始化IIC总线（已实现）
    BH1750_SendCmd(POWER_ON);  // 上电（退出掉电模式）
    Delay_ms(10);              // 等待稳定
}

void BH1750_SendCmd(uint8_t cmd) {
    MyIIC1_Start();                       // IIC起始信号
    MyIIC1_SendByte(BH1750_ADDR_L << 1);  // 发送从机地址（左移1位，最低位为写标志0）
    MyIIC1_Receive_Ack();                 // 等待从机应答
    MyIIC1_SendByte(cmd);                 // 发送指令
    MyIIC1_Receive_Ack();                 // 等待从机应答
    MyIIC1_Stop();                        // IIC终止信号
}

// 读取光照强度（返回值单位：lx）
float BH1750_ReadLight(void) {
    uint8_t data_high, data_low;
    uint16_t raw_data;
    float lux;


    BH1750_SendCmd(CONT_H_RES);
    Delay_ms(120);  

    // 读取测量数据（16位数据，分高低字节）
    MyIIC1_Start();                       
    MyIIC1_SendByte((BH1750_ADDR_L << 1) | 0x01);  
    MyIIC1_Receive_Ack();                 
    data_high = MyIIC1_ReceiveByte();     
    MyIIC1_Send_Ack(0);                   
    data_low = MyIIC1_ReceiveByte();      
    MyIIC1_Send_Ack(1);                   
    MyIIC1_Stop();                        

    // 计算光照强度（参考规格书第7章公式）
    raw_data = (data_high << 8) | data_low;  // 组合16位原始数据
    lux = raw_data / 1.2f;                   // 转换为实际光照值（lx）
	UART5_Printf("Light: %.1f lx\r\n", lux);
    return lux;
}

