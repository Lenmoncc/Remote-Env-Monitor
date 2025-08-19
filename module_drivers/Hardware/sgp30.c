#include "sgp30.h"
#include "Delay.h"

uint16_t g_co2eq = 0;
uint16_t g_tvoc = 0;
uint8_t g_init_flag = 0;


void sgp30_data_show_init(void)
{
	// ��ʼ��SGP30
    if (SGP30_Init() != 0) {
        UART5_Printf("SGP30 init failed!\r\n");
        return ;
    }
    printf("SGP30 init success, entering main loop\r\n");
	
	if (SGP30_SetHumidity(12) != 0) {
        UART5_Printf("Humidity compensation set failed\r\n");
    } else {
        UART5_Printf("Humidity compensation set: 11.8g/m3\r\n");
    }
}

void sgp30_data_show(void)
{
	
	if (SGP30_MeasureIAQ(&g_co2eq, &g_tvoc) == 0) {
            // �����ʼ���׶����ݣ�ǰ15�룩
            if (g_init_flag == 0) {
                HandleInitPhaseData(g_co2eq, g_tvoc);
            } else {
                // ��ӡ��Ч���ݣ�CO2eq��Χ400~60000ppm��TVOC��Χ0~60000ppb��
                UART5_Printf("CO2eq: %d ppm, TVOC: %d ppb\r\n", g_co2eq, g_tvoc);
            }
        } else {
            UART5_Printf("Data read failed\r\n");
        }
		
}


void HandleInitPhaseData(uint16_t co2, uint16_t tvoc) {
    static uint8_t init_count = 0;
    // ��ʼ���׶Σ�15�룩���־��λ
    if (init_count >= 15) {
        g_init_flag = 1;
        UART5_Printf("SGP30 initialization completed\r\n");
    } else {
        init_count++;
        UART5_Printf("Initializing... %ds remaining\r\n", 15 - init_count);
    }
}


/**
 * @brief CRC8У�飨����SGP30Ҫ��
 * @param data����У�����ݣ�2�ֽڣ�
 * @return ����õ���CRCֵ
 */
static uint8_t SGP30_CRC8(uint8_t *data) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < 2; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31; // ����ʽ0x31
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief ����SGP30���16λ��
 * @param cmd��������
 * @return 0-�ɹ���1-ʧ��
 */
static uint8_t SGP30_SendCmd(uint16_t cmd) {
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_W); // ����д��ַ
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1; // ��Ӧ��
    }
    // ����16λ���MSB��ǰ��
    MyIIC1_SendByte(cmd >> 8);    // ��8λ
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(cmd & 0xFF);  // ��8λ
    MyIIC1_Receive_Ack();
    MyIIC1_Stop();
    return 0;
}

/**
 * @brief ��ʼ��SGP30
 * @return 0-�ɹ���1-ʧ��
 */
uint8_t SGP30_Init(void) {
    //IIC1_Init(); // ��ʼ�����I2C��GPIOB8/9��
    Delay_ms(10); // �ϵ��ȶ���ʱ
    // ���ͳ�ʼ�������ȴ����10ms
    if (SGP30_SendCmd(SGP30_CMD_INIT) != 0) return 1;
    Delay_ms(10);
    return 0;
}

/**
 * @brief ��ȡTVOC��CO2eq����
 * @param co2eq�����CO2��ЧŨ�ȣ�ppm��
 * @param tvoc������ܻӷ����л���Ũ�ȣ�ppb��
 * @return 0-�ɹ���1-ʧ�ܣ�CRCУ�����
 */
uint8_t SGP30_MeasureIAQ(uint16_t *co2eq, uint16_t *tvoc) {
    uint8_t data[6]; // �������ݣ�CO2(2�ֽ�)+CRC(1)+TVOC(2�ֽ�)+CRC(1)
    
    // ���Ͳ�������ȴ����12ms
    if (SGP30_SendCmd(SGP30_CMD_MEASURE_IAQ) != 0) return 1;
    Delay_ms(12);
    
    // ��ȡ6�ֽ�����
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_R); // ���Ͷ���ַ
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1;
    }
    // ��ȡCO2eq���ݼ�CRC
    data[0] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0); // ��������
    data[1] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[2] = MyIIC1_ReceiveByte(); // CO2 CRC
    MyIIC1_Send_Ack(0);
    // ��ȡTVOC���ݼ�CRC
    data[3] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[4] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[5] = MyIIC1_ReceiveByte(); // TVOC CRC
    MyIIC1_Send_Ack(1); // ֹͣ����
    MyIIC1_Stop();
    
    // CRCУ��
    if (SGP30_CRC8(&data[0]) != data[2]) return 1; // CO2 CRC����
    if (SGP30_CRC8(&data[3]) != data[5]) return 1; // TVOC CRC����
    
    // �������ݣ�MSB��ǰ��
    *co2eq = (data[0] << 8) | data[1];
    *tvoc = (data[3] << 8) | data[4];
    return 0;
}

/**
 * @brief ���þ���ʪ�Ȳ�������߲������ȣ�
 * @param abs_humidity������ʪ�ȣ�g/m3����ת��Ϊ8.8�̶����ʽ��
 * @return 0-�ɹ���1-ʧ��
 */
uint8_t SGP30_SetHumidity(uint32_t abs_humidity) {
    uint8_t data[3];
    // ת��Ϊ8.8�̶����ʽ������11.8g/m3 �� 0x04A0��
    uint16_t hum_data = (uint16_t)(abs_humidity * 256); 
    data[0] = hum_data >> 8;    // ��8λ
    data[1] = hum_data & 0xFF;  // ��8λ
    data[2] = SGP30_CRC8(data); // ����CRC
    
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_W);
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1;
    }
    // ��������
    MyIIC1_SendByte(SGP30_CMD_SET_HUMIDITY >> 8);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(SGP30_CMD_SET_HUMIDITY & 0xFF);
    MyIIC1_Receive_Ack();
    // ����ʪ�����ݼ�CRC
    MyIIC1_SendByte(data[0]);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(data[1]);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(data[2]);
    MyIIC1_Receive_Ack();
    MyIIC1_Stop();
    return 0;
}
