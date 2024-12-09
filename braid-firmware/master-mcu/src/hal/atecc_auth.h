/**
 * @file
 * @brief   Contains API for performing the symmetric Authentication between the Host and the device
 *
 * @copyright (c) 2015-2020 Microchip Technology Inc. and its subsidiaries.
 *
 * @page License
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE
 * OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
 * MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
 * FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL
 * LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED
 * THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR
 * THIS SOFTWARE.
 */

#pragma once

#include "cryptoauthlib.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Authenticates access to the ATECC device by comparing generated secrets.
 *
 * @param[in]  slot                The slot number used for the symmetric authentication.
 * @param[in]  master_key          The master key used for the calculating the symmetric key.
 * @param[in]  rand_number         The 20 byte rand_number from the host.
 * @return `ATCA_SUCCESS` on successful authentication, otherwise an error code.
 */
ATCA_STATUS atecc_authenticate(uint8_t slot, const uint8_t *master_key, const uint8_t *rand_number);

#ifdef __cplusplus
}
#endif