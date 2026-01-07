#ifndef __CONTROL_H
#define __CONTROL_H

#include "main.h"
#include "aht20.h"

// 继电器模式定义
#define RELAY_MODE_HEAT 0
#define RELAY_MODE_FAN  1

typedef struct {
    float temp_high; // C1
    float temp_low;  // C2
    float humi_high; // H1
    float humi_low;  // H2
    // --- 新增参数 ---
    uint8_t relay1_mode; // 0:加热(H), 1:风扇(F)
    uint8_t relay2_mode; // 0:加热(H), 1:风扇(F)
} System_Params_t;

extern System_Params_t sys_params;
extern uint8_t manual_heat_active;
extern uint16_t manual_timer;

void Control_Init(void);
void Control_Process(AHT20_Data_t *data);
void Control_Manual_Tick(void);

#endif
