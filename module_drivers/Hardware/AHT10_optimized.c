#include "AHT10_optimized.h"
#include "AHT10.h"
#include "IIC_1.h"
#include "Delay.h"
#include "UART_5.h"

/* FreeRTOS support */
#include "FreeRTOS.h"
#include "task.h"

/*=================================================================================
 * 私有常量定义
 =================================================================================*/

#define AHT10_ADDR              0x70        /* I2C 地址 */
#define AHT10_MAX_RETRIES       3           /* 最大重试次数 */
#define AHT10_READ_TIMEOUT_MS   100         /* 读取超时（毫秒） */
#define AHT10_FILTER_SIZE       5           /* 滤波窗口大小 */

/* AHT10 命令定义 */
#define AHT10_CMD_TRIGGER       0xAC        /* 触发测量命令 */
#define AHT10_CMD_STATUS        0x71        /* 获取状态命令 */
#define AHT10_CMD_SOFT_RESET    0xBA        /* 软复位命令 */

/*=================================================================================
 * 私有函数
 =================================================================================*/

/**
 * @brief  计算校验和
 * @param  data：数据指针
 * @param  len：数据长度
 * @return 校验和
 */
static uint8_t AHT10_CalculateChecksum(uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

/**
 * @brief  验证校验和
 * @param  data：数据指针
 * @param  len：数据长度（包含校验和）
 * @return 1 正确，0 错误
 */
static uint8_t AHT10_VerifyChecksum(uint8_t *data, uint8_t len)
{
    if (len < 2) {
        return 0;
    }

    uint8_t calc_crc = AHT10_CalculateChecksum(data, len - 1);
    return (calc_crc == data[len - 1]) ? 1 : 0;
}

/**
 * @brief  等待测量完成（非阻塞方式）
 * @param  timeout_ms：超时时间（毫秒）
 * @return 1 完成，0 超时
 */
static uint8_t AHT10_WaitMeasurement(uint32_t timeout_ms)
{
    uint32_t start_tick = xTaskGetTickCount();
    uint8_t status;

    while (1) {
        /* 检查调度器状态 */
        if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
            /* 使用 FreeRTOS 延时 */
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            /* 调度器未启动，使用轮询 */
            Delay_ms(10);
        }

        /* 读取状态字节 */
        MyIIC1_Start();
        MyIIC1_SendByte(AHT10_ADDR);
        if (MyIIC1_Receive_Ack() != 0) {
            MyIIC1_Stop();
            continue;  /* ACK 失败，重试 */
        }

        /* 接收状态 */
        status = MyIIC1_ReceiveByte();
        MyIIC1_Stop();

        /* 检查测量完成标志（bit7） */
        if ((status & 0x80) == 0) {
            return 1;  /* 完成 */
        }

        /* 超时检查 */
        if ((xTaskGetTickCount() - start_tick) >= pdMS_TO_TICKS(timeout_ms)) {
            return 0;  /* 超时 */
        }
    }
}

/**
 * @brief  解析温度数据
 * @param  raw_data：原始数据（3字节）
 * @return 温度值（℃）
 */
static float AHT10_ParseTemperature(uint8_t *raw_data)
{
    uint32_t temp_raw = ((uint32_t)raw_data[3] << 12) |
                        ((uint32_t)raw_data[4] << 4) |
                        ((uint32_t)raw_data[5] >> 4);

    return (float)((temp_raw / 1048576.0f) * 200.0f - 50.0f);
}

/**
 * @brief  解析湿度数据
 * @param  raw_data：原始数据（3字节）
 * @return 相对湿度（%RH）
 */
static float AHT10_ParseHumidity(uint8_t *raw_data)
{
    uint32_t hum_raw = ((uint32_t)raw_data[1] << 12) |
                       ((uint32_t)raw_data[2] << 4) |
                       ((uint32_t)raw_data[3] >> 4);

    return (float)((hum_raw / 1048576.0f) * 100.0f);
}

/*=================================================================================
 * 公开函数实现
 =================================================================================*/

AHT10_Status_t AHT10_Init_Optimized(AHT10_Manager_t *manager, uint8_t enable_filter)
{
    if (manager == NULL) {
        return AHT10_INIT_FAILED;
    }

    /* 初始化管理结构 */
    manager->use_filter = enable_filter;
    manager->error_count = 0;
    manager->success_count = 0;
    manager->last_read_time = 0;

    /* 初始化数据 */
    manager->data.temperature = 0.0f;
    manager->data.humidity = 0.0f;
    manager->data.timestamp = xTaskGetTickCount();

    /* 初始化滤波器（如果启用） */
    if (enable_filter) {
        if (MovingAverage_Init(&manager->temp_filter, AHT10_FILTER_SIZE) != 0) {
            UART5_SendString("[AHT10] Temperature filter init failed\r\n");
            return AHT10_INIT_FAILED;
        }

        if (MovingAverage_Init(&manager->hum_filter, AHT10_FILTER_SIZE) != 0) {
            UART5_SendString("[AHT10] Humidity filter init failed\r\n");
            return AHT10_INIT_FAILED;
        }

        UART5_SendString("[AHT10] Filters enabled\r\n");
    }

    /* 软复位 */
    MyIIC1_Start();
    MyIIC1_SendByte(AHT10_ADDR);
    MyIIC1_SendByte(AHT10_CMD_SOFT_RESET);
    MyIIC1_Stop();

    /* 等待复位完成 */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        vTaskDelay(pdMS_TO_TICKS(20));
    } else {
        Delay_ms(20);
    }

    UART5_SendString("[AHT10] Initialized successfully\r\n");
    return AHT10_OK;
}

