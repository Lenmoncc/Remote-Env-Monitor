#include <stdio.h>
#include <modbus.h>
#include <errno.h>
#include <unistd.h>

#define SLAVE_ID 1
#define BAUDRATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY 'N'

// 寄存器地址定义（从机映射）
#define REG_AHT10_TEMP 0x0000
#define REG_AHT10_HUMI 0x0001
#define REG_BH1750_LUX 0x0002
#define REG_BMP280_TEMP 0x0003
#define REG_BMP280_PRESS 0x0004
#define REG_SGP30_CO2 0x0005
#define REG_SGP30_TVOC 0x0006
#define NUM_REGISTERS 7

int main(void)
{
    modbus_t *ctx = NULL;
    uint16_t tab_reg[NUM_REGISTERS] = {0};
    int rc;
    int i;
    
    // 创建RTU上下文
    ctx = modbus_new_rtu("/dev/ttymxc2", BAUDRATE, PARITY, DATA_BITS, STOP_BITS);
    if (ctx == NULL) {
        fprintf(stderr, "Unable to create the libmodbus context\n");
        return -1;
    }
    
    // 设置从机地址
    modbus_set_slave(ctx, SLAVE_ID);
    
    // 设置响应超时
    modbus_set_response_timeout(ctx, 1, 0);
    
    // 连接Modbus从机
    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }
    
    printf("Connected to Modbus slave\n");
    
    while (1) {
        // 读取保持寄存器
        rc = modbus_read_registers(ctx, REG_AHT10_TEMP, NUM_REGISTERS, tab_reg);
        if (rc == -1) {
            fprintf(stderr, "Read failed: %s\n", modbus_strerror(errno));
            continue;
        }
        printf("实际读取寄存器数量: %d\n", rc);
        // 解析并打印传感器数据
        printf("=== Sensor Data ===\n");
        printf("AHT10 Temperature: %d °C\n", (int16_t)tab_reg[0]);
        printf("AHT10 Humidity: %d %%\n", tab_reg[1]);
        printf("BH1750 Light: %d lux\n", tab_reg[2]);
        printf("BMP280 Temperature: %d °C\n", (int16_t)tab_reg[3]);
        printf("BMP280 Pressure: %d hPa\n", tab_reg[4]);
        printf("SGP30 CO2: %d ppm\n", tab_reg[5]);
        printf("SGP30 TVOC: %d ppb\n", tab_reg[6]);
        printf("===================\n\n");
        
        sleep(2);
    }
    
    // 关闭连接并释放资源
    modbus_close(ctx);
    modbus_free(ctx);
    
    return 0;
}

//编译
//arm-linux-gnueabihf-gcc master.c -I /home/test/linux/libmodbus/include/modbus/ -L /home/test/linux/libmodbus/lib -lmodbus -o master

