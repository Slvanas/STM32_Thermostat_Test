#include "control.h"
#include "flash_storage.h"

// 默认参数: Relay1=Heat(0), Relay2=Fan(1)
System_Params_t sys_params = {
    .temp_high = 13.0f,
    .temp_low = 5.0f,
    .humi_high = 85.0f,
    .humi_low = 75.0f,
    .relay1_mode = RELAY_MODE_HEAT, // 默认加热
    .relay2_mode = RELAY_MODE_FAN   // 默认风扇
};

uint8_t manual_heat_active = 0;
uint16_t manual_timer = 0;

// 继电器控制逻辑
static void Run_Relay_Logic(GPIO_TypeDef* port, uint16_t pin, uint8_t mode, AHT20_Data_t *data)
{
    if (mode == RELAY_MODE_HEAT)
    {
        if (manual_heat_active) {
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        } else {
            if (data->temperature < sys_params.temp_low)
                HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
            else if (data->temperature >= sys_params.temp_high)
                HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        }
    }
    else
    {
        if (data->humidity >= sys_params.humi_high)
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        else if (data->humidity < sys_params.humi_low)
            HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    }
}

void Control_Init(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);
    Flash_Load_Params();
}

void Control_Process(AHT20_Data_t *data)
{
    if(data->ok == 0) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
        manual_heat_active = 0;
        return;
    }

    // 控制继电器 1 (PC13)
    Run_Relay_Logic(GPIOC, GPIO_PIN_13, sys_params.relay1_mode, data);

    // 控制继电器 2 (PC14)
    Run_Relay_Logic(GPIOC, GPIO_PIN_14, sys_params.relay2_mode, data);
}

void Control_Manual_Tick(void) {
    if(manual_heat_active) {
        if(manual_timer > 0) manual_timer--;
        else manual_heat_active = 0;
    }
}
