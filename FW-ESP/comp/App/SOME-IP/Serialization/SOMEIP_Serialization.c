/**
 ************************************************************************************************************
 * @file    SOMEIP_Serialization.c
 * @brief   SOME/IP payload serialization / deserialization.
 *
 * @details This file provides the functions to:
 *          - Write / read basic datatypes with configurable byte order    [PRS_SOMEIP_00065] [PRS_SOMEIP_00369]
 *          - Insert / skip padding behind variable length p_data            [PRS_SOMEIP_00611] [PRS_SOMEIP_00569]
 *          - Write / read the length field of a struct or array           [PRS_SOMEIP_00370] [PRS_SOMEIP_00371]
 *                                                                         [PRS_SOMEIP_00376] [PRS_SOMEIP_00377]
 *          - Write / read fixed and dynamic length strings                [PRS_SOMEIP_00087] [PRS_SOMEIP_00089]
 *
 *          Every public function:
 *          1. returns p_cur->status at once if a previous call already failed (sticky status),
 *          2. on a new error stores it in p_cur->status and returns it,
 *          3. never moves p_cur->pos on error.
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.3 / 4.1.4 (4.1.4.1 .. 4.1.4.7)
 ************************************************************************************************************
 */

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include "SOMEIP_Serialization.h"
#include "SOMEIP_Endian.h"
#include "SOMEIP_Types.h"
#include <stdint.h>
#include <string.h>

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/* EXPORTED VARIABLES ****************************************************************************************
 ************************************************************************************************************/

/* PRIVATE VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* PRIVATE FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

static someip_result_t SOMEIP_Ser_Reserve(someip_ser_cursor_t *p_cur, uint32_t len);
static someip_result_t SOMEIP_Deser_Require(someip_deser_cursor_t *p_cur, uint32_t len);
static uint32_t SOMEIP_PaddingSize(uint32_t absPos, uint16_t alignBits);
static uint8_t SOMEIP_IsAlignValid(uint16_t alignBits);
static uint8_t SOMEIP_IsLengthFieldValid(someip_length_field_size_t lfSize);
static const uint8_t *SOMEIP_GetBom(someip_string_encoding_t enc, uint32_t *p_bom_size);
static uint32_t SOMEIP_GetTerminatorSize(someip_string_encoding_t enc);

/* PUBLIC VARIABLES ******************************************************************************************
 ************************************************************************************************************/

/* String Byte Order Marks                                                                [PRS_SOMEIP_00087] */
static const uint8_t SOMEIP_BomUtf8[SOMEIP_BOM_UTF8_SIZE] = {SOMEIP_BOM_UTF8_0, SOMEIP_BOM_UTF8_1,
                                                             SOMEIP_BOM_UTF8_2};
static const uint8_t SOMEIP_BomUtf16Be[SOMEIP_BOM_UTF16_SIZE] = {SOMEIP_BOM_UTF16BE_0,
                                                                 SOMEIP_BOM_UTF16BE_1};
static const uint8_t SOMEIP_BomUtf16Le[SOMEIP_BOM_UTF16_SIZE] = {SOMEIP_BOM_UTF16LE_0,
                                                                 SOMEIP_BOM_UTF16LE_1};

/* PUBLIC FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * PRIVATE HELPERS
 * ======================================================================================================== */

/**
 * @brief   Check that @p len more bytes can be written.
 *
 * @param[in,out] p_cur  Write cursor (not NULL, checked by the caller).
 * @param[in]     len  Number of bytes the caller wants to write.
 *
 * @retval SOMEIP_RES_OK         Enough space.
 * @retval SOMEIP_RES_E_NO_SPACE Buffer too small (or previous sticky error).
 */
