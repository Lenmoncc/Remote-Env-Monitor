#include "AHT10.h"
#include "IIC_1.h"
#include "Delay.h"  
#include "UART_5.h"

AHT10_Data data;

uint8_t aht10_init(void) {
  //vTaskDelay(pdMS_TO_TICKS(40));
	Delay_ms(40);
  //IIC1_Init();
  
  MyIIC1_Start();
  MyIIC1_SendByte(AHT10_CMD_WRITE);
  if(!(MyIIC1_Receive_Ack())) goto error; 
  
  MyIIC1_SendByte(AHT10_CMD_INIT);
  if(!(MyIIC1_Receive_Ack())) goto error;
  
  MyIIC1_SendByte(AHT10_INIT_PRAM1);
  if(!(MyIIC1_Receive_Ack())) goto error;
  
  MyIIC1_SendByte(AHT10_INIT_PRAM2);
  if(!(MyIIC1_Receive_Ack())) goto error;
  
  MyIIC1_Stop();
  UART5_Printf("AHT10 init success!\r\n"); 
  return 0;
  
error:
  MyIIC1_Stop();
  UART5_Printf("AHT10 init failed!\r\n");
  return 1; // ��ʼ��ʧ��
}

void aht10_soft_reset(void)
{
    MyIIC1_Start();
    MyIIC1_SendByte(AHT10_CMD_WRITE); 
    MyIIC1_Receive_Ack(); 
    MyIIC1_SendByte(AHT10_CMD_SOFTRESET); 
    MyIIC1_Receive_Ack(); 
    MyIIC1_Stop();
    
    Delay_ms(20); // �ȴ���λ���
}

uint8_t aht10_get_data(AHT10_Data* data)
{
    uint8_t raw_data[6];
    
    // ��������
    MyIIC1_Start();
    MyIIC1_SendByte(AHT10_CMD_WRITE); 
    if (MyIIC1_Receive_Ack() != 0) goto error;  

    MyIIC1_SendByte(AHT10_CMD_TRIGGER); 
    if (MyIIC1_Receive_Ack() != 0) goto error; 

    MyIIC1_SendByte(AHT10_TRIGGER_PRAM1); 
    if (MyIIC1_Receive_Ack() != 0) goto error;  

    MyIIC1_SendByte(AHT10_TRIGGER_PRAM2); 
    if (MyIIC1_Receive_Ack() != 0) goto error;  

    MyIIC1_Stop();
    Delay_ms(80); // �ȴ��������

    // ��ȡ����
    MyIIC1_Start();
    MyIIC1_SendByte(AHT10_CMD_READ); 
    MyIIC1_Receive_Ack(); 
    raw_data[0] = MyIIC1_ReceiveByte(); 
    MyIIC1_Send_Ack(1); // ����ACKλ��׼����

    if (raw_data[0] & 0x80)
    {
        UART5_Printf("AHT10 status error! Status: 0x%02X\r\n", raw_data[0]);
        MyIIC1_Stop();
        return 1; // ״̬����
    }

    for (int i = 1; i < 6; i++) {
        raw_data[i] = MyIIC1_ReceiveByte(); // ��ȡ6���ֽ�����
        if (i < 5) {
            MyIIC1_Send_Ack(1); // ����ACKλ�������һ���ֽ���
        } else {
            MyIIC1_Send_Ack(0); // ���һ���ֽڷ���NACKλ
        }
    }
    MyIIC1_Stop();
    Delay_us(2); // ��ʱ��ȷ�������ȶ�

    // ��ӡԭʼ����
    UART5_Printf("Raw data: ");
    for (int i = 0; i < 6; i++) {
        UART5_Printf("%02X ", raw_data[i]);
    }
    UART5_Printf("\r\n");

    // ����ʪ�ȣ�20λ���ݣ�
    uint32_t hum_raw = ((uint32_t)raw_data[1] << 12) | 
                       ((uint32_t)raw_data[2] << 4) | 
                       ((uint32_t)raw_data[3] >> 4);
    data->humidity = (int)((hum_raw / 1048576.0f) * 100.0f);
    //printf("humidity: %d\r\n", (int)data->humidity);
    
    // �����¶ȣ�20λ���ݣ�
    uint32_t temp_raw = ((((uint32_t)raw_data[3]) & 0x0F) << 16) |  
                    ((uint32_t)raw_data[4] << 8) | 
                    ((uint32_t)raw_data[5]);
    data->temperature = (int)((temp_raw / 1048576.0f) * 200.0f - 50.0f);
    //printf("temperature: %d\r\n", (int)data->temperature);
    
    return 0; // �ɹ�

error:
    MyIIC1_Stop();
    UART5_Printf("AHT10 trigger failed!\r\n");
    return 1;
}