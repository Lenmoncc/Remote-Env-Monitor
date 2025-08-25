#include "BMP280.h"


BMP280_CalibTypeDef bmp280_calib;
int32_t t_fine;  

BMP280_ConfigTypeDef bmp_config = {
	  .osrs_t = 0x02,  // �¶�x2������
      .osrs_p = 0x05,  // ѹ��x16������
      .mode = 0x03,    // ����ģʽ
      .filter = 0x04,  // �˲���ϵ��16
      .t_sb = 0x01     // ����ʱ��62.5ms
};


/**
 * @brief  д��BMP280�Ĵ���
 * @param  reg���Ĵ�����ַ
 * @param  data��Ҫд�������
 */
void BMP280_WriteReg(uint8_t reg, uint8_t data) {
    MySPI_Start();                  
    MySPI_SwapByte(reg & 0x7F);     
    MySPI_SwapByte(data);           
    MySPI_Stop();                   
}

/**
 * @brief  ��ȡBMP280�Ĵ���
 * @param  reg���Ĵ�����ַ
 * @return ��ȡ������
 */
uint8_t BMP280_ReadReg(uint8_t reg) {
    uint8_t data;
    MySPI_Start();                  
    MySPI_SwapByte(reg | 0x80);     
    data = MySPI_SwapByte(0xFF);    
    MySPI_Stop();                   
    return data;
}

/**
 * @brief  ������ȡBMP280�Ĵ���
 * @param  reg����ʼ�Ĵ�����ַ
 * @param  buf�����ջ�����
 * @param  len����ȡ����
 */
void BMP280_ReadMultiReg(uint8_t reg, uint8_t *buf, uint8_t len) {
    MySPI_Start();
    MySPI_SwapByte(reg | 0x80);    
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = MySPI_SwapByte(0xFF);  
    }
    MySPI_Stop();
}

/**
 * @brief  ��λBMP280
 */
void BMP280_Reset(void) {
    BMP280_WriteReg(BMP280_REG_RESET, 0xB6); 
    Delay_ms(10);  
	//vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * @brief  ��ȡBMP280оƬID
 * @return IDֵ������Ϊ0x58��
 */
uint8_t BMP280_ReadID(void) {
    return BMP280_ReadReg(BMP280_REG_ID);
}

/**
 * @brief  ��ȡBMP280У׼����
 * @return 0���ɹ���1��ʧ��
 */
uint8_t BMP280_ReadCalib(void) {
    uint8_t calib_buf[24];
    BMP280_ReadMultiReg(BMP280_REG_CALIB, calib_buf, 24);  // ��ȡ24�ֽ�У׼����
    
    // ����У׼�������ߵ��ֽ�ƴ�ӣ�
    bmp280_calib.dig_T1 = (calib_buf[1] << 8) | calib_buf[0];
    bmp280_calib.dig_T2 = (calib_buf[3] << 8) | calib_buf[2];
    bmp280_calib.dig_T3 = (calib_buf[5] << 8) | calib_buf[4];
    
    bmp280_calib.dig_P1 = (calib_buf[7] << 8) | calib_buf[6];
    bmp280_calib.dig_P2 = (calib_buf[9] << 8) | calib_buf[8];
    bmp280_calib.dig_P3 = (calib_buf[11] << 8) | calib_buf[10];
    bmp280_calib.dig_P4 = (calib_buf[13] << 8) | calib_buf[12];
    bmp280_calib.dig_P5 = (calib_buf[15] << 8) | calib_buf[14];
    bmp280_calib.dig_P6 = (calib_buf[17] << 8) | calib_buf[16];
    bmp280_calib.dig_P7 = (calib_buf[19] << 8) | calib_buf[18];
    bmp280_calib.dig_P8 = (calib_buf[21] << 8) | calib_buf[20];
    bmp280_calib.dig_P9 = (calib_buf[23] << 8) | calib_buf[22];
    
    return 0;
}

/**
 * @brief  ��ʼ��BMP280
 * @param  config�����ò������������ʡ�ģʽ�ȣ�
 * @return 0���ɹ���1��ID����2��У׼������ȡʧ��
 */
uint8_t BMP280_Init(BMP280_ConfigTypeDef *config) {

    MySPI_Init();

    BMP280_Reset();

    if (BMP280_ReadID() != 0x58) {
        return 1;  // ID����
    }

    if (BMP280_ReadCalib() != 0) {
        return 2;  // У׼������ȡʧ��
    }
    
    //���ò������������ƼĴ���0xF4��
    // ��ʽ��[osrs_t(3bit) | osrs_p(3bit) | mode(2bit)]
    uint8_t ctrl_meas = (config->osrs_t << 5) | (config->osrs_p << 2) | config->mode;
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, ctrl_meas);
    
    // �����˲��ʹ���ʱ�䣨���üĴ���0xF5��
    // ��ʽ��[t_sb(3bit) | filter(3bit) | reserved(1bit) | spi3w_en(1bit)]
    uint8_t config_reg = (config->t_sb << 5) | (config->filter << 2) | 0x00;  // ����3��SPI
    BMP280_WriteReg(BMP280_REG_CONFIG, config_reg);
    
    UART5_Printf("Calib: T1=%u, T2=%d, T3=%d\r\n", 
                 bmp280_calib.dig_T1, bmp280_calib.dig_T2, bmp280_calib.dig_T3);
    UART5_Printf("Calib: P1=%u, P2=%d, P3=%d, P4=%d\r\n", 
                 bmp280_calib.dig_P1, bmp280_calib.dig_P2, 
                 bmp280_calib.dig_P3, bmp280_calib.dig_P4);

    return 0;  // ��ʼ���ɹ�
}

