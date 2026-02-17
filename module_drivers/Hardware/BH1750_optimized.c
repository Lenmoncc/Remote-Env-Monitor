#include "BH1750_optimized.h"
#include "BH1750.h"
#include "IIC_1.h"
#include "Delay.h"
#include "UART_5.h"

/* FreeRTOS support */
#include "FreeRTOS.h"
#include "task.h"

/*=================================================================================
 * 私有常量定义
 =================================================================================*/

#define BH1750_ADDR             0x23        /* I2C 地址 */
#define BH1750_MIN_LUX          5           /* 光照下限（lux）*/
#define BH1750_MAX_LUX_LOW      60000       /* 低分辨率模式上限 */
#define BH1750_MAX_LUX_HIGH     10000       /* 高分辨率模式上限 */
#define BH1750_UNDERFLOW_TH     10          /* 欠饱和阈值 */
#define BH1750_OVERFLOW_TH      50000       /* 过饱和阈值 */
#define BH1750_FILTER_SIZE      3           /* 中值滤波窗口 */
#define BH1750_ADJUSTMENT_INTERVAL 20       /* 增益调整间隔（采样次数） */

/*=================================================================================
 * 私有函数
 =================================================================================*/

/**
 * @brief  发送命令到 BH1750
 * @param  cmd：命令字节
 * @return 0 成功，-1 失败
 */
static int32_t BH1750_SendCommand(uint8_t cmd)
{
    MyIIC1_Start();
    MyIIC1_SendByte(BH1750_ADDR);

    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return -1;
    }

    MyIIC1_SendByte(cmd);
    MyIIC1_Send_Ack();
    MyIIC1_Stop();

    return 0;
}

/**
 * @brief  接收数据（2字节）
 * @param  high：输出高字节
 * @param  low：输出低字节
 * @return 0 成功，-1 失败
 */
static int32_t BH1750_ReceiveData(uint8_t *high, uint8_t *low)
{
    MyIIC1_Start();
    MyIIC1_SendByte(BH1750_ADDR | 0x01);  /* 读地址 */

    if (MyIIC1_Receive_Ack() != 0) {
        MyIIC1_Stop();
        return -1;
    }

    *high = MyIIC1_ReceiveByte();
    MyIIC1_Send_Ack();

    *low = MyIIC1_ReceiveByte();
    MyIIC1_Send_Nack();

    MyIIC1_Stop();

    return 0;
}

/**
 * @brief  计算光照值（整数化，避免浮点误差）
 * @param  high：高字节
 * @param  low：低字节
 * @return 光照值（lux）
 */
static uint16_t BH1750_CalculateLux(uint8_t high, uint8_t low)
{
    uint16_t raw = (high << 8) | low;

    /* 定点化计算：lux = raw / 1.2
     * 为避免浮点，使用：lux = (raw * 5 + 6) / 12
     * 这是 raw * (5/12) 的定点近似
     */
    return (uint16_t)((raw * 5 + 6) / 12);
}

/*=================================================================================
 * 公开函数实现
 =================================================================================*/

BH1750_Status_t BH1750_Init_Optimized(BH1750_Manager_t *manager,
                                      uint8_t enable_filter,
                                      uint8_t enable_auto_gain)
{
    if (manager == NULL) {
        return BH1750_INIT_FAILED;
    }

    /* 初始化管理结构 */
    manager->use_filter = enable_filter;
    manager->auto_gain_enabled = enable_auto_gain;
    manager->adjustment_counter = 0;
    manager->error_count = 0;
    manager->success_count = 0;
    manager->saturation_count = 0;

    /* 初始化数据 */
    manager->data.lux = 0;
    manager->data.mode = BH1750_RESOLUTION_HIGH;
    manager->data.is_saturated = 0;
    manager->data.timestamp = xTaskGetTickCount();

    /* 初始化滤波器（如果启用） */
    if (enable_filter) {
        if (MedianFilter_Init(&manager->filter, BH1750_FILTER_SIZE) != 0) {
            UART5_SendString("[BH1750] Filter init failed\r\n");
            return BH1750_INIT_FAILED;
        }
        UART5_SendString("[BH1750] Median filter enabled\r\n");
    }

    /* 初始化传感器 */
    if (BH1750_SendCommand(BH1750_RESOLUTION_HIGH) != 0) {
        UART5_SendString("[BH1750] Init failed\r\n");
        return BH1750_INIT_FAILED;
    }

    if (enable_auto_gain) {
        UART5_SendString("[BH1750] Auto-gain enabled\r\n");
    }

    UART5_SendString("[BH1750] Initialized successfully\r\n");
    return BH1750_OK;
}

