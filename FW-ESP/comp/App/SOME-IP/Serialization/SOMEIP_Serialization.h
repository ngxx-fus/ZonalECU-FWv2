/**
 ************************************************************************************************************
 * @file    SomeIp_Serialization.h
 * @brief   SOME/IP payload serialization / deserialization: public API.
 *
 * @details - Byte order per parameter                                     [PRS_SOMEIP_00369]
 *          - Basic datatypes (Table 4.6), boolean                         [PRS_SOMEIP_00065] [PRS_SOMEIP_00615]
 *          - Alignment / padding behind variable length p_data              [PRS_SOMEIP_00611] [PRS_SOMEIP_00569]
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

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_SERIALIZATION_H__
#define __SOMEIP_SERIALIZATION_H__

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include "Macros.h"
#include "SomeIp_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

ZE_HEADER

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/* TYPEDEFS **************************************************************************************************
 ************************************************************************************************************/

/* Write cursor (serializer) */
typedef struct {
    uint8_t *p_buf;            /* output buffer                                         */
    uint32_t size;             /* capacity of p_buf in bytes                              */
    uint32_t pos;              /* next byte to write = bytes written so far             */
    uint32_t msgOffset;        /* offset of p_buf[0] from the start of the SOME/IP message */
                               /* (SOMEIP_OFS_PAYLOAD if p_buf is the payload) [00569]    */
    someip_byte_order_t order; /* byte order of the next parameters        [00369]      */
    someip_result_t status;    /* first error, SOMEIP_RES_OK while no error             */
} someip_ser_cursor_t;

/* Read cursor (deserializer) */
typedef struct {
    const uint8_t *p_buf; /* received payload                                      */
    uint32_t size;        /* number of valid bytes in p_buf                          */
    uint32_t pos;         /* next byte to read                                     */
    uint32_t msgOffset;   /* offset of p_buf[0] from the start of the SOME/IP message */
    someip_byte_order_t order;
    someip_result_t status;
} someip_deser_cursor_t;

/* Backward-compatible typedef aliases */
typedef someip_ser_cursor_t someip_ser_cursor_t;
typedef someip_deser_cursor_t someip_deser_cursor_t;
typedef someip_result_t someip_result_t;
typedef someip_byte_order_t someip_byte_order_t;
typedef someip_length_field_size_t someip_length_field_size_t;
typedef someip_string_encoding_t someip_string_encoding_t;

/* PUBLIC FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * SERIALIZER
 * ======================================================================================================== */

/* ---- Cursor --------------------------------------------------------------------------------------------- */
someip_result_t SomeIp_Ser_Init(someip_ser_cursor_t *p_cur, uint8_t *p_buf, uint32_t size,
                                uint32_t msgOffset, someip_byte_order_t order);
someip_result_t SomeIp_Ser_SetByteOrder(someip_ser_cursor_t *p_cur, someip_byte_order_t order);
someip_result_t SomeIp_Ser_GetStatus(const someip_ser_cursor_t *p_cur);
uint32_t SomeIp_Ser_GetLength(const someip_ser_cursor_t *p_cur);

/* ---- Basic datatypes (Table 4.6) ------------------------------------------------------------------------ */
someip_result_t SomeIp_Ser_WriteBool(someip_ser_cursor_t *p_cur, uint8_t v);
someip_result_t SomeIp_Ser_WriteU8(someip_ser_cursor_t *p_cur, uint8_t v);
someip_result_t SomeIp_Ser_WriteU16(someip_ser_cursor_t *p_cur, uint16_t v);
someip_result_t SomeIp_Ser_WriteU32(someip_ser_cursor_t *p_cur, uint32_t v);
someip_result_t SomeIp_Ser_WriteU64(someip_ser_cursor_t *p_cur, uint64_t v);
someip_result_t SomeIp_Ser_WriteS8(someip_ser_cursor_t *p_cur, int8_t v);
someip_result_t SomeIp_Ser_WriteS16(someip_ser_cursor_t *p_cur, int16_t v);
someip_result_t SomeIp_Ser_WriteS32(someip_ser_cursor_t *p_cur, int32_t v);
someip_result_t SomeIp_Ser_WriteS64(someip_ser_cursor_t *p_cur, int64_t v);
someip_result_t SomeIp_Ser_WriteF32(someip_ser_cursor_t *p_cur, float v);
someip_result_t SomeIp_Ser_WriteF64(someip_ser_cursor_t *p_cur, double v);

/* ---- Raw bytes / alignment ------------------------------------------------------------------------------ */
someip_result_t SomeIp_Ser_WriteBytes(someip_ser_cursor_t *p_cur, const uint8_t *p_data, uint32_t len);
someip_result_t SomeIp_Ser_Align(someip_ser_cursor_t *p_cur, uint16_t alignBits);

