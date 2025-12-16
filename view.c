#include "view.h"

// 【内部私有变量】
// 段码表 (0-9) - 不需要给别人看，定义为 static 比较安全
static const uint8_t SEG_CODE[] = {
		0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90, //无小数点
		0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10,	//有小数点
		0xC6,0x89,0x8C,0xDC,0xE3,0x86,0xAF,0xBF,0xff,0xc7,0x88,0xa1,0x83,0xe3,0XA3
		//20   21   22   23	   24  25  26   27	  28   29   30    31   32   33  34
		//C,   H,   P,  上限, 下限,  E   R  －     空    L   A     d     b   u    o
};

// 当前扫描索引
static uint8_t scan_index = 0;

// 【全局变量】
// 显示缓存 (初始化显示 123456)
uint8_t DISP_BUFF[6] = {1, 2, 3, 4, 5, 6};

// 【功能函数】
// 这个函数会被定时器每 2ms 调用一次
void View_Scan(void)
{
    // 1. 消隐 (关位选和段选)
    // 根据原理图: 位选在 PB0-2, PB12-14 [cite: 327-362]
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);


    // 2. 准备数据
    uint8_t num_val = DISP_BUFF[scan_index];
    if(num_val > 9) num_val = 0; // 越界保护
    uint8_t seg_val = SEG_CODE[num_val];

    // 3. 输出段码 (操作 PA 低8位 ODR)
    GPIOA->ODR = (GPIOA->ODR & 0xFF00) | seg_val;

    // 4. 打开位选
    switch(scan_index)
    {
        case 0: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;
        case 1: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); break;
        case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); break;
        case 3: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); break;
        case 4: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); break;
        case 5: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); break;
    }

    // 5. 索引递增
    scan_index++;
    if (scan_index >= 6) scan_index = 0;
}
