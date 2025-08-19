#include "sgp30.h"
#include "Delay.h"

uint16_t g_co2eq = 0;
uint16_t g_tvoc = 0;
uint8_t g_init_flag = 0;


void sgp30_data_show_init(void)
{
	// 初始化SGP30
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
            // 处理初始化阶段数据（前15秒）
            if (g_init_flag == 0) {
                HandleInitPhaseData(g_co2eq, g_tvoc);
            } else {
                // 打印有效数据（CO2eq范围400~60000ppm，TVOC范围0~60000ppb）
                UART5_Printf("CO2eq: %d ppm, TVOC: %d ppb\r\n", g_co2eq, g_tvoc);
            }
        } else {
            UART5_Printf("Data read failed\r\n");
        }
		
}


void HandleInitPhaseData(uint16_t co2, uint16_t tvoc) {
    static uint8_t init_count = 0;
    // 初始化阶段（15秒）后标志置位
    if (init_count >= 15) {
        g_init_flag = 1;
        UART5_Printf("SGP30 initialization completed\r\n");
    } else {
        init_count++;
        UART5_Printf("Initializing... %ds remaining\r\n", 15 - init_count);
    }
}


/**
 * @brief CRC8校验（符合SGP30要求）
 * @param data：待校验数据（2字节）
 * @return 计算得到的CRC值
 */
static uint8_t SGP30_CRC8(uint8_t *data) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < 2; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31; // 多项式0x31
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief 发送SGP30命令（16位）
 * @param cmd：命令码
 * @return 0-成功，1-失败
 */
static uint8_t SGP30_SendCmd(uint16_t cmd) {
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_W); // 发送写地址
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1; // 无应答
    }
    // 发送16位命令（MSB在前）
    MyIIC1_SendByte(cmd >> 8);    // 高8位
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(cmd & 0xFF);  // 低8位
    MyIIC1_Receive_Ack();
    MyIIC1_Stop();
    return 0;
}

/**
 * @brief 初始化SGP30
 * @return 0-成功，1-失败
 */
uint8_t SGP30_Init(void) {
    //IIC1_Init(); // 初始化软件I2C（GPIOB8/9）
    Delay_ms(10); // 上电稳定延时
    // 发送初始化命令，需等待最大10ms
    if (SGP30_SendCmd(SGP30_CMD_INIT) != 0) return 1;
    Delay_ms(10);
    return 0;
}

/**
 * @brief 读取TVOC和CO2eq数据
 * @param co2eq：输出CO2等效浓度（ppm）
 * @param tvoc：输出总挥发性有机物浓度（ppb）
 * @return 0-成功，1-失败（CRC校验错误）
 */
uint8_t SGP30_MeasureIAQ(uint16_t *co2eq, uint16_t *tvoc) {
    uint8_t data[6]; // 接收数据：CO2(2字节)+CRC(1)+TVOC(2字节)+CRC(1)
    
    // 发送测量命令，等待最大12ms
    if (SGP30_SendCmd(SGP30_CMD_MEASURE_IAQ) != 0) return 1;
    Delay_ms(12);
    
    // 读取6字节数据
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_R); // 发送读地址
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1;
    }
    // 读取CO2eq数据及CRC
    data[0] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0); // 继续接收
    data[1] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[2] = MyIIC1_ReceiveByte(); // CO2 CRC
    MyIIC1_Send_Ack(0);
    // 读取TVOC数据及CRC
    data[3] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[4] = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack(0);
    data[5] = MyIIC1_ReceiveByte(); // TVOC CRC
    MyIIC1_Send_Ack(1); // 停止接收
    MyIIC1_Stop();
    
    // CRC校验
    if (SGP30_CRC8(&data[0]) != data[2]) return 1; // CO2 CRC错误
    if (SGP30_CRC8(&data[3]) != data[5]) return 1; // TVOC CRC错误
    
    // 解析数据（MSB在前）
    *co2eq = (data[0] << 8) | data[1];
    *tvoc = (data[3] << 8) | data[4];
    return 0;
}

/**
 * @brief 设置绝对湿度补偿（提高测量精度）
 * @param abs_humidity：绝对湿度（g/m3，需转换为8.8固定点格式）
 * @return 0-成功，1-失败
 */
uint8_t SGP30_SetHumidity(uint32_t abs_humidity) {
    uint8_t data[3];
    // 转换为8.8固定点格式（例如11.8g/m3 → 0x04A0）
    uint16_t hum_data = (uint16_t)(abs_humidity * 256); 
    data[0] = hum_data >> 8;    // 高8位
    data[1] = hum_data & 0xFF;  // 低8位
    data[2] = SGP30_CRC8(data); // 计算CRC
    
    MyIIC1_Start();
    MyIIC1_SendByte(SGP30_ADDR_W);
    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return 1;
    }
    // 发送命令
    MyIIC1_SendByte(SGP30_CMD_SET_HUMIDITY >> 8);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(SGP30_CMD_SET_HUMIDITY & 0xFF);
    MyIIC1_Receive_Ack();
    // 发送湿度数据及CRC
    MyIIC1_SendByte(data[0]);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(data[1]);
    MyIIC1_Receive_Ack();
    MyIIC1_SendByte(data[2]);
    MyIIC1_Receive_Ack();
    MyIIC1_Stop();
    return 0;
}
