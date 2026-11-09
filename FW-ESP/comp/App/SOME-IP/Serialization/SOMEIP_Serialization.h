

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_SERIALIZATION_H__
#define __SOMEIP_SERIALIZATION_H__

/** @brief SOME/IP payload serialization / deserialization: public API (chapter 4.1.3 / 4.1.4)
 *  @details - Byte order per parameter [PRS_SOMEIP_00369]
 *           - Basic datatypes (Table 4.6), boolean [PRS_SOMEIP_00065] [PRS_SOMEIP_00615]
 *           - Alignment / padding behind variable length elements [00611] [00569] [00612] [00613]
 *           - Length field of struct / array [00079] [00370] [00371] [00900] [00376] [00377]
 *           - Strings (fixed / dynamic length, BOM, "\0") [PRS_SOMEIP_00087] [PRS_SOMEIP_00089]
 *           - Enumeration / bitfield: use the unsigned integer functions [00705] [00300]
 *  @note    One cursor per payload. Every function stops on the first error and keeps it in the cursor
 *           (sticky status), so a sequence of calls can be checked once at the end. On error the position
 *           is never moved.
 *  @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.4.1 .. 4.1.4.7 */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/** @brief Allowed alignment of the element behind a variable length element, in bits [PRS_SOMEIP_00613] */
enum e_someip_align {
    SOMEIP_ALIGN_8BIT = 8,
    SOMEIP_ALIGN_16BIT = 16,
    SOMEIP_ALIGN_32BIT = 32,
    SOMEIP_ALIGN_64BIT = 64,
    SOMEIP_ALIGN_128BIT = 128,
    SOMEIP_ALIGN_256BIT = 256,
};

/** @brief Write cursor (serializer) */
typedef struct st_someip_ser_cursor {
    uint8_t *p_buf;
    uint32_t buf_size;         /*!< Capacity of p_buf in bytes */
    uint32_t pos;              /*!< Next byte to write = bytes written so far */
    uint32_t msg_offset;       /*!< p_buf[0] offset in the SOME/IP message [00569] */
    someip_byte_order_t order; /*!< Byte order of the next parameters [00369] */
    someip_result_t status;    /*!< First error, SOMEIP_RES_OK while no error */
} someip_ser_cursor_t;

/** @brief Read cursor (deserializer) */
typedef struct st_someip_deser_cursor {
    uint8_t const *p_buf; /*!< Received payload */
    uint32_t buf_size;    /*!< Number of valid bytes in p_buf */
    uint32_t pos;         /*!< Next byte to read */
    uint32_t msg_offset;  /*!< p_buf[0] offset in the SOME/IP message [00569] */
    someip_byte_order_t order;
    someip_result_t status;
} someip_deser_cursor_t;

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/* Serializer: cursor */

/** @brief Initialize a write cursor
 *  @param p_cur      Cursor to initialize
 *  @param p_buf      Output buffer
 *  @param buf_size   Capacity of p_buf in bytes
 *  @param msg_offset Offset of p_buf[0] from the start of the SOME/IP message, base of the alignment
 *                    [PRS_SOMEIP_00569]; SOMEIP_OFS_PAYLOAD if p_buf is the payload
 *  @param order      Initial byte order [PRS_SOMEIP_00369]
 *  @retval SOMEIP_RES_OK      Cursor ready
 *  @retval SOMEIP_RES_E_PARAM p_cur or p_buf is NULL */
someip_result_t SOMEIP_SerInit(OUT someip_ser_cursor_t *p_cur, OUT uint8_t *p_buf, IN uint32_t buf_size,
                               IN uint32_t msg_offset, IN someip_byte_order_t order);

/** @brief Change the byte order of the next parameters [PRS_SOMEIP_00369]
 *  @param p_cur Write cursor
 *  @param order New byte order
 *  @retval SOMEIP_RES_OK      Byte order changed
 *  @retval SOMEIP_RES_E_PARAM p_cur is NULL, or unknown byte order (stored as sticky error)
 *  @retval Other              Previous sticky error, byte order not changed */
someip_result_t SOMEIP_SerSetByteOrder(INOUT someip_ser_cursor_t *p_cur, IN someip_byte_order_t order);

