# STM32F407+imx6ull
基于STM32F407微控制器与IMX6ULL处理器的多功能远程环境检测系统，支持多种环境参数的实时采集、本地UI显示。
## 目录介绍
- **Backup**：备份文件（重要调试版本代码）
- **LVGL**：UI界面工程（VS Code + Gui Guider）
- **Master**：在主机端使用libmodbus测试通信
- **module_drivers**：各种传感器的驱动和移植库（keil工程）
- **TEST**：keil工程模板，可使用ws2812快速测试
## 开发环境
- Keil MDK
- VS Code
- Gui Guider
## 移植库
（1）freemodbus（1.6.0）
（2）FreeRTOS-Kernel（11.2.0）
（3）libmodbus（3.1.4）
