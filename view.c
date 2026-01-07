#include "view.h"

// 更新段码表: 增加 'r' (索引20)
static const uint8_t SEG_CODE[] = {
    0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90, // 0-9
    0x88, // 10: A
    0x83, // 11: b
    0xC6, // 12: C
    0xA1, // 13: d
    0x86, // 14: E
    0x8E, // 15: F
    0x89, // 16: H
    0xC7, // 17: L
    0xBF, // 18: -
    0xFF, // 19: Empty
    0xAF  // 20: r (用于菜单 r-1, r-2) <--- 新增
};

static uint8_t scan_index = 0;
uint8_t DISP_BUFF[6] = {18, 18, 18, 18, 18, 18};

void View_Scan(void)
{
    // ... (保持原有的 View_Scan 代码不变) ...
    // 为了节省篇幅，这里不重复 View_Scan 的具体实现
    // 请确保上面的 SEG_CODE 数组已被更新

    // 1. 全局消隐
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);
    GPIOA->ODR &= 0xFF00;

    // 2. 取段码
    uint8_t num_idx = DISP_BUFF[scan_index];
    if(num_idx >= sizeof(SEG_CODE)) num_idx = 19;
    uint8_t seg_val = SEG_CODE[num_idx];

    // 3. 输出
    GPIOA->ODR |= seg_val;

    // 4. 位选
    switch(scan_index) {
        case 0: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); break;
        case 1: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET); break;
        case 2: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); break;
        case 3: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); break;
        case 4: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET); break;
        case 5: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); break;
    }

    scan_index++;
    if (scan_index >= 6) scan_index = 0;
}
