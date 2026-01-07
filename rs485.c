#include "rs485.h"
#include "usart.h" // 引用 huart1
#include "aht20.h" // 引用传感器数据类型

// 【修改点1】: 引用 main.c 中定义的新变量
extern AHT20_Data_t sensor_data_1; // 通道1
extern AHT20_Data_t sensor_data_2; // 通道2

// --- 变量定义 ---
uint8_t rs485_rx_buffer[1];   // 接收单字节缓存
uint8_t rec[20];              // 接收完整包缓存
uint8_t recs = 0;             // 接收计数
uint8_t send_buf[20];         // 发送缓存

// 全局任务标志 (由中断置位，main处理)
volatile uint8_t rs485_task_flag = 0;

// --- 内部函数声明 ---
void sendpccmd27(void);
void sendpccmd34(void);
unsigned char CRC16(unsigned char *tempsend, unsigned char Data_Len);

// --- 硬件控制 ---
// High = 发送 (TX), Low = 接收 (RX)
static void RS485_TX_Mode(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
}

static void RS485_RX_Mode(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
}

// --- 初始化 ---
void RS485_Init(void)
{
    RS485_RX_Mode(); // 默认进入接收模式
    HAL_UART_Receive_IT(&huart1, rs485_rx_buffer, 1);
}

// --- 接收中断回调 ---
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        // 1. 存入 buffer
        rec[recs++] = rs485_rx_buffer[0];

        // 2. 防溢出保护
        if(recs >= 20) recs = 0;

        // 3. 协议解析 (简单帧头判断)
        // 地址 + 功能码校验
        if(rec[0] == DEVICE_ADDR && recs >= 8)
        {
            if(rec[1] == 0x03)
            {
                // 读数据命令 0x27
                if(rec[3] == 0x27 && rec[5] == 0x04)
                {
                    rs485_task_flag = 1;
                    recs = 0;
                }
                // 读状态命令 0x34
                else if(rec[3] == 0x34 && rec[5] == 0x01)
                {
                    rs485_task_flag = 2;
                    recs = 0;
                }
            }
        }

        // 4. 继续接收
        HAL_UART_Receive_IT(&huart1, rs485_rx_buffer, 1);
    }
}

// --- 主循环调用的处理任务 ---
void RS485_Process_Task(void)
{
    if(rs485_task_flag == 1)
    {
        sendpccmd27();
        rs485_task_flag = 0;
    }
    else if(rs485_task_flag == 2)
    {
        sendpccmd34();
        rs485_task_flag = 0;
    }
}

// --- 辅助函数：填充温湿度数据 ---
// 将 AHT20 数据打包进 buffer 的指定位置 (index_start)
static void Pack_Sensor_Data(AHT20_Data_t *data, uint8_t *buf, uint8_t index_start)
{
    if (data->ok == 1)
    {
        // 温度计算: (temp * 10) + 500 的偏移算法
        int16_t wddata = (int16_t)(data->temperature * 10) + 500;
        int16_t pldata = (int16_t)(data->humidity * 10);

        // 温度打包
        if (wddata >= 500) {
            buf[index_start]     = (wddata - 500) >> 8;
            buf[index_start + 1] = (wddata - 500);
        } else {
            buf[index_start]     = (((500 - wddata) >> 8) | 0x80);
            buf[index_start + 1] = (500 - wddata);
        }

        // 湿度打包
        buf[index_start + 2] = pldata >> 8;
        buf[index_start + 3] = pldata;

        // 异常值保护
        if(wddata < 200) {
            buf[index_start] = 0x00; buf[index_start+1] = 0xCC;
        }
    }
    else
    {
        // 传感器故障显示 0x00CC
        buf[index_start]     = 0x00; buf[index_start + 1] = 0xCC; // 温度Err
        buf[index_start + 2] = 0x00; buf[index_start + 3] = 0xCC; // 湿度Err
    }
}

// --- 发送命令 0x27 (读数值) ---
void sendpccmd27(void)
{
    send_buf[0] = DEVICE_ADDR;
    send_buf[1] = 0x03;
    send_buf[2] = 0x08; // 数据总字节数 (2路 * 4字节 = 8)

    // 【修改点2】: 分别填充通道1和通道2的数据
    // 通道 1 (字节 3,4,5,6)
    Pack_Sensor_Data(&sensor_data_1, send_buf, 3);

    // 通道 2 (字节 7,8,9,10)
    Pack_Sensor_Data(&sensor_data_2, send_buf, 7);

    // 计算 CRC16 (长度 = 地址1 + 功能1 + 长度1 + 数据8 = 11)
    CRC16(send_buf, 11);

    // 发送
    RS485_TX_Mode();
    HAL_UART_Transmit(&huart1, send_buf, 13, 100);
    RS485_RX_Mode();
}

// --- 发送命令 0x34 (读状态) ---
void sendpccmd34(void)
{
    send_buf[0] = DEVICE_ADDR;
    send_buf[1] = 0x03;
    send_buf[2] = 0x02;
    send_buf[3] = 0x00;
    send_buf[4] = 0x00;

    // 读取继电器状态
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) send_buf[4] |= 0x01; // 加热
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_SET) send_buf[4] |= 0x02; // 风扇

    CRC16(send_buf, 5);

    RS485_TX_Mode();
    HAL_UART_Transmit(&huart1, send_buf, 7, 100);
    RS485_RX_Mode();
}

unsigned char CRC16(unsigned char *tempsend, unsigned char Data_Len)
{
    unsigned char CRC16Lo = 0xff, CRC16Hi = 0xff, CH = 0xA0, CL = 0x01;
    unsigned char SaveHi, SaveLo;
    unsigned char i, Flag;

    for (i = 0; i < Data_Len; i++)
    {
        CRC16Lo = (unsigned char)(CRC16Lo ^ tempsend[i]);
        for (Flag = 0; Flag < 8; Flag++)
        {
            SaveHi = CRC16Hi;
            SaveLo = CRC16Lo;
            CRC16Hi = (unsigned char)(CRC16Hi >> 1);
            CRC16Lo = (unsigned char)(CRC16Lo >> 1);
            if ((SaveHi & 0x01) == 0x01) {
                CRC16Lo = (unsigned char)(CRC16Lo | 0x80);
            }
            if ((SaveLo & 0x01) == 0x01) {
                CRC16Hi = (unsigned char)(CRC16Hi ^ CH);
                CRC16Lo = (unsigned char)(CRC16Lo ^ CL);
            }
        }
    }
    tempsend[Data_Len + 1] = CRC16Hi;
    tempsend[Data_Len] = CRC16Lo;
    return 1;
}
