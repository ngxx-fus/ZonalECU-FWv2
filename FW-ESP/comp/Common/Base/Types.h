/********************************************************************************************************************************
 * @file Types.h
 * @ingroup ZonalECU_FWv2
 * @brief Header file for [[Types]]. Standardized primitive scalar, pointer, and word-access union types.
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __ZONALECU_FWV2_FW_ESP_COMP_COMMON_BASE_TYPES_H__
#define __ZONALECU_FWV2_FW_ESP_COMP_COMMON_BASE_TYPES_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "__prv_common__.h"
#include "Macros.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define HWORD2BYTE  (2U)               /*!< Number of bytes in a half-word (16 bits) */

#define WORD2BYTE   (4U)               /*!< Number of bytes in a single word (32 bits) */
#define WORD2HWORD  (2U)               /*!< Number of half-words in a single word */

#define DWORD2WORD  (2U)               /*!< Number of words in a double-word (64 bits) */
#define DWORD2HWORD (WORD2HWORD << 1U) /*!< Number of half-words in a double-word (4) */
#define DWORD2BYTE  (WORD2BYTE << 1U)  /*!< Number of bytes in a double-word (8) */

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* Standard unsigned integer pointer types ***************************************************************************************/

typedef uint64_t * uint64ptr_t; /*!< Pointer to unsigned 64-bit integer */
typedef uint32_t * uint32ptr_t; /*!< Pointer to unsigned 32-bit integer */
typedef uint16_t * uint16ptr_t; /*!< Pointer to unsigned 16-bit integer */
typedef uint8_t  * uint8ptr_t;  /*!< Pointer to unsigned 8-bit integer */

/* Standard signed integer pointer types *****************************************************************************************/

typedef int64_t * int64ptr_t; /*!< Pointer to signed 64-bit integer */
typedef int32_t * int32ptr_t; /*!< Pointer to signed 32-bit integer */
typedef int16_t * int16ptr_t; /*!< Pointer to signed 16-bit integer */
typedef int8_t  * int8ptr_t;  /*!< Pointer to signed 8-bit integer */

/* Standard size and generic types ***********************************************************************************************/

typedef uint32_t   ze_size_t;         /*!< Standard size type (32-bit unsigned) */

typedef void       ze_generic_data_t; /*!< Base generic data type without pointer (pair with ro_ptr/rw_ptr) */
typedef void       void_t;            /*!< Standard void type alias */
typedef void       ze_void_t;         /*!< Standard void type alias with ze prefix */
typedef void     * voidptr_t;         /*!< Pointer to generic/void memory buffer */
typedef void     * ze_voidptr_t;      /*!< Pointer to generic/void memory buffer with ze prefix */

typedef uint8_t    ze_byte_t;         /*!< Standard single-byte data type (8-bit unsigned) */

/* Word and bit-width structured unions ******************************************************************************************/

/** @brief  16-bit half-word representation with byte-level access */
typedef union ze_hword_un {
    uint16_t  half;              /*!< 16-bit scalar access */
    ze_byte_t byte [HWORD2BYTE]; /*!< 8-bit individual byte access */
} ze_hword_t;

/** @brief  32-bit word representation with half-word and byte-level access */
typedef union ze_word_un {
    uint32_t   word;                 /*!< 32-bit scalar access */
    uint16_t   half    [WORD2HWORD]; /*!< 16-bit scalar half-word access */
    ze_hword_t half_st [WORD2HWORD]; /*!< 16-bit structured half-word union access */
    ze_byte_t  byte    [WORD2BYTE];  /*!< 8-bit individual byte access */
} ze_word_t;

/** @brief  64-bit double-word representation with word, half-word, and byte-level access */
typedef union ze_dword_un {
    uint64_t   dword;                 /*!< 64-bit scalar access */
    uint32_t   word    [DWORD2WORD];  /*!< 32-bit scalar word access */
    ze_word_t  word_st [DWORD2WORD];  /*!< 32-bit structured word union access */
    ze_hword_t half    [DWORD2HWORD]; /*!< 16-bit structured half-word access */
    ze_byte_t  byte    [DWORD2BYTE];  /*!< 8-bit individual byte access */
} ze_dword_t;

/* Typed pointer aliases for structured unions ***********************************************************************************/

typedef ze_byte_t  * ze_byteptr_t;  /*!< Pointer to a byte */
typedef ze_hword_t * ze_hwordptr_t; /*!< Pointer to a 16-bit half-word union */
typedef ze_word_t  * ze_wordptr_t;  /*!< Pointer to a 32-bit word union */
typedef ze_dword_t * ze_dwordptr_t; /*!< Pointer to a 64-bit double-word union */

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __ZONALECU_FWV2_FW_ESP_COMP_COMMON_BASE_TYPES_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/