/** @brief Get the sticky status of a write cursor
 *  @param p_cur Write cursor
 *  @return p_cur->status, or SOMEIP_RES_E_PARAM if p_cur is NULL */
someip_result_t SOMEIP_SerGetStatus(IN someip_ser_cursor_t *p_cur);

/** @brief Get the number of bytes written so far (= payload length for the header Length field)
 *  @param p_cur Write cursor
 *  @return p_cur->pos, or 0 if p_cur is NULL */
uint32_t SOMEIP_SerGetLength(IN someip_ser_cursor_t *p_cur);

/* Serializer: basic datatypes (Table 4.6) [PRS_SOMEIP_00065] */

/** @brief Write a boolean: 8 bit, SOMEIP_BOOL_TRUE if value != 0, else SOMEIP_BOOL_FALSE (Table 4.6)
 *  @param p_cur Write cursor
 *  @param value 0 = FALSE, any other value = TRUE
 *  @return SOMEIP_RES_OK, or the error of SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteBool(INOUT someip_ser_cursor_t *p_cur, IN uint8_t value);

/** @brief Write a uint8 (no byte order)
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @retval SOMEIP_RES_OK         Written
 *  @retval SOMEIP_RES_E_PARAM    p_cur is NULL
 *  @retval SOMEIP_RES_E_NO_SPACE Buffer full, or a previous sticky error */
someip_result_t SOMEIP_SerWriteU8(INOUT someip_ser_cursor_t *p_cur, IN uint8_t value);

/** @brief Write a uint16 in p_cur->order; byte-wise access, no alignment needed in p_buf
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteU16(INOUT someip_ser_cursor_t *p_cur, IN uint16_t value);

/** @brief Write a uint32 in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteU32(INOUT someip_ser_cursor_t *p_cur, IN uint32_t value);

/** @brief Write a uint64 in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteU64(INOUT someip_ser_cursor_t *p_cur, IN uint64_t value);

/** @brief Write a sint8 (two's complement, same bytes as uint8)
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteS8(INOUT someip_ser_cursor_t *p_cur, IN int8_t value);

/** @brief Write a sint16 in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteS16(INOUT someip_ser_cursor_t *p_cur, IN int16_t value);

/** @brief Write a sint32 in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteS32(INOUT someip_ser_cursor_t *p_cur, IN int32_t value);

/** @brief Write a sint64 in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteS64(INOUT someip_ser_cursor_t *p_cur, IN int64_t value);

/** @brief Write a float32 (IEEE 754 binary32 bit pattern) in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteF32(INOUT someip_ser_cursor_t *p_cur, IN float value);

/** @brief Write a float64 (IEEE 754 binary64 bit pattern) in p_cur->order
 *  @param p_cur Write cursor
 *  @param value Value to write
 *  @return Same as SOMEIP_SerWriteU8() */
someip_result_t SOMEIP_SerWriteF64(INOUT someip_ser_cursor_t *p_cur, IN double value);

/* Serializer: raw bytes / alignment */

/** @brief Copy raw bytes, no byte order
 *  @param p_cur    Write cursor
 *  @param p_data   Bytes to copy, may be NULL only if data_len is 0 (empty array)
 *  @param data_len Number of bytes
 *  @retval SOMEIP_RES_OK         Written
 *  @retval SOMEIP_RES_E_PARAM    p_cur is NULL, or p_data is NULL with data_len != 0
 *  @retval SOMEIP_RES_E_NO_SPACE Buffer too small, or a previous sticky error */
someip_result_t SOMEIP_SerWriteBytes(INOUT someip_ser_cursor_t *p_cur, IN uint8_t *p_data,
                                     IN uint32_t data_len);

/** @brief Insert 0x00 padding so that the next element starts aligned to align_bits
 *  @note  Call it ONLY behind a variable length element that is not the last element [PRS_SOMEIP_00611]
 *         [PRS_SOMEIP_00612]; the serializer never pads by itself [PRS_SOMEIP_00077]. The alignment is
 *         computed from the start of the SOME/IP message [PRS_SOMEIP_00569]. The padding value is not
 *         defined by the spec, 0x00 is used.
 *  @param p_cur      Write cursor
 *  @param align_bits One of enum e_someip_align (8, 16, 32, 64, 128, 256) [PRS_SOMEIP_00613]
 *  @retval SOMEIP_RES_OK         Padding written (0 bytes if already aligned)
 *  @retval SOMEIP_RES_E_PARAM    p_cur is NULL, or invalid align_bits
 *  @retval SOMEIP_RES_E_NO_SPACE Buffer too small, or a previous sticky error */