static someip_result_t SOMEIP_Ser_Reserve(someip_ser_cursor_t *p_cur, uint32_t len) {
    /* Previous error: stop all following writes */
    if (p_cur->status != SOMEIP_RES_OK) {
        return p_cur->status;
    }

    /* Not enough space: keep the error */
    if ((p_cur->size - p_cur->pos) < len) {
        p_cur->status = SOMEIP_RES_E_NO_SPACE;
        return p_cur->status;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Check that @p len more bytes can be read.
 *
 * @param[in,out] p_cur  Read cursor (not NULL, checked by the caller).
 * @param[in]     len  Number of bytes the caller wants to read.
 *
 * @retval SOMEIP_RES_OK          Enough bytes.
 * @retval SOMEIP_RES_E_MALFORMED Not enough bytes (or previous sticky error).
 */
static someip_result_t SOMEIP_Deser_Require(someip_deser_cursor_t *p_cur, uint32_t len) {
    /* Previous error: stop all following reads */
    if (p_cur->status != SOMEIP_RES_OK) {
        return p_cur->status;
    }

    /* Not enough bytes: keep the error */
    if ((p_cur->size - p_cur->pos) < len) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;
        return p_cur->status;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Number of padding bytes needed so that the next element starts aligned.
 *
 * @param[in]  absPos     Position from the start of the SOME/IP message.
 * @param[in]  alignBits  8, 16, 32, 64, 128 or 256 (already checked by SOMEIP_IsAlignValid()).
 *
 * @return  Number of padding bytes (0 .. alignBits/8 - 1).
 */
static uint32_t SOMEIP_PaddingSize(uint32_t absPos, uint16_t alignBits) {
    uint32_t alignBytes;
    uint32_t remainder;

    /* 32 bit -> 4 byte */
    alignBytes = (uint32_t)alignBits / 8u;

    /* How many bytes past the last boundary */
    remainder = absPos % alignBytes;

    /* Already on a boundary: no padding */
    if (remainder == 0u) {
        return 0u;
    }

    /* Otherwise: fill up to the next boundary */
    return alignBytes - remainder;
}

/**
 * @brief   Check an alignment value.
 *
 * @details TODO: accept only 8, 16, 32, 64, 128, 256 [PRS_SOMEIP_00613].
 *
 * @param[in]  alignBits  Alignment in bits.
 *
 * @retval 1  Valid.
 * @retval 0  Invalid.
 */
static uint8_t SOMEIP_IsAlignValid(uint16_t alignBits) {
    uint8_t status;

    switch (alignBits) {
        case 8u:
        case 16u:
        case 32u:
        case 64u:
        case 128u:
        case 256u:
            status = 1u;
            break;

        default:
            status = 0u;
            break;
    }

    return status;
}

/**
 * @brief   Check a length field size for struct / array / string.
 *
 * @param[in]  lfSize  Length field size.
 *
 * @retval 1  Valid.
 * @retval 0  Invalid.
 *
 * @see     [PRS_SOMEIP_00079] [PRS_SOMEIP_00093] [PRS_SOMEIP_00107]
 */
static uint8_t SOMEIP_IsLengthFieldValid(someip_length_field_size_t lfSize) {
    uint8_t status;

    switch (lfSize) {
        case SOMEIP_LENGTH_FIELD_8BIT:
        case SOMEIP_LENGTH_FIELD_16BIT:
        case SOMEIP_LENGTH_FIELD_32BIT:
            status = 1u;
            break;

        case SOMEIP_LENGTH_FIELD_NONE: /* no length field: do not call LengthBegin/End */
        default:
            status = 0u;
            break;
    }

    return status;
}

/**
 * @brief   Get the Byte Order Mark of a string encoding.
 *
 * @param[in]  enc      String encoding.
 * @param[out] p_bom_size  Size of the BOM in bytes.
 *
 * @return  Pointer to the BOM bytes, or NULL.
 *
 * @see     [PRS_SOMEIP_00087]
 */
static const uint8_t *SOMEIP_GetBom(someip_string_encoding_t enc, uint32_t *p_bom_size) {
    const uint8_t *bom;

    switch (enc) {
        case SOMEIP_STRING_UTF8:
            bom = SOMEIP_BomUtf8;
            *p_bom_size = SOMEIP_BOM_UTF8_SIZE;
            break;

        case SOMEIP_STRING_UTF16BE:
            bom = SOMEIP_BomUtf16Be;
            *p_bom_size = SOMEIP_BOM_UTF16_SIZE;
            break;

        case SOMEIP_STRING_UTF16LE:
            bom = SOMEIP_BomUtf16Le;
            *p_bom_size = SOMEIP_BOM_UTF16_SIZE;
            break;

        default:
            /* Unknown encoding */
            bom = NULL;
            *p_bom_size = 0u;
            break;
    }

    return bom;
}

/**
 * @brief   Size of the "\0" terminator of a string encoding.
 *
 * @param[in]  enc  String encoding.
 *
 * @return  Terminator size in bytes.
 */
static uint32_t SOMEIP_GetTerminatorSize(someip_string_encoding_t enc) {
    uint32_t termSize;

    switch (enc) {
        case SOMEIP_STRING_UTF8:
            termSize = 1u;
            break;

        case SOMEIP_STRING_UTF16BE:
        case SOMEIP_STRING_UTF16LE:
            termSize = 2u;
            break;

        default:
            /* Unknown encoding */
            termSize = 0u;
            break;
    }

    return termSize;
}

/* ==========================================================================================================
 * SERIALIZER - CURSOR
 * ======================================================================================================== */

/**
 * @brief   Initialize a write cursor.
 *
 * @param[out] p_cur        Cursor to initialize.
 * @param[in]  p_buf        Output buffer.
 * @param[in]  size       Capacity of @p p_buf.
 * @param[in]  msgOffset  Offset of p_buf[0] from the start of the SOME/IP message, used for alignment
 *                        [PRS_SOMEIP_00569]. Use SOMEIP_OFS_PAYLOAD if @p p_buf is the payload.
 * @param[in]  order      Initial byte order [PRS_SOMEIP_00369].
 *
 * @retval SOMEIP_RES_OK       Cursor ready.
 * @retval SOMEIP_RES_E_PARAM  @p p_cur or @p p_buf is NULL.
 */
someip_result_t SOMEIP_Ser_Init(someip_ser_cursor_t *p_cur, uint8_t *p_buf, uint32_t size,
                                uint32_t msgOffset, someip_byte_order_t order) {
    /* Validate parameters */
    if ((p_cur == NULL) || (p_buf == NULL)) {
        return SOMEIP_RES_E_PARAM;
    }

    p_cur->p_buf = p_buf;
    p_cur->size = size;
    p_cur->pos = 0u;              /* nothing written yet                      */
    p_cur->msgOffset = msgOffset; /* alignment base       [PRS_SOMEIP_00569]  */
    p_cur->order = order;         /* default byte order   [PRS_SOMEIP_00369]  */
    p_cur->status = SOMEIP_RES_OK;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Change the byte order for the next parameters.
 *
 * @param[in,out] p_cur    Write cursor.
 * @param[in]     order  New byte order.
 *
 * @retval SOMEIP_RES_OK
 * @retval SOMEIP_RES_E_PARAM  @p p_cur is NULL.
 */
someip_result_t SOMEIP_Ser_SetByteOrder(someip_ser_cursor_t *p_cur, someip_byte_order_t order) {
    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    if ((order != SOMEIP_BYTE_ORDER_BIG_ENDIAN) && (order != SOMEIP_BYTE_ORDER_LITTLE_ENDIAN)) {
        p_cur->status = SOMEIP_RES_E_PARAM;
        return p_cur->status;
    }

    /* Byte order of the next parameters [PRS_SOMEIP_00369] */
    p_cur->order = order;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Get the sticky status of a write cursor.
 *
 * @param[in]  p_cur  Write cursor.
 *
 * @return  p_cur->status, or SOMEIP_RES_E_PARAM if @p p_cur is NULL.
 */
someip_result_t SOMEIP_Ser_GetStatus(const someip_ser_cursor_t *p_cur) {
    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* First error, SOMEIP_RES_OK while no error */
    return p_cur->status;
}

/**
 * @brief   Number of bytes written so far (= payload length for the header Length field).
 *
 * @param[in]  p_cur  Write cursor.
 *
 * @return  p_cur->pos, or 0 if @p p_cur is NULL.
 */
uint32_t SOMEIP_Ser_GetLength(const someip_ser_cursor_t *p_cur) {
    /* Validate parameters */
    if (p_cur == NULL) {
        return 0u;
    }

    /* Bytes written so far = next write position */
    return p_cur->pos;
}

/* ==========================================================================================================
 * SERIALIZER - BASIC DATATYPES                                              Table 4.6, [PRS_SOMEIP_00065]
 * ======================================================================================================== */

/**
 * @brief   Write a boolean (8 bit).
 *
 * @details TODO: write SOMEIP_BOOL_TRUE (0x01) if v != 0, else SOMEIP_BOOL_FALSE (0x00) -> WriteU8.
 *
 * @see     Table 4.6 "FALSE (0), TRUE (1)"
 */
someip_result_t SOMEIP_Ser_WriteBool(someip_ser_cursor_t *p_cur, uint8_t v) {
    uint8_t wireValue;

    if (v != 0u) {
        wireValue = SOMEIP_BOOL_TRUE;
    } else {
        wireValue = SOMEIP_BOOL_FALSE;
    }

    /* boolean = 8 bit, same as uint8 */
    return SOMEIP_Ser_WriteU8(p_cur, wireValue);
}

/**
 * @brief   Write a uint8.
 */
someip_result_t SOMEIP_Ser_WriteU8(someip_ser_cursor_t *p_cur, uint8_t v) {
    someip_result_t res;

    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 1 byte */
    res = SOMEIP_Ser_Reserve(p_cur, 1u);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* uint8: 1 byte, no byte order [Table 4.6] */
    p_cur->p_buf[p_cur->pos] = v;
    p_cur->pos = p_cur->pos + 1u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Write a uint16 in p_cur->order.
 *
 * @details Byte-wise access, so no alignment needed in p_buf.
 */
someip_result_t SOMEIP_Ser_WriteU16(someip_ser_cursor_t *p_cur, uint16_t v) {
    someip_result_t res;

    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 2 bytes */
    res = SOMEIP_Ser_Reserve(p_cur, 2u);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* uint16 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU16(&p_cur->p_buf[p_cur->pos], v, p_cur->order);
    p_cur->pos += 2u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Write a uint32 in p_cur->order.
 *
 */
someip_result_t SOMEIP_Ser_WriteU32(someip_ser_cursor_t *p_cur, uint32_t v) {
    someip_result_t res;

    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 4 bytes */
    res = SOMEIP_Ser_Reserve(p_cur, 4u);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* uint32 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU32(&p_cur->p_buf[p_cur->pos], v, p_cur->order);
    p_cur->pos += 4u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Write a uint64 in p_cur->order.
 *
 * @details TODO: same as WriteU16 with 8 bytes and SOMEIP_WriteU64().
 */
someip_result_t SOMEIP_Ser_WriteU64(someip_ser_cursor_t *p_cur, uint64_t v) {
    someip_result_t res;

    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 8 bytes */
    res = SOMEIP_Ser_Reserve(p_cur, 8u);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* uint64 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU64(&p_cur->p_buf[p_cur->pos], v, p_cur->order);
    p_cur->pos += 8u;

    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_Ser_WriteS8(someip_ser_cursor_t *p_cur, int8_t v) {
    return SOMEIP_Ser_WriteU8(p_cur, (uint8_t)v);
}

someip_result_t SOMEIP_Ser_WriteS16(someip_ser_cursor_t *p_cur, int16_t v) {
    return SOMEIP_Ser_WriteU16(p_cur, (uint16_t)v);
}

someip_result_t SOMEIP_Ser_WriteS32(someip_ser_cursor_t *p_cur, int32_t v) {
    return SOMEIP_Ser_WriteU32(p_cur, (uint32_t)v);
}

someip_result_t SOMEIP_Ser_WriteS64(someip_ser_cursor_t *p_cur, int64_t v) {
    return SOMEIP_Ser_WriteU64(p_cur, (uint64_t)v);
}

someip_result_t SOMEIP_Ser_WriteF32(someip_ser_cursor_t *p_cur, float v) {
    uint32_t raw;

    /* float32 = IEEE 754 binary32 bit pattern [Table 4.6] */
    (void)memcpy(&raw, &v, sizeof(raw));

    /* Same 4 bytes as uint32, in configured byte order [PRS_SOMEIP_00369] */
    return SOMEIP_Ser_WriteU32(p_cur, raw);
}

someip_result_t SOMEIP_Ser_WriteF64(someip_ser_cursor_t *p_cur, double v) {
    uint64_t raw;

    /* float64 = IEEE 754 binary64 bit pattern [Table 4.6] */
    (void)memcpy(&raw, &v, sizeof(raw));

    /* Same 8 bytes as uint64, in configured byte order [PRS_SOMEIP_00369] */
    return SOMEIP_Ser_WriteU64(p_cur, raw);
}

/* ==========================================================================================================
 * SERIALIZER - RAW BYTES / ALIGNMENT
 * ======================================================================================================== */

someip_result_t SOMEIP_Ser_WriteBytes(someip_ser_cursor_t *p_cur, const uint8_t *p_data, uint32_t len) {
    someip_result_t res;

    /* Validate parameters */
    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* Data may be NULL only for an emty array */
    if ((p_data == NULL) && (len != 0u)) {
        p_cur->status = SOMEIP_RES_E_PARAM;
        return p_cur->status;
    }

    /* Sticky status + enough space for len bytes */
    res = SOMEIP_Ser_Reserve(p_cur, len);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* Raw copy, no byte order [PRS_SOMEIP_00099] */
    if (len != 0u) {
        (void)memcpy(&p_cur->p_buf[p_cur->pos], p_data, len);
        p_cur->pos += len;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Insert padding so that the next element is aligned to @p alignBits.
 *
 * @param[in,out] p_cur        Write cursor.
 * @param[in]     alignBits  8, 16, 32, 64, 128 or 256.
 */
someip_result_t SOMEIP_Ser_Align(someip_ser_cursor_t *p_cur, uint16_t alignBits) {
    uint32_t n;
    someip_result_t res;

    if (p_cur == NULL) {
        return SOMEIP_RES_E_PARAM;
    }

    /* [PRS_SOMEIP_00613] */
    if (SOMEIP_IsAlignValid(alignBits) == 0u) {
        p_cur->status = SOMEIP_RES_E_PARAM;
        return p_cur->status;
    }

    /* [PRS_SOMEIP_00569] */
    n = SOMEIP_PaddingSize(p_cur->msgOffset + p_cur->pos, alignBits);

    res = SOMEIP_Ser_Reserve(p_cur, n);
    if (res != SOMEIP_RES_OK) {
        return res;
    }

    /* [PRS_SOMEIP_00611] */
    if (n != 0u) {
        (void)memset(&p_cur->p_buf[p_cur->pos], 0, n);
    }

    return SOMEIP_RES_OK;
}

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
