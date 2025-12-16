#ifndef __RS485_H
#define __RS485_H

#include "main.h"

// 定义本机设备地址 (根据旧代码逻辑)
#define DEVICE_ADDR  0x01

// 初始化 RS485
void RS485_Init(void);

// 这里不需要 Process 函数，因为接收是在中断回调里自动处理的
// 但如果有复杂的包解析，可以加一个 Process 放在 main while(1) 里
// 目前逻辑简单，直接在中断回调处理即可。
void RS485_Process_Task(void);
#endif
