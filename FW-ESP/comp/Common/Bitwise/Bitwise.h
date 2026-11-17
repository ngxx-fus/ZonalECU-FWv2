/********************************************************************************************************************************
 * @file Mask.h
 * @ingroup FW_ESP
 * @brief Header file for [[Mask]]
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __FW_ESP_COMP_COMMON_MASK_MASK_H__
#define __FW_ESP_COMP_COMMON_MASK_MASK_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdint.h>
#include "../Config/Macros.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define UINT64(x) ((uint64_t)(x))
#define UINT32(x) ((uint32_t)(x))
#define UINT16(x) ((uint16_t)(x))
#define UINT8(x)  ((uint8_t)(x))

#define INT64(x) ((uint64_t)(x))
#define INT32(x) ((uint32_t)(x))
#define INT16(x) ((uint16_t)(x))
#define INT8(x)  ((uint8_t)(x))

#define UINT64_BIT(bit) UINT64(1LLU << (bit))
#define UINT32_BIT(bit) UINT32(1U << (bit))
#define UINT16_BIT(bit) UINT16(1U << (bit))
#define UINT8_BIT(bit)  UINT8(1U << (bit))

#define BW_INV(X)    (~(X))
#define BW_OR(A, B)  ((A) | (B))
#define BW_AND(A, B) ((A) & (B))
#define BW_XOR(A, B) ((A) ^ (B))

#define BW_OR3(A, B, C)  BW_OR(BW_OR(A, B), C)
#define BW_AND3(A, B, C) BW_AND(BW_AND(A, B), C)
#define BW_XOR3(A, B, C) BW_XOR(BW_AND(A, B), C)

#define BW_OR4(A, B, C)  BW_OR(BW_OR3(A, B, C), D)
#define BW_AND4(A, B, C) BW_AND(BW_AND3(A, B, C), D)
#define BW_XOR4(A, B, C) BW_XOR(BW_XOR(A, B, C), D)

#define BW_SELF_AND(n, x) ((n) = BW_AND((n), (x)))
#define BW_SELF_OR(n, x)  ((n) = BW_OR((n), (x)))
#define BW_SELF_XOR(n, x) ((n) = BW_XOR((n), (x)))

#define UINT64_IS_SET(n, bit) IS_NOT_ZERO(BW_AND(n, UINT64_BIT(bit)))
#define UINT32_IS_SET(n, bit) IS_NOT_ZERO(BW_AND(n, UINT32_BIT(bit)))
#define UINT16_IS_SET(n, bit) IS_NOT_ZERO(BW_AND(n, UINT16_BIT(bit)))
#define UINT8_IS_SET(n, bit)  IS_NOT_ZERO(BW_AND(n, UINT8_BIT(bit)))

#define UINT64_IS_CLR(n, bit) IS_ZERO(BW_AND(n, UINT64_BIT(bit)))
#define UINT32_IS_CLR(n, bit) IS_ZERO(BW_AND(n, UINT32_BIT(bit)))
#define UINT16_IS_CLR(n, bit) IS_ZERO(BW_AND(n, UINT16_BIT(bit)))
#define UINT8_IS_CLR(n, bit)  IS_ZERO(BW_AND(n, UINT8_BIT(bit)))

#define UINT64_BIT_SET(n, bit) BW_SELF_OR(n, UINT64_BIT(bit))
#define UINT32_BIT_SET(n, bit) BW_SELF_OR(n, UINT32_BIT(bit))
#define UINT16_BIT_SET(n, bit) BW_SELF_OR(n, UINT16_BIT(bit))
#define UINT8_BIT_SET(n, bit)  BW_SELF_OR(n, UINT8_BIT(bit))

#define UINT64_BIT_CLR(n, bit) BW_SELF_AND(n, BW_INV(UINT64_BIT(bit)))
#define UINT32_BIT_CLR(n, bit) BW_SELF_AND(n, BW_INV(UINT32_BIT(bit)))
#define UINT16_BIT_CLR(n, bit) BW_SELF_AND(n, BW_INV(UINT16_BIT(bit)))
#define UINT8_BIT_CLR(n, bit)  BW_SELF_AND(n, BW_INV(UINT8_BIT(bit)))

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __FW_ESP_COMP_COMMON_MASK_MASK_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/