/**
 * @brief  �¶Ȳ�������ԭʼֵת��Ϊ�棩
 * @param  raw_temp���¶�ԭʼֵ��20λ��
 * @return ��������¶ȣ��棩
 */
float BMP280_CompensateTemp(int32_t raw_temp) {
    int32_t var1, var2;
    
    var1 = ((((raw_temp >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * 
           ((int32_t)bmp280_calib.dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1)) * 
           ((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * 
           ((int32_t)bmp280_calib.dig_T3)) >> 14;
    t_fine = var1 + var2;  // ����ѹ������
    return ((t_fine * 5 + 128) >> 8) / 100.0f;  // ת��Ϊ��
}


/**
 * @brief  ѹ����������ԭʼֵת��ΪPa��
 * @param  raw_press��ѹ��ԭʼֵ��20λ��
 * @return �������ѹ����Pa��
 */
float BMP280_CompensatePress(int32_t raw_press) {
    int64_t var1, var2, p;
    
    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * (int64_t)bmp280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)bmp280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)bmp280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)bmp280_calib.dig_P3) >> 8) + 
           ((var1 * (int64_t)bmp280_calib.dig_P2) << 12);
    var1 = ((((int64_t)1) << 47) + var1) * (int64_t)bmp280_calib.dig_P1 >> 33;
    
    if (var1 == 0) return 0;  // ����������
    
    p = 1048576 - raw_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)bmp280_calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)bmp280_calib.dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)bmp280_calib.dig_P7 << 4);
    
    return (float)p / 256.0f;  // Q24.8��ʽת��ΪPa
}

/**
 * @brief  ��ȡ���������ʪ������
 * @return �����¶Ⱥ�ѹ���Ľṹ��
 */
BMP280_DataTypedef BMP280_GetData(void) {
    const float stdlevel_pressure = 1013.25f;
    float altitude = 0.0f; 
    float ratio = 0.0f;
    BMP280_RawDataTypedef raw;
    uint8_t data[6];
    BMP280_DataTypedef res = {0,0};
    
    // �ȴ���������ɲ�����״̬�Ĵ���bit3Ϊ0ʱ���У�
    while ((BMP280_ReadReg(BMP280_REG_STATUS) & 0x08) != 0);
    
    // ��ȡѹ��ԭʼ���ݣ�0xF7-0xF9����MSB(8bit) + LSB(8bit) + XLSB(4bit)
    //BMP280_ReadMultiReg(BMP280_REG_PRESS_MSB, data, 6);
    BMP280_ReadMultiReg(BMP280_REG_PRESS_MSB, data, 3);
    raw.press = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    
    // ��ȡ�¶�ԭʼ���ݣ�0xFA-0xFC��
    BMP280_ReadMultiReg(BMP280_REG_TEMP_MSB, data, 3);
    raw.temp = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    
    // ��������
    res.temp = BMP280_CompensateTemp(raw.temp);
    //res.press = BMP280_CompensatePress(raw.press);
    float press_pa = BMP280_CompensatePress(raw.press);
    res.press = press_pa / 100.0f;  // Paת��ΪhPa
    
    // ���㺣�θ߶ȣ���λ���ף�
    ratio = stdlevel_pressure / res.press;  
    altitude = (powf(ratio, 0.190295f) - 1.0f) * 44330.77f;  
    
    UART5_Printf("Temp=%.2fC, Press=%.2fhPa\r\n", res.temp, res.press);
    UART5_Printf("Altitude=%.2fm\r\n", altitude);
    
    return res;
}