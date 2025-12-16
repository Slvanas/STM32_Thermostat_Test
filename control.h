#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"
#include "aht20.h" // 需要知道 AHT20_Data_t 结构体

// 定义温控阈值 (修改这里即可调整策略)
#define TEMP_HEAT_START   5.0f   // 温度 < 5.0 开启加热
#define TEMP_HEAT_STOP    13.0f  // 温度 >= 13.0 停止加热

// 定义湿控阈值
#define HUMI_FAN_START    85.0f  // 湿度 >= 85.0 开启风扇
#define HUMI_FAN_STOP     75.0f  // 湿度 < 75.0 停止风扇

// 初始化控制模块 (比如上电默认关闭继电器)
void Control_Init(void);

// 核心处理函数 (传入最新的传感器数据)
void Control_Process(AHT20_Data_t *data);

#endif
