#ifndef __AHT20_H
#define __AHT20_H

#include "main.h" // 只要引用这个，I2C_HandleTypeDef 就能认出来

// 定义一个结构体来存放温湿度
typedef struct {
    float temperature; // 温度值 (摄氏度)
    float humidity;    // 湿度值 (%)
    uint8_t ok;        // 状态标记 (1:读取成功, 0:失败)
} AHT20_Data_t;

// 【外部接口】
// 初始化函数 (给传感器上电)
void AHT20_Init(void);

// 读取函数 (执行一次完整的 触发->等待->读取 流程)
// 参数：传入结构体指针，函数会把读到的值填进去
void AHT20_Read_Data(AHT20_Data_t *data);

#endif
