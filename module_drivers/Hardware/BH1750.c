#include "BH1750.h"

void BH1750_Init(void) {
    //IIC1_Init();               // ��ʼ��IIC���ߣ���ʵ�֣�
    BH1750_SendCmd(POWER_ON);  // �ϵ磨�˳�����ģʽ��
    Delay_ms(10);              // �ȴ��ȶ�
}

void BH1750_SendCmd(uint8_t cmd) {
    MyIIC1_Start();                       // IIC��ʼ�ź�
    MyIIC1_SendByte(BH1750_ADDR_L << 1);  // ���ʹӻ���ַ������1λ�����λΪд��־0��
    MyIIC1_Receive_Ack();                 // �ȴ��ӻ�Ӧ��
    MyIIC1_SendByte(cmd);                 // ����ָ��
    MyIIC1_Receive_Ack();                 // �ȴ��ӻ�Ӧ��
    MyIIC1_Stop();                        // IIC��ֹ�ź�
}

// ��ȡ����ǿ�ȣ�����ֵ��λ��lx��
float BH1750_ReadLight(void) {
    uint8_t data_high, data_low;
    uint16_t raw_data;
    float lux;


    BH1750_SendCmd(CONT_H_RES);
    Delay_ms(120);  

    // ��ȡ�������ݣ�16λ���ݣ��ָߵ��ֽڣ�
    MyIIC1_Start();                       
    MyIIC1_SendByte((BH1750_ADDR_L << 1) | 0x01);  
    MyIIC1_Receive_Ack();                 
    data_high = MyIIC1_ReceiveByte();     
    MyIIC1_Send_Ack(0);                   
    data_low = MyIIC1_ReceiveByte();      
    MyIIC1_Send_Ack(1);                   
    MyIIC1_Stop();                        

    // �������ǿ�ȣ��ο�������7�¹�ʽ��
    raw_data = (data_high << 8) | data_low;  // ���16λԭʼ����
    lux = raw_data / 1.2f;                   // ת��Ϊʵ�ʹ���ֵ��lx��
	UART5_Printf("Light: %.1f lx\r\n", lux);
    return lux;
}

