/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "factory_data.h"
#include "flash.h"
#include "proto.h"
#include "proto_payload.h"
#include "sensors/lis2dh.h"
#include "sensors/sht4x.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#ifdef WRITE_PROTECTION_ENABLE
bool FlashProtectionEnabled = true;
#else
bool FlashProtectionEnabled = false;
#endif

FactoryData eepromData;
SensorPayload sensorPayload;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
    MX_I2C1_Init();
    MX_USART2_UART_Init();
    MX_RTC_Init();
    /* USER CODE BEGIN 2 */
    // If D2/PA_12 is NOT shorted to GND, disable flash protection
    if (HAL_GPIO_ReadPin(FLASH_PROT_DISABLE_GPIO_Port, FLASH_PROT_DISABLE_Pin) == GPIO_PIN_RESET) {
        FlashProtectionEnabled = false;
    }

    if (FlashProtectionEnabled) {
        uint32_t page128 = FLASH_BASE + FLASH_PAGE_SIZE * 128;
        ERR_CHECK(Flash_EnableProtection(OB_WRP_AllPages));
        ERR_CHECK(Flash_WriteTest(page128));
    } else {
        ERR_CHECK(Flash_DisableProtection(OB_WRP_AllPages));
    }

    // Read the EEPROM
    ERR_CHECK(FactoryData_Load(&eepromData));
    while (eepromData.tamper2 == 1) {
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        HAL_Delay(900);
    }
    while (eepromData.tamper3 == 1) {
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
        HAL_Delay(900);
    }
    FactoryData_Unload(&eepromData);

    // Init the temp/humid sensor
    sensirion_i2c_init();
    while (sht4x_probe() != 0) {
    }

    // Init the accelerometer
    stmdev_ctx_t stmdevCtx;
    lis2dh_init(&stmdevCtx);
    while (lis2dh_probe(&stmdevCtx) != 0) {
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    int16_t data_raw_acceleration[3];
    lis2dh12_reg_t reg;

    ERR_CHECK_CUSTOM(ProtoPing(&protoCtx), PROTO_SUCCESS);
    while (1) {
        ERR_CHECK_CUSTOM(ProtoProcessMessage(&protoCtx), PROTO_SUCCESS);

        // Check for tampering
        if (RTC_CheckTamper2()) {
            ERR_CHECK(FactoryData_Load(&eepromData));
            eepromData.tamper2 = 1;
            ERR_CHECK(FactoryData_EraseSecrets(&eepromData));
            FactoryData_Unload(&eepromData);
            HAL_NVIC_SystemReset();
        }
        if (RTC_CheckTamper3()) {
            ERR_CHECK(FactoryData_Load(&eepromData));
            eepromData.tamper3 = 1;
            ERR_CHECK(FactoryData_EraseSecrets(&eepromData));
            FactoryData_Unload(&eepromData);
            HAL_NVIC_SystemReset();
        }

        // Read output only if new value available
        ERR_CHECK_CUSTOM(lis2dh12_xl_data_ready_get(&stmdevCtx, &reg.byte), 0);
        if (reg.byte) {
            // Read accelerometer data
            memset(data_raw_acceleration, 0, 3 * sizeof(int16_t));
            lis2dh12_acceleration_raw_get(&stmdevCtx, data_raw_acceleration);
            sensorPayload.data.acceleration_mg[0] = lis2dh12_from_fs2_hr_to_mg(data_raw_acceleration[0]);
            sensorPayload.data.acceleration_mg[1] = lis2dh12_from_fs2_hr_to_mg(data_raw_acceleration[1]);
            sensorPayload.data.acceleration_mg[2] = lis2dh12_from_fs2_hr_to_mg(data_raw_acceleration[2]);
        }

        // Measure temperature and relative humidity and store into variables temperature, humidity (each output
        // multiplied by 1000)
        ERR_CHECK_CUSTOM(sht4x_measure_blocking_read(&sensorPayload.data.temperature, &sensorPayload.data.humidity),
                         STATUS_OK);

        ERR_CHECK(FactoryData_Load(&eepromData));
        PayloadHash(&sensorPayload, eepromData.key, HMAC_KEY_LENGTH);
        FactoryData_Unload(&eepromData);
        ERR_CHECK_CUSTOM(
            ProtoSend(&protoCtx, PROTO_MSG_TYPE_RESPONSE, sizeof(SensorPayload), (uint8_t *)&sensorPayload),
            PROTO_SUCCESS);

        HAL_Delay(500);
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_4;
    RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state
     */
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        HAL_Delay(100);
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
