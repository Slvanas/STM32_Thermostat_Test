/*
 * view.h
 *
 *  Created on: Dec 16, 2025
 *      Author: 17253
 */

#ifndef SRC_VIEW_H_
#define SRC_VIEW_H_
// 包含 main.h 以便使用 GPIO_PIN_x 和 HAL 库定义
#include "main.h"

// 【外部接口】
// 1. 显示缓存：其他文件（比如逻辑层）只要修改这个数组，屏幕就会变
extern uint8_t DISP_BUFF[6];

// 2. 扫描函数：需要被定时器中断周期性调用
void View_Scan(void);


#endif /* SRC_VIEW_H_ */