someip_result_t SOMEIP_SerAlign(INOUT someip_ser_cursor_t *p_cur, IN uint16_t align_bits);

/* Serializer: length field (struct, array) [PRS_SOMEIP_00079] [PRS_SOMEIP_00370] [PRS_SOMEIP_00376] */

/** @brief Reserve a length field in front of a struct / dynamic array, the value is written by LengthEnd
 *  @details Usage:
 *           @verbatim
 *           uint32_t mark = 0U;
 *           SOMEIP_SerLengthBegin(&cursor, SOMEIP_LENGTH_FIELD_32BIT, &mark);
 *           ... write members / elements ...
 *           SOMEIP_SerLengthEnd(&cursor, SOMEIP_LENGTH_FIELD_32BIT, mark);
 *           @endverbatim
 *  @param p_cur   Write cursor
 *  @param lf_size 8, 16 or 32 bit
 *  @param p_mark  Position of the length field, to pass to SOMEIP_SerLengthEnd()
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_NO_SPACE */
someip_result_t SOMEIP_SerLengthBegin(INOUT someip_ser_cursor_t *p_cur,
                                      IN someip_length_field_size_t lf_size, OUT uint32_t *p_mark);

/** @brief Fill the length field reserved by SOMEIP_SerLengthBegin()
 *  @note  The length counts the bytes AFTER the length field, not the length field itself
 *         [PRS_SOMEIP_00370] [PRS_SOMEIP_00377].
 *  @param p_cur   Write cursor
 *  @param lf_size Same value as passed to SOMEIP_SerLengthBegin()
 *  @param mark    Value returned by SOMEIP_SerLengthBegin()
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_NO_SPACE (length does not fit in lf_size) */
someip_result_t SOMEIP_SerLengthEnd(INOUT someip_ser_cursor_t *p_cur, IN someip_length_field_size_t lf_size,
                                    IN uint32_t mark);

/* Serializer: strings (chapter 4.1.4.4) */

/** @brief Write a dynamic length string: [length field][BOM][characters]["\0"]
 *  @param p_cur   Write cursor
 *  @param enc     UTF-8, UTF-16BE or UTF-16LE [PRS_SOMEIP_00372]
 *  @param lf_size 8, 16 or 32 bit (default 32) [PRS_SOMEIP_00094]
 *  @param p_str   Characters already encoded in enc, WITHOUT BOM and WITHOUT "\0"
 *  @param str_len Size of p_str in bytes, even for UTF-16; p_str may be NULL only if str_len is 0
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_NO_SPACE */
someip_result_t SOMEIP_SerWriteString(INOUT someip_ser_cursor_t *p_cur, IN someip_string_encoding_t enc,
                                      IN someip_length_field_size_t lf_size, IN uint8_t *p_str,
                                      IN uint32_t str_len);

/** @brief Write a fixed length string: [BOM][characters]["\0"][0x00 fill] = exactly fixed_size bytes
 *  @note  fixed_size comes from the type definition and includes BOM and "\0"
 *         [PRS_SOMEIP_00374] [PRS_SOMEIP_00087].
 *  @param p_cur      Write cursor
 *  @param enc        String encoding
 *  @param fixed_size Size on the wire in bytes, including BOM and "\0", even for UTF-16
 *  @param p_str      Characters WITHOUT BOM and WITHOUT "\0"
 *  @param str_len    Size of p_str in bytes, even for UTF-16; p_str may be NULL only if str_len is 0
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM (string too long) or SOMEIP_RES_E_NO_SPACE */
someip_result_t SOMEIP_SerWriteStringFixed(INOUT someip_ser_cursor_t *p_cur,
                                           IN someip_string_encoding_t enc, IN uint32_t fixed_size,
                                           IN uint8_t *p_str, IN uint32_t str_len);

/* Deserializer: cursor */

