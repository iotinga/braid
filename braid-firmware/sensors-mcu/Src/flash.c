#include "flash.h"

// #define FLASH_USER_START_ADDR (FLASH_BASE + FLASH_PAGE_SIZE * 128)           /* Start @ of user Flash area */
// #define FLASH_USER_END_ADDR   (FLASH_USER_START_ADDR + FLASH_PAGE_SIZE * 32) /* End @ of user Flash area */
// #define FLASH_PAGE_TO_BE_PROTECTED OB_WRP_Pages128to159

#define TEST_DATA ((uint32_t)0x12345678)

static HAL_StatusTypeDef Get_OptionsBytes(FLASH_OBProgramInitTypeDef *optionsBytes) {
    // Unlock the Flash to enable the flash control register access
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    // Unlock the Options Bytes
    status = HAL_FLASH_OB_Unlock();
    if (status != HAL_OK) {
        return status;
    }

    // Get pages write protection status
    HAL_FLASHEx_OBGetConfig(optionsBytes);

    return HAL_OK;
}

HAL_StatusTypeDef Flash_EnableProtection(uint32_t pages) {
    FLASH_OBProgramInitTypeDef OptionsBytes;
    HAL_StatusTypeDef status = Get_OptionsBytes(&OptionsBytes);
    if (status != HAL_OK) {
        return status;
    }

    // Check if desired pages are not yet write protected
    if ((OptionsBytes.WRPSector & pages) != pages) {
        // Enable write protection
        OptionsBytes.OptionType = OPTIONBYTE_WRP | OPTIONBYTE_RDP;
        OptionsBytes.WRPState = OB_WRPSTATE_ENABLE;
        OptionsBytes.WRPSector = pages;
        // Level 1 allows protection to be disabled
        OptionsBytes.RDPLevel = OB_RDP_LEVEL_1;
        if (HAL_FLASHEx_OBProgram(&OptionsBytes) != HAL_OK) {
            return HAL_ERROR;
        }

        // Generate System Reset to load the new option byte values
        status = HAL_FLASH_OB_Launch();
        if (status != HAL_OK) {
            return status;
        }
    }

    // Lock the Options Bytes
    status = HAL_FLASH_OB_Lock();
    if (status != HAL_OK) {
        return status;
    }

    return HAL_FLASH_Lock();
}

HAL_StatusTypeDef Flash_DisableProtection(uint32_t pages) {
    FLASH_OBProgramInitTypeDef OptionsBytes;
    HAL_StatusTypeDef status = Get_OptionsBytes(&OptionsBytes);
    if (status != HAL_OK) {
        return status;
    }

    // Check if desired pages are already write protected
    if ((OptionsBytes.WRPSector & pages) == pages) {
        // Restore write protected pages
        OptionsBytes.OptionType = OPTIONBYTE_WRP | OPTIONBYTE_RDP;
        OptionsBytes.WRPState = OB_WRPSTATE_DISABLE;
        OptionsBytes.WRPSector = pages;
        OptionsBytes.RDPLevel = OB_RDP_LEVEL_0;
        if (HAL_FLASHEx_OBProgram(&OptionsBytes) != HAL_OK) {
            return HAL_ERROR;
        }

        // Generate System Reset to load the new option byte values
        status = HAL_FLASH_OB_Launch();
        if (status != HAL_OK) {
            return status;
        }
    }

    // Lock the Options Bytes
    status = HAL_FLASH_OB_Lock();
    if (status != HAL_OK) {
        return status;
    }

    return HAL_FLASH_Lock();
}

HAL_StatusTypeDef Flash_WriteTest(uint32_t pageAddr) {
    HAL_FLASH_Unlock();

    // Check that it is not allowed to write in this page
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, pageAddr, TEST_DATA) != HAL_OK) {
        if (HAL_FLASH_GetError() != HAL_FLASH_ERROR_WRP) {
            return HAL_ERROR;
        }
    }

    return HAL_FLASH_Lock();
}

HAL_StatusTypeDef Flash_Erase(uint32_t firstPageAddr, uint32_t lastPageAddr) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    // Fill EraseInit structure
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = firstPageAddr;
    EraseInitStruct.NbPages = (lastPageAddr - firstPageAddr) / FLASH_PAGE_SIZE;

    HAL_FLASH_Unlock();

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        /*
          Error occurred while page erase.
          User can add here some code to deal with this error.
          PageError will contain the faulty page and then to know the code error on this page,
          user can call function 'HAL_FLASH_GetError()'
        */
        return HAL_ERROR;
    }

    // FLASH Word program of TEST_DATA at addresses defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR
    uint32_t Address = firstPageAddr;
    while (Address < lastPageAddr) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, TEST_DATA) == HAL_OK) {
            Address = Address + 4;
        } else {
            return HAL_ERROR;
        }
    }

    // Check the correctness of written data
    Address = firstPageAddr;
    while (Address < lastPageAddr) {
        if ((*(__IO uint32_t *)Address) != TEST_DATA) {
            return HAL_ERROR;
        }
        Address += 4;
    }

    return HAL_FLASH_Lock();
}
