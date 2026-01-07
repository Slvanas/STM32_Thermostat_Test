#include "flash_storage.h"
#include "control.h"

// 改变这个值，强制初始化参数
#define FLASH_SAVE_ADDR  0x08007C00
#define MAGIC_NUM        0x5A5A1236 // <--- 修改了最后一位，确保覆盖旧版参数

// ... (其余代码与之前相同，Flash_Load_Params 和 Flash_Save_Params 逻辑通用，不用改)
void Flash_Load_Params(void)
{
    uint32_t magic = *(__IO uint32_t*)FLASH_SAVE_ADDR;
    if (magic == MAGIC_NUM) {
        // 读取
        uint32_t *pFlash = (uint32_t*)(FLASH_SAVE_ADDR + 4);
        uint32_t *pRam   = (uint32_t*)&sys_params;
        for (int i = 0; i < sizeof(System_Params_t) / 4; i++) pRam[i] = pFlash[i];
    } else {
        // 初始化默认值
        Flash_Save_Params();
    }
}

void Flash_Save_Params(void)
{
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_SAVE_ADDR;
    EraseInitStruct.NbPages = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SAVE_ADDR, MAGIC_NUM);
        uint32_t *pRam = (uint32_t*)&sys_params;
        uint32_t writeAddr = FLASH_SAVE_ADDR + 4;
        for (int i = 0; i < sizeof(System_Params_t) / 4; i++) {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, writeAddr, pRam[i]);
            writeAddr += 4;
        }
    }
    HAL_FLASH_Lock();
}
