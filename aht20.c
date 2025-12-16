#include "aht20.h"
#include "i2c.h" // 确保你的 CubeMX 生成了 i2c.h，里面有 hi2c1 定义

// AHT20 I2C地址 (7位地址 0x38, 左移一位变为 0x70)
#define AHT20_ADDR  (0x38 << 1)

void AHT20_Init(void)
{
    // 1. 【关键】给传感器上电！
    // 根据原理图，PB15 连接 AHT-VDD
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

    // 2. 上电后必须等待至少 100ms 让传感器内部电路稳定
    HAL_Delay(100);
}

void AHT20_Read_Data(AHT20_Data_t *data)
{
    uint8_t sendBuffer[3] = {0xAC, 0x33, 0x00}; // 触发测量命令
    uint8_t readBuffer[6]; // 存放读回来的数据

    // 1. 发送触发命令
    // 参数: I2C句柄, 设备地址, 发送数组, 长度, 超时时间
    if (HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR, sendBuffer, 3, 100) != HAL_OK)
    {
        data->ok = 0; // 发送失败 (可能是线没接好)
        return;
    }

    // 2. 等待测量完成 (AHT20 需要约 80ms)
    HAL_Delay(80);

    // 3. 读取 6 字节数据
    // 格式: 状态字(1) + 湿度数据(2.5) + 温度数据(2.5)
    if (HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR, readBuffer, 6, 100) != HAL_OK)
    {
        data->ok = 0; // 读取失败
        return;
    }

    // 4. 数据解析与计算 (位运算拼接)
    // 湿度是 20 bit
    uint32_t humi_raw = ((uint32_t)readBuffer[1] << 12) | ((uint32_t)readBuffer[2] << 4) | (readBuffer[3] >> 4);
    // 温度是 20 bit
    uint32_t temp_raw = ((uint32_t)(readBuffer[3] & 0x0F) << 16) | ((uint32_t)readBuffer[4] << 8) | readBuffer[5];

    // 5. 转换为物理量 (公式来自数据手册)
    data->humidity = (float)humi_raw * 100.0f / 1048576.0f; // 2^20 = 1048576
    data->temperature = ((float)temp_raw * 200.0f / 1048576.0f) - 50.0f;

    data->ok = 1; // 标记成功
}