BH1750_Status_t BH1750_ReadData_Optimized(BH1750_Manager_t *manager, BH1750_Data_t *data)
{
    if (manager == NULL || data == NULL) {
        return BH1750_COMM_ERR;
    }

    uint8_t high, low;

    /* 1. 发送测量命令 */
    if (BH1750_SendCommand(manager->data.mode) != 0) {
        manager->error_count++;
        return BH1750_COMM_ERR;
    }

    /* 2. 等待测量完成（120ms） */
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        vTaskDelay(pdMS_TO_TICKS(120));
    } else {
        Delay_ms(120);
    }

    /* 3. 接收数据 */
    if (BH1750_ReceiveData(&high, &low) != 0) {
        manager->error_count++;
        return BH1750_COMM_ERR;
    }

    /* 4. 计算光照值 */
    uint16_t raw_lux = BH1750_CalculateLux(high, low);

    /* 5. 范围检查 */
    manager->data.is_saturated = 0;

    if (raw_lux < BH1750_MIN_LUX) {
        /* 光照太弱 */
        manager->data.lux = 0;
        manager->data.is_saturated = 1;
        manager->saturation_count++;
        UART5_SendString("[BH1750] Underflow detected\r\n");
        return BH1750_UNDERFLOW;
    }

    if (raw_lux > BH1750_OVERFLOW_TH) {
        /* 光照太强，可能饱和 */
        manager->data.lux = raw_lux;
        manager->data.is_saturated = 1;
        manager->saturation_count++;
        UART5_SendString("[BH1750] Overflow detected\r\n");
        return BH1750_OVERFLOW;
    }

    /* 6. 应用滤波（如果启用） */
    float lux_float = (float)raw_lux;
    if (manager->use_filter) {
        lux_float = MedianFilter_Update(&manager->filter, lux_float);
    }
    uint16_t filtered_lux = (uint16_t)lux_float;

    /* 7. 自适应增益调整 */
    if (manager->auto_gain_enabled) {
        manager->adjustment_counter++;

        if (manager->adjustment_counter >= BH1750_ADJUSTMENT_INTERVAL) {
            manager->adjustment_counter = 0;

            /* 光照太弱 → 切换到高分辨率（更敏感） */
            if (filtered_lux < BH1750_UNDERFLOW_TH && manager->data.mode != BH1750_RESOLUTION_HIGH2) {
                manager->data.mode = BH1750_RESOLUTION_HIGH2;
                UART5_SendString("[BH1750] Mode: HIGH2 (0.5 lux/step)\r\n");
            }
            /* 光照太强 → 切换到低分辨率 */
            else if (filtered_lux > BH1750_MAX_LUX_HIGH && manager->data.mode != BH1750_RESOLUTION_LOW) {
                manager->data.mode = BH1750_RESOLUTION_LOW;
                UART5_SendString("[BH1750] Mode: LOW (4 lux/step)\r\n");
            }
            /* 光照适中 → 切换到标准分辨率 */
            else if (filtered_lux > 100 && filtered_lux < BH1750_MAX_LUX_HIGH &&
                     manager->data.mode != BH1750_RESOLUTION_HIGH) {
                manager->data.mode = BH1750_RESOLUTION_HIGH;
                UART5_SendString("[BH1750] Mode: HIGH (1 lux/step)\r\n");
            }
        }
    }

    /* 8. 更新数据 */
    manager->data.lux = filtered_lux;
    manager->data.timestamp = xTaskGetTickCount();
    manager->success_count++;

    *data = manager->data;

    UART5_Printf("[BH1750] Lux=%d, Mode=%d, Saturated=%d\r\n",
                 filtered_lux, manager->data.mode, manager->data.is_saturated);

    return BH1750_OK;
}

BH1750_Status_t BH1750_SetResolution(BH1750_Manager_t *manager, BH1750_Resolution_t mode)
{
    if (manager == NULL) {
        return BH1750_COMM_ERR;
    }

    if (BH1750_SendCommand((uint8_t)mode) != 0) {
        return BH1750_COMM_ERR;
    }

    manager->data.mode = mode;

    const char *mode_str[] = {"HIGH (1 lux)", "HIGH2 (0.5 lux)", "LOW (4 lux)"};
    UART5_Printf("[BH1750] Resolution set to %s\r\n", mode_str[mode - 0x10]);

    return BH1750_OK;
}

uint8_t BH1750_GetStatistics(BH1750_Manager_t *manager,
                             uint16_t *success_count,
                             uint16_t *error_count,
                             uint16_t *saturation_count)
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

    if (saturation_count != NULL) {
        *saturation_count = manager->saturation_count;
    }

    uint32_t total = manager->success_count + manager->error_count;
    if (total == 0) {
        return 0;
    }

    return (uint8_t)((manager->success_count * 100) / total);
}

void BH1750_Reset(BH1750_Manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    manager->error_count = 0;
    manager->success_count = 0;
    manager->saturation_count = 0;
    manager->adjustment_counter = 0;

    if (manager->use_filter) {
        MedianFilter_Reset(&manager->filter);
    }
}
