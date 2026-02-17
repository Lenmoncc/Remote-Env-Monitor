/**
 * @file    main.c
 * @brief   STM32F407 多传感器环境监测系统 - FreeRTOS 多任务版本
 *
 * 系统架构：
 *   - 传感器采集任务（Priority 1）：周期性采集传感器数据，1Hz
 *   - Modbus 通信任务（Priority 3）：快速处理 Modbus 请求，<50ms 响应时间
 *   - 监控任务（Priority 2）：定期输出系统状态信息
 *
 * 数据流：
 *   传感器 --采集--> 全局受保护数据结构 --读取--> Modbus回调 --响应--> 上位机
 *
 * @author  Claude Code
 * @date    2026-02-17
 */

#include "stm32f4xx.h"
#include "IIC_1.h"
#include "UART_5.h"
#include "USART_1.h"
#include "Delay.h"

#include "FreeRTOS.h"
#include "task.h"

#include "task_manager.h"
#include "mb.h"
#include "mbport.h"
#include "mb_user.h"

/*=================================================================================
 * main 函数 - 系统入口
 =================================================================================*/

int main(void)
{
    /* 系统初始化和硬件配置 */

    /* 1. 系统初始化
     * 配置系统时钟、内存管理等
     */
    SystemInit();
    UART5_SendString("=== System Initializing ===\r\n");

    /* 2. 初始化 I²C 总线
     * 供 AHT10、BH1750、SGP30 使用
     */
    IIC1_Init();
    Delay_ms(100);
    UART5_SendString("✓ I²C Bus initialized\r\n");

    /* 3. 初始化延时模块
     * 包括 TIM2（微秒延时）和 FreeRTOS 支持
     */
    Delay_Init();
    UART5_SendString("✓ Delay module initialized\r\n");

    /* 4. 初始化 USART1（Modbus 通信）
     * 115200 bps，由 ModbusTask 管理
     */
    USART1_Init(115200);
    UART5_SendString("✓ USART1 initialized (115200 bps)\r\n");

    UART5_SendString("\n=== FreeRTOS Initialization ===\r\n");

    /* 5. 初始化任务管理器
     * 创建所有任务和同步原语
     */
    TaskManager_Init();

    UART5_SendString("\n=== Starting FreeRTOS Scheduler ===\r\n");

    /* 6. 启动 FreeRTOS 调度器
     * 从这一刻起，调度器接管系统，定期运行各任务
     * 正常情况下，此函数不会返回
     */
    vTaskStartScheduler();

    /* 如果执行到这里，说明调度器启动失败 */
    UART5_SendString("ERROR: Scheduler failed to start!\r\n");
    while (1) {
        /* 无限循环，等待看门狗复位 */
    }

    return 0;
}

/*=================================================================================
 * 系统说明和优化点
 =================================================================================*/

/*
 * 改进说明：
 *
 * 1. 任务分离
 *    - SensorTask（优先级1）：采集传感器，可被中断
 *    - ModbusTask（优先级3）：快速处理通信，优先响应
 *    - MonitorTask（优先级2）：系统监控
 *
 * 2. Modbus 响应时间优化
 *    - 改造前：可能需要等待 SensorTask 采集完成（可能 >200ms）
 *    - 改造后：ModbusTask 优先级高，立即响应（<50ms）
 *    - 即使 SensorTask 正在执行（如 BH1750_ReadLight() 的 120ms 延时），
 *      ModbusTask 也可以抢占并响应 Modbus 请求
 *
 * 3. 数据线程安全
 *    - 所有传感器数据受互斥量保护
 *    - SensorTask 更新数据时加锁
 *    - Modbus 回调函数读取数据时加锁
 *    - 避免读取半更新的数据（撕裂）
 *
 * 4. 延时优化
 *    - Delay_ms() 改用 vTaskDelay()，让出 CPU
 *    - Delay_us() 保留轮询式，仅用于 <10us 的硬件时序
 *    - 传感器读取中的延时（80ms + 120ms）现在可以被其他任务使用
 *
 * 5. 堆栈使用
 *    - SensorTask：260 字（传感器数据结构）
 *    - ModbusTask：390 字（Modbus 帧缓冲）
 *    - MonitorTask：260 字（监控变量）
 *    - 总计：配置堆 ~75KB，实际使用 ~40-50KB
 *
 * 6. 内存分配
 *    - 互斥量：约 40-50 字节
 *    - 队列：约 100 字节
 *    - 任务控制块：约 100 字节/任务
 *    - 堆栈：见上文
 */

/*=================================================================================
 * 编译和部署说明
 =================================================================================*/

/*
 * 1. 编译
 *    - 确保 FreeRTOS_CORE、FreeRTOS_PORT、System 模块都在工程中
 *    - 新增文件：System/task_manager.c 和 System/task_manager.h
 *    - 修改文件：System/Delay.c 和 User/main.c
 *
 * 2. 烧录和测试
 *    - 查看 UART5 (115200 bps) 的启动信息
 *    - 应该看到以下输出：
 *      === System Initializing ===
 *      ✓ I²C Bus initialized
 *      ✓ Delay module initialized
 *      ✓ USART1 initialized (115200 bps)
 *      === FreeRTOS Initialization ===
 *      ========== TaskManager_Init ==========
 *      ✓ Sensor data mutex created
 *      ✓ Modbus event queue created
 *      ✓ SensorTask created (Priority: 1)
 *      ✓ ModbusTask created (Priority: 3)
 *      ✓ MonitorTask created (Priority: 2)
 *      ========== TaskManager Ready ==========
 *      === Starting FreeRTOS Scheduler ===
 *
 * 3. 验证
 *    - Modbus 上位机测试：应能快速读取传感器寄存器
 *    - 响应时间：应 <50ms
 *    - 数据一致性：多次读取同一寄存器，数据应一致
 *
 * 4. 调试
 *    - UART5 输出系统状态
 *    - 可用 UART5_Printf() 输出调试信息
 *    - 不要使用 printf()（会占用 USART1）
 *
 * 5. 优化方向
 *    - 如果 Modbus 响应仍不满足，可提高 ModbusTask 优先级到 4（最高）
 *    - 如果内存不足，可减小 STACK_*_TASK 的值（下限为 configMINIMAL_STACK_SIZE）
 *    - 如果需要更多传感器任务，可在 TaskManager_Init() 中添加
 */

/*=================================================================================
 * 故障排除
 =================================================================================*/

/*
 * 问题 1: 系统启动后立即复位
 * 原因：堆内存不足，内存分配失败
 * 解决：增大 FreeRTOSConfig.h 中的 configTOTAL_HEAP_SIZE
 *
 * 问题 2: Modbus 无响应
 * 原因：ModbusTask 未创建成功，或 USART1 初始化失败
 * 解决：检查 UART5 输出信息，找到失败点
 *
 * 问题 3: 传感器数据不更新
 * 原因：SensorTask 堆栈溢出或初始化失败
 * 解决：查看 UART5 输出，增加堆栈大小
 *
 * 问题 4: Modbus 数据不一致
 * 原因：读取中途数据被更新
 * 解决：已使用互斥量保护，不应出现此问题
 *
 * 问题 5: 系统响应缓慢
 * 原因：任务优先级设置不当
 * 解决：调整 PRIO_* 定义，确保重要任务优先级高
 */

#endif /* Main program end */
