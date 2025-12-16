#include "control.h"

// 初始化：上电默认关闭所有继电器，确保安全
void Control_Init(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // 关加热
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET); // 关风扇
    // 如果有报警输出(PC15)，也可以在这里关掉
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
}

// 核心逻辑：根据传入的数据决定继电器动作
void Control_Process(AHT20_Data_t *data)
{
    // 1. 安全检查：如果传感器读数无效，强制关闭设备
    if(data->ok == 0)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
        return; // 直接返回，不执行后面逻辑
    }

    // 2. 加热控制逻辑 (Hysteresis / 迟滞比较)
    // 对应原理图 PC13 (HEAT)
    if(data->temperature < TEMP_HEAT_START)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); // 启动
    }
    else if(data->temperature >= TEMP_HEAT_STOP)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // 停止
    }
    // 注意：在 5~13 度之间，继电器保持上一次的状态不变

    // 3. 除湿/风扇控制逻辑
    // 对应原理图 PC14 (FAN/HUMID)
    if(data->humidity >= HUMI_FAN_START)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET); // 启动
    }
    else if(data->humidity < HUMI_FAN_STOP)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET); // 停止
    }
}
