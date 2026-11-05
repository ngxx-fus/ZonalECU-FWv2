/**
 ************************************************************************************************************
 * @file    SomeIp_Serialization.h
 * @brief   SOME/IP payload serialization / deserialization: public API.
 *
 * @details - Byte order per parameter                                     [PRS_SOMEIP_00369]
 *          - Basic datatypes (Table 4.6), boolean                         [PRS_SOMEIP_00065] [PRS_SOMEIP_00615]
 *          - Alignment / padding behind variable length data              [PRS_SOMEIP_00611] [PRS_SOMEIP_00569]
 *                                                                        [PRS_SOMEIP_00612] [PRS_SOMEIP_00613]
 *          - Length field of struct / array                               [PRS_SOMEIP_00079] [PRS_SOMEIP_00370]
 *                                                                        [PRS_SOMEIP_00371] [PRS_SOMEIP_00900]
 *                                                                        [PRS_SOMEIP_00376] [PRS_SOMEIP_00377]
 *          - Strings (fixed / dynamic length, BOM, "\0")                  [PRS_SOMEIP_00087] [PRS_SOMEIP_00089]
 *          - Enumeration / bitfield: use the unsigned integer functions   [PRS_SOMEIP_00705] [PRS_SOMEIP_00300]
 *
 *          Usage: one cursor per payload. Every function stops on the first error and keeps it
 *          (sticky status), so a sequence of calls can be checked once at the end.
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.3 / 4.1.4 (4.1.4.1 .. 4.1.4.7)
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_SERIALIZATION_H__
#define __SOMEIP_SERIALIZATION_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "Macros.h"
#include "SomeIp_Types.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
ZE_HEADER

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* TYPEDEFS *************************************************************************************************
 ************************************************************************************************************/

/* Write cursor (serializer) */
typedef struct
{
    uint8_t            *buf;                     /* output buffer                                         */
    uint32_t            size;                    /* capacity of buf in bytes                              */
    uint32_t            pos;                     /* next byte to write = bytes written so far             */
    uint32_t            msgOffset;               /* offset of buf[0] from the start of the SOME/IP message */
                                                 /* (SOMEIP_OFS_PAYLOAD if buf is the payload) [00569]    */
    SomeIp_ByteOrder_t  order;                   /* byte order of the next parameters        [00369]      */
    SomeIp_Result_t     status;                  /* first error, SOMEIP_RES_OK while no error             */
} SomeIp_SerCursor_t;

/* Read cursor (deserializer) */
typedef struct
{
    const uint8_t      *buf;                     /* received payload                                      */
    uint32_t            size;                    /* number of valid bytes in buf                          */
    uint32_t            pos;                     /* next byte to read                                     */
    uint32_t            msgOffset;               /* offset of buf[0] from the start of the SOME/IP message */
    SomeIp_ByteOrder_t  order;
    SomeIp_Result_t     status;
} SomeIp_DeserCursor_t;

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * SERIALIZER
 * ======================================================================================================== */

/* ---- Cursor --------------------------------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Ser_Init(SomeIp_SerCursor_t *cur, uint8_t *buf, uint32_t size, uint32_t msgOffset,
                                SomeIp_ByteOrder_t order);
SomeIp_Result_t SomeIp_Ser_SetByteOrder(SomeIp_SerCursor_t *cur, SomeIp_ByteOrder_t order);
SomeIp_Result_t SomeIp_Ser_GetStatus(const SomeIp_SerCursor_t *cur);
uint32_t        SomeIp_Ser_GetLength(const SomeIp_SerCursor_t *cur);

/* ---- Basic datatypes (Table 4.6) ------------------------------------------------------------------------ */
SomeIp_Result_t SomeIp_Ser_WriteBool(SomeIp_SerCursor_t *cur, uint8_t v);
SomeIp_Result_t SomeIp_Ser_WriteU8(SomeIp_SerCursor_t *cur, uint8_t v);
SomeIp_Result_t SomeIp_Ser_WriteU16(SomeIp_SerCursor_t *cur, uint16_t v);
SomeIp_Result_t SomeIp_Ser_WriteU32(SomeIp_SerCursor_t *cur, uint32_t v);
SomeIp_Result_t SomeIp_Ser_WriteU64(SomeIp_SerCursor_t *cur, uint64_t v);
SomeIp_Result_t SomeIp_Ser_WriteS8(SomeIp_SerCursor_t *cur, int8_t v);
SomeIp_Result_t SomeIp_Ser_WriteS16(SomeIp_SerCursor_t *cur, int16_t v);
SomeIp_Result_t SomeIp_Ser_WriteS32(SomeIp_SerCursor_t *cur, int32_t v);
SomeIp_Result_t SomeIp_Ser_WriteS64(SomeIp_SerCursor_t *cur, int64_t v);
SomeIp_Result_t SomeIp_Ser_WriteF32(SomeIp_SerCursor_t *cur, float v);
SomeIp_Result_t SomeIp_Ser_WriteF64(SomeIp_SerCursor_t *cur, double v);

