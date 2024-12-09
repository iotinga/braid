/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    rtc.c
 * @brief   This file provides code for the configuration
 *          of the RTC instances.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */
#include "factory_data.h"
#include "main.h"

typedef enum TamperStatus {
    INTACT,
    TAMPERED,
} TamperStatus;

__IO TamperStatus tamper2 = INTACT;
__IO TamperStatus tamper3 = INTACT;
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void) {
    /* USER CODE BEGIN RTC_Init 0 */

    /* USER CODE END RTC_Init 0 */

    RTC_TamperTypeDef sTamper = {0};

    /* USER CODE BEGIN RTC_Init 1 */

    /* USER CODE END RTC_Init 1 */

    /** Initialize RTC Only
     */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the WakeUp
     */
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the RTC Tamper 2
     */
    sTamper.Tamper = RTC_TAMPER_2;
    sTamper.Interrupt = RTC_ALL_TAMPER_INTERRUPT;
    sTamper.Trigger = RTC_TAMPERTRIGGER_RISINGEDGE;
    sTamper.NoErase = RTC_TAMPER_ERASE_BACKUP_ENABLE;
    sTamper.MaskFlag = RTC_TAMPERMASK_FLAG_DISABLE;
    sTamper.Filter = RTC_TAMPERFILTER_DISABLE;
    sTamper.SamplingFrequency = RTC_TAMPERSAMPLINGFREQ_RTCCLK_DIV2048;
    sTamper.PrechargeDuration = RTC_TAMPERPRECHARGEDURATION_1RTCCLK;
    sTamper.TamperPullUp = RTC_TAMPER_PULLUP_ENABLE;
    sTamper.TimeStampOnTamperDetection = RTC_TIMESTAMPONTAMPERDETECTION_ENABLE;
    if (HAL_RTCEx_SetTamper_IT(&hrtc, &sTamper) != HAL_OK) {
        Error_Handler();
    }

    /** Enable the RTC Tamper 3
     */
    sTamper.Tamper = RTC_TAMPER_3;
    sTamper.Trigger = RTC_TAMPERTRIGGER_FALLINGEDGE;
    if (HAL_RTCEx_SetTamper_IT(&hrtc, &sTamper) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN RTC_Init 2 */
    // Clear the Tamper interrupt pending bit
    __HAL_RTC_TAMPER_CLEAR_FLAG(&hrtc, RTC_FLAG_TAMP2F);
    __HAL_RTC_TAMPER_CLEAR_FLAG(&hrtc, RTC_FLAG_TAMP3F);
    // Since the interrupt handlers were called when resettings the flags, the sentinel values must be reset here
    tamper2 = INTACT;
    tamper3 = INTACT;
    /* USER CODE END RTC_Init 2 */
}

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle) {
    if (rtcHandle->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */
        /* RTC clock enable */
        __HAL_RCC_RTC_ENABLE();

        /* RTC interrupt Init */
        HAL_NVIC_SetPriority(RTC_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
        /* USER CODE BEGIN RTC_MspInit 1 */
        HAL_PWR_EnableBkUpAccess();
        /* USER CODE END RTC_MspInit 1 */
    }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle) {
    if (rtcHandle->Instance == RTC) {
        /* USER CODE BEGIN RTC_MspDeInit 0 */

        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();

        /* RTC interrupt Deinit */
        HAL_NVIC_DisableIRQ(RTC_IRQn);
        /* USER CODE BEGIN RTC_MspDeInit 1 */
        HAL_PWR_DisableBkUpAccess();
        /* USER CODE END RTC_MspDeInit 1 */
    }
}

/* USER CODE BEGIN 1 */
/**
 * @brief Checks if a tamper was triggered on pin PA0 (TAMP2). Should be checked as early as possible
 *
 * @return `true` If a tamper event was stored, `false` otherwise
 */
bool RTC_CheckTamper2() {
    return tamper2 != INTACT;
}

/**
 * @brief Checks if a tamper was triggered on pin PA2 (TAMP3). Should be checked as early as possible
 *
 * @return `true` If a tamper event was stored, `false` otherwise
 */
bool RTC_CheckTamper3() {
    return tamper3 != INTACT;
}

/**
 * @brief  Tamper 2 event callback function
 * @param  RTC handle
 * @retval None
 */
void HAL_RTCEx_Tamper2EventCallback(RTC_HandleTypeDef *hrtc) {
    tamper2 = TAMPERED;
}

/**
 * @brief  Tamper 3 event callback function
 * @param  RTC handle
 * @retval None
 */
void HAL_RTCEx_Tamper3EventCallback(RTC_HandleTypeDef *hrtc) {
    tamper3 = TAMPERED;
}
/* USER CODE END 1 */
