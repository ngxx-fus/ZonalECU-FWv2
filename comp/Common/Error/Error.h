/**
 * @file Error.h
 * @ingroup Common
 * @brief Standard Zonal ECU Error Code definitions and conversion APIs.
 */
/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __COMMON_ERROR_H__
#define __COMMON_ERROR_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "Macros.h"
#include "esp_err.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
HEADER_CPP

/* DEFINITIONS **********************************************************************************************
 ************************************************************************************************************/

/** @brief Standard Zonal ECU Error Code definitions (zecu_err_t)
 *  @note  Mapped 1:1 with ESP-IDF standard error codes (esp_err_t) and extended for ECU peripherals. */
typedef enum ZECU_Err_e {

    /* Success & Generic Status (ESP_OK, ESP_FAIL) */
    ZECU_OK                             = ESP_OK,                  /*!< 0x000: Success / No error */
    ZECU_SUCCESS                        = ESP_OK,                  /*!< 0x000: Success alias */
    ZECU_FAIL                           = ESP_FAIL,                /*!< 0x1  : Generic failure */

    /* Standard ESP-IDF System Error Codes (Mapped from ESP_ERR_*) */
    ZECU_ERR_NO_MEM                     = ESP_ERR_NO_MEM,          /*!< 0x101: Out of memory */
    ZECU_ERR_INVALID_ARG                = ESP_ERR_INVALID_ARG,     /*!< 0x102: Invalid argument */
    ZECU_ERR_INVALID_STATE              = ESP_ERR_INVALID_STATE,   /*!< 0x103: Invalid state */
    ZECU_ERR_INVALID_SIZE               = ESP_ERR_INVALID_SIZE,    /*!< 0x104: Invalid size */
    ZECU_ERR_NOT_FOUND                  = ESP_ERR_NOT_FOUND,       /*!< 0x105: Requested resource not found */
    ZECU_ERR_NOT_SUPPORTED              = ESP_ERR_NOT_SUPPORTED,   /*!< 0x106: Operation or feature not supported */
    ZECU_ERR_TIMEOUT                    = ESP_ERR_TIMEOUT,         /*!< 0x107: Operation timed out */
    ZECU_ERR_INVALID_RESPONSE           = ESP_ERR_INVALID_RESPONSE,/*!< 0x108: Received response was invalid */
    ZECU_ERR_INVALID_CRC                = ESP_ERR_INVALID_CRC,     /*!< 0x109: CRC or checksum was invalid */
    ZECU_ERR_INVALID_VERSION            = ESP_ERR_INVALID_VERSION, /*!< 0x10A: Version was invalid */
    ZECU_ERR_INVALID_MAC                = ESP_ERR_INVALID_MAC,     /*!< 0x10B: MAC address was invalid */
    ZECU_ERR_NOT_FINISHED               = ESP_ERR_NOT_FINISHED,    /*!< 0x10C: Operation has not fully completed */
    ZECU_ERR_NOT_ALLOWED                = ESP_ERR_NOT_ALLOWED,     /*!< 0x10D: Action is not allowed */

    /* Custom Error Code Bases for Zonal ECU Peripheral Modules */
    ZECU_ERR_BASE_COMM                  = 0x1000,                  /*!< Base error code for communication modules */
    ZECU_ERR_BASE_W5500                 = 0x1100,                  /*!< Base error code for W5500 SPI Ethernet */
    ZECU_ERR_BASE_CAN                   = 0x1200,                  /*!< Base error code for CAN / CAN-FD bus */
    ZECU_ERR_BASE_STORAGE               = 0x1300,                  /*!< Base error code for Flash / EEPROM / NVS */
    ZECU_ERR_BASE_IO                    = 0x1400,                  /*!< Base error code for GPIO / ADC / Power switches */

} zecu_err_t;

/* SHARED FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/** @brief Returns constant string representation of a zecu_err_t error code
 *  @param err Error code
 *  @return const char* Static string representation of the error name */
const char * ZECU_ErrToStr(IN zecu_err_t err);

/** @brief Copies error string name into user-provided buffer
 *  @param err Error code
 *  @param str Destination character buffer (minimum 32 bytes)
 *  @return ZECU_OK on success, ZECU_ERR_INVALID_ARG if str is NULL */
zecu_err_t ZECU_Err2Str(IN zecu_err_t err, OUT char * str);

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
FOOTER_CPP

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __COMMON_ERROR_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