/** @brief Initialize a read cursor
 *  @details Typical call:
 *           SOMEIP_DeserInit(&cursor, msg.p_payload, msg.payload_len, SOMEIP_OFS_PAYLOAD, order)
 *  @param p_cur      Cursor to initialize
 *  @param p_buf      Received payload, may be NULL only if buf_size is 0
 *  @param buf_size   Number of valid bytes in p_buf
 *  @param msg_offset Offset of p_buf[0] from the start of the SOME/IP message [PRS_SOMEIP_00569]
 *  @param order      Initial byte order [PRS_SOMEIP_00369]
 *  @return SOMEIP_RES_OK or SOMEIP_RES_E_PARAM */
someip_result_t SOMEIP_DeserInit(OUT someip_deser_cursor_t *p_cur, IN uint8_t *p_buf, IN uint32_t buf_size,
                                 IN uint32_t msg_offset, IN someip_byte_order_t order);

/** @brief Change the byte order of the next parameters [PRS_SOMEIP_00369]
 *  @param p_cur Read cursor
 *  @param order New byte order
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or the previous sticky error */
someip_result_t SOMEIP_DeserSetByteOrder(INOUT someip_deser_cursor_t *p_cur, IN someip_byte_order_t order);

/** @brief Get the sticky status of a read cursor
 *  @note  The server maps SOMEIP_RES_E_MALFORMED to the Return Code SOMEIP_E_MALFORMED_MESSAGE
 *         (Table 4.11) and answers with SOMEIP_Error_BuildHeader().
 *  @param p_cur Read cursor
 *  @return p_cur->status, or SOMEIP_RES_E_PARAM if p_cur is NULL */
someip_result_t SOMEIP_DeserGetStatus(IN someip_deser_cursor_t *p_cur);

/** @brief Get the number of bytes not read yet
 *  @param p_cur Read cursor
 *  @return p_cur->buf_size - p_cur->pos, or 0 if p_cur is NULL */
uint32_t SOMEIP_DeserGetRemaining(IN someip_deser_cursor_t *p_cur);

/* Deserializer: basic datatypes (Table 4.6) [PRS_SOMEIP_00065] */

/** @brief Read a boolean: only the lowest bit is evaluated, 0xFE reads as FALSE, 0x03 as TRUE
 *         [PRS_SOMEIP_00615]
 *  @param p_cur   Read cursor
 *  @param p_value SOMEIP_BOOL_FALSE or SOMEIP_BOOL_TRUE
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED (not enough bytes) */
someip_result_t SOMEIP_DeserReadBool(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_value);

/** @brief Read a uint8
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED (not enough bytes) */
someip_result_t SOMEIP_DeserReadU8(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_value);

/** @brief Read a uint16 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadU16(INOUT someip_deser_cursor_t *p_cur, OUT uint16_t *p_value);

/** @brief Read a uint32 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadU32(INOUT someip_deser_cursor_t *p_cur, OUT uint32_t *p_value);

/** @brief Read a uint64 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadU64(INOUT someip_deser_cursor_t *p_cur, OUT uint64_t *p_value);

/** @brief Read a sint8
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadS8(INOUT someip_deser_cursor_t *p_cur, OUT int8_t *p_value);

/** @brief Read a sint16 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadS16(INOUT someip_deser_cursor_t *p_cur, OUT int16_t *p_value);

/** @brief Read a sint32 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadS32(INOUT someip_deser_cursor_t *p_cur, OUT int32_t *p_value);

/** @brief Read a sint64 in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadS64(INOUT someip_deser_cursor_t *p_cur, OUT int64_t *p_value);

/** @brief Read a float32 (IEEE 754 binary32) in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadF32(INOUT someip_deser_cursor_t *p_cur, OUT float *p_value);

/** @brief Read a float64 (IEEE 754 binary64) in p_cur->order
 *  @param p_cur   Read cursor
 *  @param p_value Read value
 *  @return Same as SOMEIP_DeserReadU8() */
someip_result_t SOMEIP_DeserReadF64(INOUT someip_deser_cursor_t *p_cur, OUT double *p_value);

/* Deserializer: raw bytes / alignment */

/** @brief Read raw bytes, no byte order
 *  @param p_cur    Read cursor
 *  @param p_out    Destination, at least data_len bytes
 *  @param data_len Number of bytes to read
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED (not enough bytes) */
someip_result_t SOMEIP_DeserReadBytes(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_out,
                                      IN uint32_t data_len);

