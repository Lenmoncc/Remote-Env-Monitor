#include "BMP280.h"


BMP280_CalibTypeDef bmp280_calib;
int32_t t_fine;  

BMP280_ConfigTypeDef bmp_config = {
	  .osrs_t = 0x02,  // 温度x2过采样
      .osrs_p = 0x05,  // 压力x16过采样
      .mode = 0x03,    // 正常模式
      .filter = 0x04,  // 滤波器系数16
      .t_sb = 0x01     // 待机时间62.5ms
};


/**
 * @brief  写入BMP280寄存器
 * @param  reg：寄存器地址
 * @param  data：要写入的数据
 */
void BMP280_WriteReg(uint8_t reg, uint8_t data) {
    MySPI_Start();                  
    MySPI_SwapByte(reg & 0x7F);     
    MySPI_SwapByte(data);           
    MySPI_Stop();                   
}

/**
 * @brief  读取BMP280寄存器
 * @param  reg：寄存器地址
 * @return 读取的数据
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
 * @brief  连续读取BMP280寄存器
 * @param  reg：起始寄存器地址
 * @param  buf：接收缓冲区
 * @param  len：读取长度
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
 * @brief  复位BMP280
 */
void BMP280_Reset(void) {
    BMP280_WriteReg(BMP280_REG_RESET, 0xB6); 
    Delay_ms(10);  
	//vTaskDelay(pdMS_TO_TICKS(10));
}

/**
 * @brief  读取BMP280芯片ID
 * @return ID值（正常为0x58）
 */
uint8_t BMP280_ReadID(void) {
    return BMP280_ReadReg(BMP280_REG_ID);
}

/**
 * @brief  读取BMP280校准参数
 * @return 0：成功；1：失败
 */
uint8_t BMP280_ReadCalib(void) {
    uint8_t calib_buf[24];
    BMP280_ReadMultiReg(BMP280_REG_CALIB, calib_buf, 24);  // 读取24字节校准数据
    
    // 解析校准参数（高低字节拼接）
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
 * @brief  初始化BMP280
 * @param  config：配置参数（过采样率、模式等）
 * @return 0：成功；1：ID错误；2：校准参数读取失败
 */
uint8_t BMP280_Init(BMP280_ConfigTypeDef *config) {

    MySPI_Init();

    BMP280_Reset();

    if (BMP280_ReadID() != 0x58) {
        return 1;  // ID错误
    }

    if (BMP280_ReadCalib() != 0) {
        return 2;  // 校准参数读取失败
    }
    
    //配置测量参数（控制寄存器0xF4）
    // 格式：[osrs_t(3bit) | osrs_p(3bit) | mode(2bit)]
    uint8_t ctrl_meas = (config->osrs_t << 5) | (config->osrs_p << 2) | config->mode;
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, ctrl_meas);
    
    // 配置滤波和待机时间（配置寄存器0xF5）
    // 格式：[t_sb(3bit) | filter(3bit) | reserved(1bit) | spi3w_en(1bit)]
    uint8_t config_reg = (config->t_sb << 5) | (config->filter << 2) | 0x00;  // 禁用3线SPI
    BMP280_WriteReg(BMP280_REG_CONFIG, config_reg);
    
    UART5_Printf("Calib: T1=%u, T2=%d, T3=%d\r\n", 
                 bmp280_calib.dig_T1, bmp280_calib.dig_T2, bmp280_calib.dig_T3);
    UART5_Printf("Calib: P1=%u, P2=%d, P3=%d, P4=%d\r\n", 
                 bmp280_calib.dig_P1, bmp280_calib.dig_P2, 
                 bmp280_calib.dig_P3, bmp280_calib.dig_P4);

    return 0;  // 初始化成功
}

/**
 * @brief  温度补偿（将原始值转换为℃）
 * @param  raw_temp：温度原始值（20位）
 * @return 补偿后的温度（℃）
 */
float BMP280_CompensateTemp(int32_t raw_temp) {
    int32_t var1, var2;
    
    var1 = ((((raw_temp >> 3) - ((int32_t)bmp280_calib.dig_T1 << 1))) * 
           ((int32_t)bmp280_calib.dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1)) * 
           ((raw_temp >> 4) - ((int32_t)bmp280_calib.dig_T1))) >> 12) * 
           ((int32_t)bmp280_calib.dig_T3)) >> 14;
    t_fine = var1 + var2;  // 用于压力补偿
    return ((t_fine * 5 + 128) >> 8) / 100.0f;  // 转换为℃
}


/**
 * @brief  压力补偿（将原始值转换为Pa）
 * @param  raw_press：压力原始值（20位）
 * @return 补偿后的压力（Pa）
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
    
    if (var1 == 0) return 0;  // 避免除零错误
    
    p = 1048576 - raw_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)bmp280_calib.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)bmp280_calib.dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)bmp280_calib.dig_P7 << 4);
    
    return (float)p / 256.0f;  // Q24.8格式转换为Pa
}

/**
 * @brief  获取补偿后的温湿度数据
 * @return 包含温度和压力的结构体
 */
BMP280_DataTypedef BMP280_GetData(void) {
    const float stdlevel_pressure = 1013.25f;
    float altitude = 0.0f; 
    float ratio = 0.0f;
    BMP280_RawDataTypedef raw;
    uint8_t data[6];
    BMP280_DataTypedef res = {0,0};
    
    // 等待传感器完成测量（状态寄存器bit3为0时空闲）
    while ((BMP280_ReadReg(BMP280_REG_STATUS) & 0x08) != 0);
    
    // 读取压力原始数据（0xF7-0xF9）：MSB(8bit) + LSB(8bit) + XLSB(4bit)
    //BMP280_ReadMultiReg(BMP280_REG_PRESS_MSB, data, 6);
    BMP280_ReadMultiReg(BMP280_REG_PRESS_MSB, data, 3);
    raw.press = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    
    // 读取温度原始数据（0xFA-0xFC）
    BMP280_ReadMultiReg(BMP280_REG_TEMP_MSB, data, 3);
    raw.temp = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    
    // 补偿计算
    res.temp = BMP280_CompensateTemp(raw.temp);
    //res.press = BMP280_CompensatePress(raw.press);
    float press_pa = BMP280_CompensatePress(raw.press);
    res.press = press_pa / 100.0f;  // Pa转换为hPa
    
    // 计算海拔高度（单位：米）
    ratio = stdlevel_pressure / res.press;  
    altitude = (powf(ratio, 0.190295f) - 1.0f) * 44330.77f;  
    
    UART5_Printf("Temp=%.2fC, Press=%.2fhPa\r\n", res.temp, res.press);
    UART5_Printf("Altitude=%.2fm\r\n", altitude);
    
    return res;
}