AHT10_Status_t AHT10_ReadData_Optimized(AHT10_Manager_t *manager, AHT10_Data_t *data)
{
    if (manager == NULL || data == NULL) {
        return AHT10_I2C_ERR;
    }

    uint8_t raw_data[7];  /* 6字节数据 + 1字节校验和 */
    AHT10_Status_t status;

    /* 3次重试循环 */
    for (uint8_t retry = 0; retry < AHT10_MAX_RETRIES; retry++) {
        /* 1. 触发测量 */
        MyIIC1_Start();
        MyIIC1_SendByte(AHT10_ADDR);

        if (MyIIC1_Receive_Ack() != 0) {
            MyIIC1_Stop();
            continue;  /* I2C 错误，重试 */
        }

        MyIIC1_SendByte(AHT10_CMD_TRIGGER);
        MyIIC1_SendByte(0x33);  /* 参数1 */
        MyIIC1_SendByte(0x00);  /* 参数2 */
        MyIIC1_Stop();

        /* 2. 等待测量完成 */
        if (AHT10_WaitMeasurement(AHT10_READ_TIMEOUT_MS) == 0) {
            status = AHT10_TIMEOUT;
            continue;  /* 超时，重试 */
        }

        /* 3. 读取数据 */
        MyIIC1_Start();
        MyIIC1_SendByte(AHT10_ADDR | 0x01);  /* 读地址 */

        if (MyIIC1_Receive_Ack() != 0) {
            MyIIC1_Stop();
            continue;  /* I2C 错误，重试 */
        }

        /* 接收6字节数据 */
        for (uint8_t i = 0; i < 6; i++) {
            raw_data[i] = MyIIC1_ReceiveByte();
            if (i < 5) {
                MyIIC1_Send_Ack();
            } else {
                MyIIC1_Send_Nack();
            }
        }

        /* 计算并追加校验和 */
        raw_data[6] = AHT10_CalculateChecksum(raw_data, 6);

        MyIIC1_Stop();

        /* 4. 验证校验和 */
        if (AHT10_VerifyChecksum(raw_data, 7) == 0) {
            UART5_Printf("[AHT10] CRC error on retry %d\r\n", retry + 1);
            status = AHT10_CRC_ERR;
            continue;  /* 校验失败，重试 */
        }

        /* 5. 解析数据 */
        float temp = AHT10_ParseTemperature(raw_data);
        float hum = AHT10_ParseHumidity(raw_data);

        /* 6. 应用滤波（如果启用） */
        if (manager->use_filter) {
            temp = MovingAverage_Update(&manager->temp_filter, temp);
            hum = MovingAverage_Update(&manager->hum_filter, hum);
        }

        /* 7. 更新管理结构 */
        manager->data.temperature = temp;
        manager->data.humidity = hum;
        manager->data.timestamp = xTaskGetTickCount();
        manager->success_count++;
        manager->last_read_time = manager->data.timestamp;

        /* 8. 输出数据 */
        *data = manager->data;

        UART5_Printf("[AHT10] T=%.2f℃ H=%.2f%%RH (retry %d)\r\n", temp, hum, retry + 1);
        return AHT10_OK;
    }

    /* 3次都失败 */
    manager->error_count++;

    UART5_Printf("[AHT10] Read failed after %d retries\r\n", AHT10_MAX_RETRIES);
    return status;
}

AHT10_Data_t AHT10_GetFilteredData(AHT10_Manager_t *manager)
{
    if (manager == NULL) {
        AHT10_Data_t empty = {0, 0, 0};
        return empty;
    }

    return manager->data;
}

uint8_t AHT10_GetStatistics(AHT10_Manager_t *manager, uint16_t *success_count, uint16_t *error_count)
{
    if (manager == NULL) {
        return 0;
    }

    if (success_count != NULL) {
        *success_count = manager->success_count;
    }

    if (error_count != NULL) {
        *error_count = manager->error_count;
    }

    uint32_t total = manager->success_count + manager->error_count;
    if (total == 0) {
        return 0;
    }

    return (uint8_t)((manager->success_count * 100) / total);
}

void AHT10_Reset(AHT10_Manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    manager->error_count = 0;
    manager->success_count = 0;

    if (manager->use_filter) {
        MovingAverage_Reset(&manager->temp_filter);
        MovingAverage_Reset(&manager->hum_filter);
    }
}
