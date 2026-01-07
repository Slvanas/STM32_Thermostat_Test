/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (包含双路显示 + 继电器模式设置)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "view.h"
#include "aht20.h"
#include "rs485.h"
#include "control.h"
#include "flash_storage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    MENU_IDLE = 0,
    MENU_C1, MENU_C2, // 温度上下限
    MENU_H1, MENU_H2, // 湿度上下限
    MENU_R1, MENU_R2  //
} Menu_State_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 双路传感器数据
AHT20_Data_t sensor_data_1;
AHT20_Data_t sensor_data_2;

// 菜单相关
Menu_State_t current_menu = MENU_IDLE;
uint32_t menu_timeout = 0;

// 显示切换相关
uint8_t current_disp_ch = 1;
uint32_t disp_switch_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void Key_Logic_Task(void);
void Display_Update_Task(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim3);
  AHT20_Init();
  RS485_Init();
  Control_Init(); // 读取 Flash 参数
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_key_tick = 0;
  uint32_t last_1s_tick = 0;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // 1. RS485
    RS485_Process_Task();

    // 2. 按键 (20ms)
    if(HAL_GetTick() - last_key_tick > 20)
    {
        last_key_tick = HAL_GetTick();
        Key_Logic_Task();
    }

    // 3. 业务逻辑 (1秒)
    if(HAL_GetTick() - last_1s_tick > 1000)
    {
        last_1s_tick = HAL_GetTick();

        // --- 读取传感器 (读取1路)
        AHT20_Read_Data(&sensor_data_1);
        // sensor_data_2.ok = 0; // 2路

        // --- 控制逻辑 ---
        Control_Process(&sensor_data_1);
        Control_Manual_Tick();

        // --- 5秒自动切换显示通道 ---
        if(current_menu == MENU_IDLE)
        {
            if(HAL_GetTick() - disp_switch_tick > 5000)
            {
                disp_switch_tick = HAL_GetTick();
                current_disp_ch++;
                if(current_disp_ch > 2) current_disp_ch = 1;
            }
        }

        // --- 刷新显示 ---
        Display_Update_Task();
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
// --- 按键核心逻辑 ---
void Key_Logic_Task(void)
{
    static uint32_t set_press_start = 0;
    static uint8_t set_long_pressed = 0;
    static uint8_t sw_done = 0, up_done = 0, down_done = 0;

    // 读取按键 (低电平有效)
    uint8_t k_set = (HAL_GPIO_ReadPin(GPIOB, KEY_SET_Pin) == GPIO_PIN_RESET);
    uint8_t k_sw  = (HAL_GPIO_ReadPin(GPIOB, KEY_SW_Pin)  == GPIO_PIN_RESET);
    uint8_t k_up  = (HAL_GPIO_ReadPin(GPIOB, KEY_UP_Pin)  == GPIO_PIN_RESET);
    uint8_t k_down= (HAL_GPIO_ReadPin(GPIOB, KEY_DOWN_Pin)== GPIO_PIN_RESET);

    // [SET] 键逻辑
    if(k_set) {
        if(set_press_start == 0) set_press_start = HAL_GetTick();
        if(!set_long_pressed && (HAL_GetTick() - set_press_start > 5000)) {
            set_long_pressed = 1;
            // 进/出菜单
            if(current_menu == MENU_IDLE) current_menu = MENU_C1;
            else { Flash_Save_Params(); current_menu = MENU_IDLE; }
            menu_timeout = HAL_GetTick();
        }
    } else {
        if(set_press_start != 0 && !set_long_pressed) { // 短按
            if(current_menu != MENU_IDLE) {
                current_menu++;
                if(current_menu > MENU_R2) { // 循环到 R2 后退出
                    Flash_Save_Params();
                    current_menu = MENU_IDLE;
                }
                menu_timeout = HAL_GetTick();
            }
        }
        set_press_start = 0;
        set_long_pressed = 0;
    }

    // [SW] 键逻辑
    if(k_sw && !sw_done) {
        if(current_menu == MENU_IDLE) {
            manual_heat_active = !manual_heat_active;
            manual_timer = manual_heat_active ? 1800 : 0;
        }
        sw_done = 1;
    }
    if(!k_sw) sw_done = 0;

    // [UP/DOWN] 键逻辑
    if(current_menu != MENU_IDLE) {
        if((k_up && !up_done) || (k_down && !down_done)) {
            menu_timeout = HAL_GetTick();
            float change = k_up ? 1.0f : -1.0f;

            switch(current_menu) {
                case MENU_C1: sys_params.temp_high += change; break;
                case MENU_C2: sys_params.temp_low  += change; break;
                case MENU_H1: sys_params.humi_high += change; break;
                case MENU_H2: sys_params.humi_low  += change; break;

                // 模式切换 (0 <-> 1)
                case MENU_R1: sys_params.relay1_mode = !sys_params.relay1_mode; break;
                case MENU_R2: sys_params.relay2_mode = !sys_params.relay2_mode; break;
                default: break;
            }
            if(k_up) up_done = 1;
            if(k_down) down_done = 1;
        }
    }
    if(!k_up) up_done = 0;
    if(!k_down) down_done = 0;

    // 超时退出
    if(current_menu != MENU_IDLE && (HAL_GetTick() - menu_timeout > 30000)) {
        Flash_Save_Params();
        current_menu = MENU_IDLE;
    }
}

// --- 显示更新逻辑 ---
void Display_Update_Task(void)
{
    // 菜单模式
    if(current_menu != MENU_IDLE) {
        uint8_t ch = 19, sub = 19;
        int val_disp = -1;
        uint8_t is_mode_menu = 0;

        switch(current_menu) {
            case MENU_C1: ch=12; sub=1; val_disp=(int)sys_params.temp_high; break;
            case MENU_C2: ch=12; sub=2; val_disp=(int)sys_params.temp_low;  break;
            case MENU_H1: ch=16; sub=1; val_disp=(int)sys_params.humi_high; break;
            case MENU_H2: ch=16; sub=2; val_disp=(int)sys_params.humi_low;  break;
            case MENU_R1: ch=20; sub=1; is_mode_menu=1; val_disp=sys_params.relay1_mode; break;
            case MENU_R2: ch=20; sub=2; is_mode_menu=1; val_disp=sys_params.relay2_mode; break;
            default: break;
        }

        DISP_BUFF[0] = ch; DISP_BUFF[1] = 18; DISP_BUFF[2] = sub;

        if(is_mode_menu) {
            // 显示 H(16) 或 F(15)
            DISP_BUFF[3] = 19;
            DISP_BUFF[4] = (val_disp == 0) ? 16 : 15;
            DISP_BUFF[5] = 19;
        } else {
            // 显示数字
            DISP_BUFF[3] = val_disp/10;
            DISP_BUFF[4] = val_disp%10;
            DISP_BUFF[5] = 19;
        }
    }
    // 正常模式 (双路切换)
    else {
        AHT20_Data_t *pData = (current_disp_ch == 1) ? &sensor_data_1 : &sensor_data_2;

        if(pData && pData->ok) {
            int t = (int)pData->temperature;
            int h = (int)pData->humidity;
            // 格式: 25C 80H (中间的C可以用12表示, H用16)
            // 或者第3位显示通道号? 目前代码用C/H分隔
            DISP_BUFF[0] = t/10; DISP_BUFF[1] = t%10; DISP_BUFF[2] = 12; // C
            DISP_BUFF[3] = h/10; DISP_BUFF[4] = h%10; DISP_BUFF[5] = 16; // H
        } else {
            // Err
            for(int i=0; i<6; i++) DISP_BUFF[i] = (i<3)?14:18;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM3) View_Scan();
}

void Error_Handler(void) { __disable_irq(); while (1) {} }
/* USER CODE END 4 */