/** @brief Skip bytes (unknown trailing elements, padding)
 *  @param p_cur    Read cursor
 *  @param skip_len Number of bytes to skip
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED (not enough bytes) */
someip_result_t SOMEIP_DeserSkip(INOUT someip_deser_cursor_t *p_cur, IN uint32_t skip_len);

/** @brief Skip the padding inserted by the sender behind a variable length element
 *  @note  Same computation as SOMEIP_SerAlign(). The padding value is not checked, it is not defined by the
 *         spec (note of [PRS_SOMEIP_00611]). Padding at the end of the payload may be missing ("not the last
 *         element"), so at most the remaining bytes are skipped and no error is reported.
 *  @param p_cur      Read cursor
 *  @param align_bits One of enum e_someip_align [PRS_SOMEIP_00613]
 *  @return SOMEIP_RES_OK or SOMEIP_RES_E_PARAM */
someip_result_t SOMEIP_DeserAlign(INOUT someip_deser_cursor_t *p_cur, IN uint16_t align_bits);

/* Deserializer: length field (struct, array) [PRS_SOMEIP_00370] [PRS_SOMEIP_00371] [PRS_SOMEIP_00900] */

/** @brief Read the length field of a struct / array and compute where it ends
 *  @param p_cur   Read cursor
 *  @param lf_size 8, 16 or 32 bit
 *  @param p_end   Position right after the struct / array
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED (length points outside the payload) */
someip_result_t SOMEIP_DeserLengthBegin(INOUT someip_deser_cursor_t *p_cur,
                                        IN someip_length_field_size_t lf_size, OUT uint32_t *p_end);

/** @brief Close a struct / array opened by SOMEIP_DeserLengthBegin()
 *  @details - pos >  end: SOMEIP_RES_E_MALFORMED, members read past the length [PRS_SOMEIP_00900] [00918]
 *           - pos <= end: pos = end, unknown extra bytes are skipped [PRS_SOMEIP_00371] [00917] [00919]
 *  @param p_cur Read cursor
 *  @param end   Value returned by SOMEIP_DeserLengthBegin()
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED */
someip_result_t SOMEIP_DeserLengthEnd(INOUT someip_deser_cursor_t *p_cur, IN uint32_t end);

/* Deserializer: strings (chapter 4.1.4.4) */

/** @brief Read a dynamic length string into p_out (characters only, without BOM and "\0")
 *  @param p_cur     Read cursor
 *  @param enc       Expected encoding
 *  @param lf_size   8, 16 or 32 bit
 *  @param p_out     Buffer for the characters, may be NULL only if out_size is 0
 *  @param out_size  Capacity of p_out = maximum number of character bytes, without BOM and "\0"
 *  @param p_out_len Number of bytes written to p_out
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM or SOMEIP_RES_E_MALFORMED */
someip_result_t SOMEIP_DeserReadString(INOUT someip_deser_cursor_t *p_cur, IN someip_string_encoding_t enc,
                                       IN someip_length_field_size_t lf_size, OUT uint8_t *p_out,
                                       IN uint32_t out_size, OUT uint32_t *p_out_len);

/** @brief Read a fixed length string of fixed_size bytes into p_out (characters only)
 *  @param p_cur      Read cursor
 *  @param enc        Expected encoding
 *  @param fixed_size Size on the wire in bytes, including BOM and "\0", even for UTF-16 [PRS_SOMEIP_00374]
 *  @param p_out      Buffer for the characters, may be NULL only if out_size is 0
 *  @param out_size   Capacity of p_out = maximum number of character bytes, without BOM and "\0"
 *  @param p_out_len  Number of bytes written to p_out
 *  @return SOMEIP_RES_OK, SOMEIP_RES_E_PARAM (also invalid fixed_size) or SOMEIP_RES_E_MALFORMED */
someip_result_t SOMEIP_DeserReadStringFixed(INOUT someip_deser_cursor_t *p_cur,
                                            IN someip_string_encoding_t enc, IN uint32_t fixed_size,
                                            OUT uint8_t *p_out, IN uint32_t out_size,
                                            OUT uint32_t *p_out_len);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_SERIALIZATION_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
