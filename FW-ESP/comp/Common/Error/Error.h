/**
 * @file Error.h
 * @ingroup Common
 * @brief Standard Zonal ECU Error Code definitions and conversion APIs.
 */

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __COMMON_ERROR_H__
#define __COMMON_ERROR_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "Common/Config/All.h"
#include "esp_err.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Standard Zonal ECU Error Code definitions (ze_err_t)
 *  @note    Mapped with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
typedef enum ze_err_e {

    ZE_OK                   = ESP_OK,                     /*!< 0x0  : Success / No error */
    ZE_SUCCESS              = ESP_OK,                     /*!< 0x0  : Success alias */
    ZE_ERR                  = ESP_FAIL,                   /*!< 0x1  : Generic failure */
    ZE_FAIL                 = ESP_FAIL,                   /*!< 0x1  : Generic failure */

    ZE_ERR_NO_MEM           = ESP_ERR_NO_MEM,             /*!< 0x101: Out of memory */
    ZE_ERR_INVALID_ARG      = ESP_ERR_INVALID_ARG,        /*!< 0x102: Invalid argument */
    ZE_ERR_INVALID_STATE    = ESP_ERR_INVALID_STATE,      /*!< 0x103: Invalid state */
    ZE_ERR_INVALID_SIZE     = ESP_ERR_INVALID_SIZE,       /*!< 0x104: Invalid size */
    ZE_ERR_NOT_FOUND        = ESP_ERR_NOT_FOUND,          /*!< 0x105: Requested resource not found */
    ZE_ERR_NOT_SUPPORTED    = ESP_ERR_NOT_SUPPORTED,      /*!< 0x106: Operation or feature not supported */
    ZE_ERR_TIMEOUT          = ESP_ERR_TIMEOUT,            /*!< 0x107: Operation timed out */
    ZE_ERR_INVALID_RESPONSE = ESP_ERR_INVALID_RESPONSE,   /*!< 0x108: Received response was invalid */
    ZE_ERR_INVALID_CRC      = ESP_ERR_INVALID_CRC,        /*!< 0x109: CRC or checksum was invalid */
    ZE_ERR_INVALID_VERSION  = ESP_ERR_INVALID_VERSION,    /*!< 0x10A: Version was invalid */
    ZE_ERR_INVALID_MAC      = ESP_ERR_INVALID_MAC,        /*!< 0x10B: MAC address was invalid */
    ZE_ERR_NOT_FINISHED     = ESP_ERR_NOT_FINISHED,       /*!< 0x10C: Operation has not fully completed */
    ZE_ERR_NOT_ALLOWED      = ESP_ERR_NOT_ALLOWED,        /*!< 0x10D: Action is not allowed */

    ZE_ERR_BASE_COMM        = 0x1000,                     /*!< Base error code for communication modules */
    ZE_ERR_ALREADY_INIT     = ZE_ERR_BASE_COMM + 1,       /*!< 0x10E: Already initialized */
    ZE_ERR_INVALID_ACCESS   = ZE_ERR_BASE_COMM + 2,       /*!< 0x10F: Invalid access (wrong permission) */

    ZE_ERR_MULTI_TASK_BASE  = 0x2000,                     /*!< Base error code for multi-task */
    ZE_ERR_CANT_TAKE_LOCK   = ZE_ERR_MULTI_TASK_BASE + 1, /*!< Can't take lock, resource already locked */
    ZE_ERR_CANT_GIVE_LOCK   = ZE_ERR_MULTI_TASK_BASE + 2, /*!< Can't give lock, resource has not locked */
    ZE_ERR_RESOURCE_LOCKED  = ZE_ERR_MULTI_TASK_BASE + 3, /*!< Resource already locked */
    ZE_ERR_RESOURCE_FREE    = ZE_ERR_MULTI_TASK_BASE + 4, /*!< Resource has not locked yet */

    ZE_ERR_BASE_W5500       = 0x3000,                     /*!< Base error code for W5500 SPI Ethernet */
    ZE_ERR_BASE_CAN         = 0x4000,                     /*!< Base error code for CAN / CAN-FD bus */
    ZE_ERR_BASE_STORAGE     = 0x5000,                     /*!< Base error code for Flash / EEPROM / NVS */
    ZE_ERR_BASE_IO          = 0x6000,                     /*!< Base error code for GPIO / ADC / Power switches */

} ze_err_t;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Returns constant string representation of a ze_err_t error code
 *  @param   err Error code
 *  @return  const char* Static string representation of the error name */
const char *ZE_ErrToStr(IN ze_err_t err);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __COMMON_ERROR_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/