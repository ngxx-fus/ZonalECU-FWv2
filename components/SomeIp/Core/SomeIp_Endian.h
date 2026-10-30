/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_ENDIAN_H__
#define __SOMEIP_ENDIAN_H__

/*
 * SOME/IP byte order helpers.
 *
 *  - Header and TP header are always big-endian                              [PRS_SOMEIP_00368] [PRS_SOMEIP_00931]
 *  - Payload byte order is configurable per parameter                        [PRS_SOMEIP_00369], Table 4.6
 *  - Receiver must handle unaligned messages, so every access is byte-wise   [PRS_SOMEIP_00142]

 * Reference : AUTOSAR_PRS_SOMEIPProtocol, chapter 4.1.3 / 4.1.4.1
 */

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include <stdint.h>
#include "Macros.h"
#include "SomeIp_Types.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
ZE_HEADER

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* TYPEDEFS *************************************************************************************************
 ************************************************************************************************************/

/* FUNCTIONS ************************************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * BIG-ENDIAN (header, TP header, default payload)
 * ======================================================================================================== */

/* {0x12, 0x34} -> 0x1234 */
static inline uint16_t SomeIp_ReadU16_BE(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

/* {0xDE, 0xAD, 0xBE, 0xEF} -> 0xDEADBEEF */
static inline uint32_t SomeIp_ReadU32_BE(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] <<  8) |
           ((uint32_t)p[3]);
}

/* {0x01, 0x02, ..., 0x08} -> 0x0102030405060708  (cast to uint64_t before shifting >= 32) */
static inline uint64_t SomeIp_ReadU64_BE(const uint8_t *p)
{
    return ((uint64_t)SomeIp_ReadU32_BE(&p[0]) << 32) |
           ((uint64_t)SomeIp_ReadU32_BE(&p[4]));
}

/* 0x1234 -> {0x12, 0x34} */
static inline void SomeIp_WriteU16_BE(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

/* 0xDEADBEEF -> {0xDE, 0xAD, 0xBE, 0xEF} */
static inline void SomeIp_WriteU32_BE(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v);
}

/* 0x0102030405060708 -> {0x01, 0x02, ..., 0x08} */
static inline void SomeIp_WriteU64_BE(uint8_t *p, uint64_t v)
{
    SomeIp_WriteU32_BE(&p[0], (uint32_t)(v >> 32));
    SomeIp_WriteU32_BE(&p[4], (uint32_t)(v));
}

/* ==========================================================================================================
 * LITTLE-ENDIAN (payload parameters configured as little-endian)                          [PRS_SOMEIP_00369]
 * ======================================================================================================== */

/* {0x12, 0x34} -> 0x3412 */
static inline uint16_t SomeIp_ReadU16_LE(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[1] << 8) | (uint16_t)p[0]);
}

/* {0xEF, 0xBE, 0xAD, 0xDE} -> 0xDEADBEEF */
static inline uint32_t SomeIp_ReadU32_LE(const uint8_t *p)
{
    return ((uint32_t)p[3] << 24) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[1] <<  8) |
           ((uint32_t)p[0]);
}

/* {0x08, 0x07, ..., 0x01} -> 0x0102030405060708  (high half is in p[4..7]) */
static inline uint64_t SomeIp_ReadU64_LE(const uint8_t *p)
{
    return ((uint64_t)SomeIp_ReadU32_LE(&p[4]) << 32) |
           ((uint64_t)SomeIp_ReadU32_LE(&p[0]));
}

/* 0x1234 -> {0x34, 0x12} */
static inline void SomeIp_WriteU16_LE(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

/* 0xDEADBEEF -> {0xEF, 0xBE, 0xAD, 0xDE} */
static inline void SomeIp_WriteU32_LE(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >>  8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

/* 0x0102030405060708 -> {0x08, 0x07, ..., 0x01}  (low half first) */
static inline void SomeIp_WriteU64_LE(uint8_t *p, uint64_t v)
{
    SomeIp_WriteU32_LE(&p[0], (uint32_t)(v));
    SomeIp_WriteU32_LE(&p[4], (uint32_t)(v >> 32));
}

/* ==========================================================================================================
 * RUNTIME BYTE ORDER (used by Serdes: order comes from the parameter configuration)
 *  Just dispatch to the BE / LE functions above.
 * ======================================================================================================== */

static inline uint16_t SomeIp_ReadU16(const uint8_t *p, SomeIp_ByteOrder_t order)
{
    return (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN) ? SomeIp_ReadU16_LE(p) : SomeIp_ReadU16_BE(p);
}

static inline uint32_t SomeIp_ReadU32(const uint8_t *p, SomeIp_ByteOrder_t order)
{
    return (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN) ? SomeIp_ReadU32_LE(p) : SomeIp_ReadU32_BE(p);
}

static inline uint64_t SomeIp_ReadU64(const uint8_t *p, SomeIp_ByteOrder_t order)
{
    return (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN) ? SomeIp_ReadU64_LE(p) : SomeIp_ReadU64_BE(p);
}

static inline void SomeIp_WriteU16(uint8_t *p, uint16_t v, SomeIp_ByteOrder_t order)
{
    if (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN)
    {
        SomeIp_WriteU16_LE(p, v);
    }
    else
    {
        SomeIp_WriteU16_BE(p, v);
    }
}

static inline void SomeIp_WriteU32(uint8_t *p, uint32_t v, SomeIp_ByteOrder_t order)
{
    if (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN)
    {
        SomeIp_WriteU32_LE(p, v);
    }
    else
    {
        SomeIp_WriteU32_BE(p, v);
    }
}

static inline void SomeIp_WriteU64(uint8_t *p, uint64_t v, SomeIp_ByteOrder_t order)
{
    if (order == SOMEIP_BYTE_ORDER_LITTLE_ENDIAN)
    {
        SomeIp_WriteU64_LE(p, v);
    }
    else
    {
        SomeIp_WriteU64_BE(p, v);
    }
}

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_ENDIAN_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