/* ---- Raw bytes / alignment ------------------------------------------------------------------------------ */
SomeIp_Result_t SomeIp_Ser_WriteBytes(SomeIp_SerCursor_t *cur, const uint8_t *data, uint32_t len);
SomeIp_Result_t SomeIp_Ser_Align(SomeIp_SerCursor_t *cur, uint16_t alignBits);

/* ---- Length field (struct, array) ----------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Ser_LengthBegin(SomeIp_SerCursor_t *cur, SomeIp_LengthFieldSize_t lfSize, uint32_t *mark);
SomeIp_Result_t SomeIp_Ser_LengthEnd(SomeIp_SerCursor_t *cur, SomeIp_LengthFieldSize_t lfSize, uint32_t mark);

/* ---- Strings -------------------------------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Ser_WriteString(SomeIp_SerCursor_t *cur, SomeIp_StringEncoding_t enc,
                                       SomeIp_LengthFieldSize_t lfSize, const uint8_t *str, uint32_t len);
SomeIp_Result_t SomeIp_Ser_WriteStringFixed(SomeIp_SerCursor_t *cur, SomeIp_StringEncoding_t enc,
                                            uint32_t fixedSize, const uint8_t *str, uint32_t len);

/* ==========================================================================================================
 * DESERIALIZER
 * ======================================================================================================== */

/* ---- Cursor --------------------------------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Deser_Init(SomeIp_DeserCursor_t *cur, const uint8_t *buf, uint32_t size,
                                  uint32_t msgOffset, SomeIp_ByteOrder_t order);
SomeIp_Result_t SomeIp_Deser_SetByteOrder(SomeIp_DeserCursor_t *cur, SomeIp_ByteOrder_t order);
SomeIp_Result_t SomeIp_Deser_GetStatus(const SomeIp_DeserCursor_t *cur);
uint32_t        SomeIp_Deser_GetRemaining(const SomeIp_DeserCursor_t *cur);

/* ---- Basic datatypes (Table 4.6) ------------------------------------------------------------------------ */
SomeIp_Result_t SomeIp_Deser_ReadBool(SomeIp_DeserCursor_t *cur, uint8_t *v);
SomeIp_Result_t SomeIp_Deser_ReadU8(SomeIp_DeserCursor_t *cur, uint8_t *v);
SomeIp_Result_t SomeIp_Deser_ReadU16(SomeIp_DeserCursor_t *cur, uint16_t *v);
SomeIp_Result_t SomeIp_Deser_ReadU32(SomeIp_DeserCursor_t *cur, uint32_t *v);
SomeIp_Result_t SomeIp_Deser_ReadU64(SomeIp_DeserCursor_t *cur, uint64_t *v);
SomeIp_Result_t SomeIp_Deser_ReadS8(SomeIp_DeserCursor_t *cur, int8_t *v);
SomeIp_Result_t SomeIp_Deser_ReadS16(SomeIp_DeserCursor_t *cur, int16_t *v);
SomeIp_Result_t SomeIp_Deser_ReadS32(SomeIp_DeserCursor_t *cur, int32_t *v);
SomeIp_Result_t SomeIp_Deser_ReadS64(SomeIp_DeserCursor_t *cur, int64_t *v);
SomeIp_Result_t SomeIp_Deser_ReadF32(SomeIp_DeserCursor_t *cur, float *v);
SomeIp_Result_t SomeIp_Deser_ReadF64(SomeIp_DeserCursor_t *cur, double *v);

/* ---- Raw bytes / alignment ------------------------------------------------------------------------------ */
SomeIp_Result_t SomeIp_Deser_ReadBytes(SomeIp_DeserCursor_t *cur, uint8_t *out, uint32_t len);
SomeIp_Result_t SomeIp_Deser_Skip(SomeIp_DeserCursor_t *cur, uint32_t len);
SomeIp_Result_t SomeIp_Deser_Align(SomeIp_DeserCursor_t *cur, uint16_t alignBits);

/* ---- Length field (struct, array) ----------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Deser_LengthBegin(SomeIp_DeserCursor_t *cur, SomeIp_LengthFieldSize_t lfSize,
                                         uint32_t *end);
SomeIp_Result_t SomeIp_Deser_LengthEnd(SomeIp_DeserCursor_t *cur, uint32_t end);

/* ---- Strings -------------------------------------------------------------------------------------------- */
SomeIp_Result_t SomeIp_Deser_ReadString(SomeIp_DeserCursor_t *cur, SomeIp_StringEncoding_t enc,
                                        SomeIp_LengthFieldSize_t lfSize, uint8_t *out, uint32_t outCap,
                                        uint32_t *outLen);
SomeIp_Result_t SomeIp_Deser_ReadStringFixed(SomeIp_DeserCursor_t *cur, SomeIp_StringEncoding_t enc,
                                             uint32_t fixedSize, uint8_t *out, uint32_t outCap,
                                             uint32_t *outLen);

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_SERIALIZATION_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
