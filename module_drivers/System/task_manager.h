#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/*=================================================================================
 * 任务优先级定义
 * FreeRTOS 支持 0 到 (configMAX_PRIORITIES - 1) 的优先级
 * 0 = 最低优先级，4 = 最高优先级
 =================================================================================*/
#define PRIO_SENSOR_TASK    1   /* 传感器采集任务：低优先级，可被中断 */
#define PRIO_MODBUS_TASK    3   /* Modbus 通信任务：高优先级，快速响应 */
#define PRIO_MONITOR_TASK   2   /* 监控任务：中等优先级 */

/*=================================================================================
 * 任务堆栈大小（字数）
 * 每个 uint32_t 是 4 字节
 =================================================================================*/
#define STACK_SENSOR_TASK   (configMINIMAL_STACK_SIZE * 2)   /* 260 字 */
#define STACK_MODBUS_TASK   (configMINIMAL_STACK_SIZE * 3)   /* 390 字 */
#define STACK_MONITOR_TASK  (configMINIMAL_STACK_SIZE * 2)   /* 260 字 */

/*=================================================================================
 * 传感器数据结构
 * 该结构体被互斥量保护，用于 SensorTask 和 Modbus 回调函数间的数据交换
 =================================================================================*/
typedef struct {
    float temperature_aht10;    /* AHT10 温度（℃） */
    float humidity_aht10;       /* AHT10 湿度（%RH） */
    int lux;                    /* BH1750 光照（lux） */
    float temperature_bmp280;   /* BMP280 温度（℃） */
    float pressure_bmp280;      /* BMP280 气压（hPa） */
    int co2_eq;                 /* SGP30 CO₂等效值（ppm） */
    int tvoc;                   /* SGP30 TVOC（ppb） */
    uint32_t lastUpdateTime;    /* 最后更新时间戳（滴答） */
} SensorData_t;

/*=================================================================================
 * 全局数据和同步原语
 =================================================================================*/
extern SensorData_t g_sensorData;
extern SemaphoreHandle_t g_sensorDataMutex;
extern QueueHandle_t g_modbusEventQueue;

/*=================================================================================
 * 函数声明
 =================================================================================*/

/**
 * @brief  初始化任务管理器
 * @note   创建所有任务和同步原语，但不启动调度器
 *         调度器由 main() 启动
 */
void TaskManager_Init(void);

/**
 * @brief  传感器采集任务
 * @param  pvParameters：任务参数（FreeRTOS 使用）
 */
void SensorTask(void *pvParameters);

/**
 * @brief  Modbus 通信任务
 * @param  pvParameters：任务参数（FreeRTOS 使用）
 */
void ModbusTask(void *pvParameters);

/**
 * @brief  监控任务（可选）
 * @param  pvParameters：任务参数（FreeRTOS 使用）
 */
void MonitorTask(void *pvParameters);

/*=================================================================================
 * 数据访问接口（带互斥量保护）
 =================================================================================*/

/**
 * @brief  获取传感器数据快照（线程安全）
 * @return 返回当前的传感器数据副本
 * @note   此函数会阻塞等待互斥量
 */
SensorData_t SensorData_GetSnapshot(void);

/**
 * @brief  更新温度数据（线程安全）
 * @param  temp_aht10：AHT10 温度
 * @param  temp_bmp280：BMP280 温度
 */
void SensorData_UpdateTemperature(float temp_aht10, float temp_bmp280);

/**
 * @brief  更新湿度数据（线程安全）
 * @param  humidity：相对湿度
 */
void SensorData_UpdateHumidity(float humidity);

/**
 * @brief  更新光照数据（线程安全）
 * @param  lux：光照值
 */
void SensorData_UpdateLight(int lux);

/**
 * @brief  更新气压数据（线程安全）
 * @param  pressure：气压（hPa）
 */
void SensorData_UpdatePressure(float pressure);

/**
 * @brief  更新气体数据（线程安全）
 * @param  co2_eq：CO₂等效值
 * @param  tvoc：TVOC
 */
void SensorData_UpdateGas(int co2_eq, int tvoc);

#endif /* __TASK_MANAGER_H__ */
