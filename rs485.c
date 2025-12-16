#include "rs485.h"
#include "usart.h" // 引用 huart1
#include "aht20.h" // 引用传感器数据类型

// 引用外部变量 (来自 main.c 或 aht20.c)
extern AHT20_Data_t sensor_data;

volatile uint8_t rs485_task_flag = 0; // 0:无任务, 1:需回传数据, 2:需回传状态
volatile uint8_t rs485_cmd_received = 0; // 新增标志位

// --- 变量定义 ---
uint8_t rs485_rx_buffer[1];   // 接收单字节缓存
uint8_t rec[20];              // 接收完整包缓存
uint8_t recs = 0;             // 接收计数
uint8_t send_buf[20];         // 发送缓存

// --- 内部函数声明 ---
void sendpccmd27(void);
void sendpccmd34(void);
unsigned char CRC16(unsigned char *tempsend, unsigned char Data_Len);

// --- 硬件控制 ---
// 根据原理图: PA11 连接 RS485_RD (RE/DE)
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
    // 开启中断接收 1 个字节
    HAL_UART_Receive_IT(&huart1, rs485_rx_buffer, 1);
}


// --- 发送命令 0x27 (读数值) ---
void sendpccmd27(void)
{
    send_buf[0] = DEVICE_ADDR;
    send_buf[1] = 0x03;
    send_buf[2] = 0x08; // 数据字节数

    // 检查传感器状态 (由 AHT20 CRC8 校验决定)
    if (sensor_data.ok == 1)
    {
        // --- 正常数据处理 (保留旧代码算法) ---
        // 温度: 放大10倍 + 500偏移
        int16_t wddata = (int16_t)(sensor_data.temperature * 10) + 500;
        // 湿度: 放大10倍
        int16_t pldata = (int16_t)(sensor_data.humidity * 10);

        // 温度打包 (大端/小端特殊处理)
        if (wddata >= 500) {
            send_buf[3] = (wddata - 500) >> 8;
            send_buf[4] = (wddata - 500);
        } else {
            send_buf[3] = (((500 - wddata) >> 8) | 0x80);
            send_buf[4] = (500 - wddata);
        }

        // 湿度打包
        send_buf[5] = pldata >> 8;
        send_buf[6] = pldata;

        // 异常值保护 (旧代码逻辑)
        if(wddata < 200) {
            send_buf[3] = 0x00; send_buf[4] = 0xCC;
        }
    }
    else
    {
        // --- 异常处理 (发送 0x00CC) ---
        send_buf[3] = 0x00; send_buf[4] = 0xCC; // 温度错误
        send_buf[5] = 0x00; send_buf[6] = 0xCC; // 湿度错误
    }

    // 通道2 (预留)
    send_buf[7] = 0x00; send_buf[8] = 0x00;
    send_buf[9] = 0x00; send_buf[10] = 0x00;

    // 计算 CRC16
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

    // 读取继电器实际输出电平状态
    // PC13: 加热, PC14: 风扇
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET) send_buf[4] |= 0x01;
    if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14) == GPIO_PIN_SET) send_buf[4] |= 0x02;

    CRC16(send_buf, 5);

    RS485_TX_Mode();
    HAL_UART_Transmit(&huart1, send_buf, 7, 100);
    RS485_RX_Mode();
}

// --- CRC16 算法 (完全保留旧代码) ---
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


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        rec[recs++] = rs485_rx_buffer[0];
        if(recs > 20) recs = 0;

        if(rec[0] == DEVICE_ADDR && recs >= 8) // 简单校验
        {
        	rs485_cmd_received = 1;
            if(rec[1] == 0x03)
            {
                if(rec[3] == 0x27 && rec[5] == 0x04)
                {
                    rs485_task_flag = 1; // 通知主循环发送数据
                    recs = 0;
                }
                else if(rec[3] == 0x34 && rec[5] == 0x01)
                {
                    rs485_task_flag = 2; // 通知主循环发送状态
                    recs = 0;
                }
            }
        }
        HAL_UART_Receive_IT(&huart1, rs485_rx_buffer, 1);
    }
}

void RS485_Process_Task(void)
{
    if(rs485_cmd_received)
    {
        rs485_cmd_received = 0;
        if(rec[3] == 0x27) sendpccmd27();
        else if(rec[3] == 0x34) sendpccmd34();
        recs = 0; // 处理完再清零计数
    }
}