/* ---- Length field (struct, array) ----------------------------------------------------------------------- */
someip_result_t SomeIp_Ser_LengthBegin(someip_ser_cursor_t *p_cur, someip_length_field_size_t lfSize,
                                       uint32_t *p_mark);
someip_result_t SomeIp_Ser_LengthEnd(someip_ser_cursor_t *p_cur, someip_length_field_size_t lfSize,
                                     uint32_t mark);

/* ---- Strings -------------------------------------------------------------------------------------------- */
someip_result_t SomeIp_Ser_WriteString(someip_ser_cursor_t *p_cur, someip_string_encoding_t enc,
                                       someip_length_field_size_t lfSize, const uint8_t *str, uint32_t len);
someip_result_t SomeIp_Ser_WriteStringFixed(someip_ser_cursor_t *p_cur, someip_string_encoding_t enc,
                                            uint32_t fixedSize, const uint8_t *str, uint32_t len);

/* ==========================================================================================================
 * DESERIALIZER
 * ======================================================================================================== */

/* ---- Cursor --------------------------------------------------------------------------------------------- */
someip_result_t SomeIp_Deser_Init(someip_deser_cursor_t *p_cur, const uint8_t *p_buf, uint32_t size,
                                  uint32_t msgOffset, someip_byte_order_t order);
someip_result_t SomeIp_Deser_SetByteOrder(someip_deser_cursor_t *p_cur, someip_byte_order_t order);
someip_result_t SomeIp_Deser_GetStatus(const someip_deser_cursor_t *p_cur);
uint32_t SomeIp_Deser_GetRemaining(const someip_deser_cursor_t *p_cur);

/* ---- Basic datatypes (Table 4.6) ------------------------------------------------------------------------ */
someip_result_t SomeIp_Deser_ReadBool(someip_deser_cursor_t *p_cur, uint8_t *p_val);
someip_result_t SomeIp_Deser_ReadU8(someip_deser_cursor_t *p_cur, uint8_t *p_val);
someip_result_t SomeIp_Deser_ReadU16(someip_deser_cursor_t *p_cur, uint16_t *p_val);
someip_result_t SomeIp_Deser_ReadU32(someip_deser_cursor_t *p_cur, uint32_t *p_val);
someip_result_t SomeIp_Deser_ReadU64(someip_deser_cursor_t *p_cur, uint64_t *p_val);
someip_result_t SomeIp_Deser_ReadS8(someip_deser_cursor_t *p_cur, int8_t *p_val);
someip_result_t SomeIp_Deser_ReadS16(someip_deser_cursor_t *p_cur, int16_t *p_val);
someip_result_t SomeIp_Deser_ReadS32(someip_deser_cursor_t *p_cur, int32_t *p_val);
someip_result_t SomeIp_Deser_ReadS64(someip_deser_cursor_t *p_cur, int64_t *p_val);
someip_result_t SomeIp_Deser_ReadF32(someip_deser_cursor_t *p_cur, float *p_val);
someip_result_t SomeIp_Deser_ReadF64(someip_deser_cursor_t *p_cur, double *p_val);

/* ---- Raw bytes / alignment ------------------------------------------------------------------------------ */
someip_result_t SomeIp_Deser_ReadBytes(someip_deser_cursor_t *p_cur, uint8_t *p_out, uint32_t len);
someip_result_t SomeIp_Deser_Skip(someip_deser_cursor_t *p_cur, uint32_t len);
someip_result_t SomeIp_Deser_Align(someip_deser_cursor_t *p_cur, uint16_t alignBits);

/* ---- Length field (struct, array) ----------------------------------------------------------------------- */
someip_result_t SomeIp_Deser_LengthBegin(someip_deser_cursor_t *p_cur, someip_length_field_size_t lfSize,
                                         uint32_t *end);
someip_result_t SomeIp_Deser_LengthEnd(someip_deser_cursor_t *p_cur, uint32_t end);

/* ---- Strings -------------------------------------------------------------------------------------------- */
someip_result_t SomeIp_Deser_ReadString(someip_deser_cursor_t *p_cur, someip_string_encoding_t enc,
                                        someip_length_field_size_t lfSize, uint8_t *p_out, uint32_t outCap,
                                        uint32_t *outLen);
someip_result_t SomeIp_Deser_ReadStringFixed(someip_deser_cursor_t *p_cur, someip_string_encoding_t enc,
                                             uint32_t fixedSize, uint8_t *p_out, uint32_t outCap,
                                             uint32_t *outLen);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_SERIALIZATION_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
