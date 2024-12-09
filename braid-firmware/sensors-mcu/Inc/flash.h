#pragma once

#include "stm32l0xx.h"

/**
 * @brief Enables read and write protection for the given pages
 * @param pages  The sector mask to use (see `FLASH_OBProgramInitTypeDef.WRPSector`)
 * @return `HAL_ERROR` if errors were encountered, `HAL_OK` if the operation was successful
 */
HAL_StatusTypeDef Flash_EnableProtection(uint32_t pages);

/**
 * @brief Disables read and write protection for the given pages
 * @param pages  The sector mask to use (see `FLASH_OBProgramInitTypeDef.WRPSector`)
 * @return `HAL_ERROR` if errors were encountered, `HAL_OK` if the operation was successful
 */
HAL_StatusTypeDef Flash_DisableProtection(uint32_t pages);

/**
 * @brief Checks if a write to a page is successful or not. Can be used to check that flash protection was successfully
 * applied.
 * @param pageAddr  Page to test writing on (absolute address)
 * @return `HAL_ERROR` if errors were encountered, `HAL_OK` if the operation was successful
 */
HAL_StatusTypeDef Flash_WriteTest(uint32_t pageAddr);

/**
 * @brief Erases the whole flash. Necessary after disabling flash protection.
 * @param pages  The sector mask to use (see `FLASH_OBProgramInitTypeDef.WRPSector`)
 * @return `HAL_ERROR` if errors were encountered, `HAL_OK` if the operation was successful
 */
HAL_StatusTypeDef Flash_Erase(uint32_t firstPageAddr, uint32_t lastPageAddr);