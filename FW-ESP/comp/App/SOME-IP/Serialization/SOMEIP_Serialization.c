

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "SOMEIP_Serialization.h"
#include "SOMEIP_Endian.h"
#include "SOMEIP_Types.h"

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

#define SOMEIP_U8_SIZE       (1U)
#define SOMEIP_BITS_PER_BYTE (8U)

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/** @brief Size of the "\0" terminator per string encoding [PRS_SOMEIP_00948] [PRS_SOMEIP_00084] */
enum e_someip_str_term_size {
    SOMEIP_STR_TERM_NONE_SIZE = 0,
    SOMEIP_STR_TERM_UTF8_SIZE = 1,
    SOMEIP_STR_TERM_UTF16_SIZE = 2,
};

/* PRIVATE VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* String Byte Order Marks [PRS_SOMEIP_00087] */
static const uint8_t s_bom_utf8[SOMEIP_BOM_UTF8_SIZE] = {SOMEIP_BOM_UTF8_0, SOMEIP_BOM_UTF8_1,
                                                         SOMEIP_BOM_UTF8_2};
static const uint8_t s_bom_utf16_be[SOMEIP_BOM_UTF16_SIZE] = {SOMEIP_BOM_UTF16BE_0, SOMEIP_BOM_UTF16BE_1};
static const uint8_t s_bom_utf16_le[SOMEIP_BOM_UTF16_SIZE] = {SOMEIP_BOM_UTF16LE_0, SOMEIP_BOM_UTF16LE_1};

/* PRIVATE FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

static someip_result_t prv_ser_reserve(INOUT someip_ser_cursor_t *p_cur, IN uint32_t len);
static someip_result_t prv_deser_require(INOUT someip_deser_cursor_t *p_cur, IN uint32_t len);
static uint32_t prv_padding_size(IN uint32_t abs_pos, IN uint16_t align_bits);
static bool prv_is_align_valid(IN uint16_t align_bits);
static bool prv_is_length_field_valid(IN someip_length_field_size_t lf_size);
static uint8_t const *prv_get_bom(IN someip_string_encoding_t enc, OUT uint32_t *p_bom_size);
static uint32_t prv_get_terminator_size(IN someip_string_encoding_t enc);

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

/* Private helpers */

/** @brief Check that len more bytes can be written
 *  @param p_cur Write cursor, not NULL (checked by the caller)
 *  @param len   Number of bytes the caller wants to write
 *  @retval SOMEIP_RES_OK         Enough space
 *  @retval SOMEIP_RES_E_NO_SPACE Buffer too small, or a previous sticky error */
static someip_result_t prv_ser_reserve(INOUT someip_ser_cursor_t *p_cur, IN uint32_t len) {
    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Not enough space: keep the error */
    if ((p_cur->buf_size - p_cur->pos) < len) {
        p_cur->status = SOMEIP_RES_E_NO_SPACE;

        /* Report the new sticky error */
        return p_cur->status;
    }

    /* Enough space */
    return SOMEIP_RES_OK;
}

/** @brief Check that len more bytes can be read
 *  @param p_cur Read cursor, not NULL (checked by the caller)
 *  @param len   Number of bytes the caller wants to read
 *  @retval SOMEIP_RES_OK          Enough bytes
 *  @retval SOMEIP_RES_E_MALFORMED Not enough bytes, or a previous sticky error */
static someip_result_t prv_deser_require(INOUT someip_deser_cursor_t *p_cur, IN uint32_t len) {
    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Not enough bytes: keep the error */
    if ((p_cur->buf_size - p_cur->pos) < len) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* Report the new sticky error */
        return p_cur->status;
    }

    /* Enough bytes */
    return SOMEIP_RES_OK;
}

/** @brief Get the number of padding bytes so that the next element starts aligned
 *  @param abs_pos    Position from the start of the SOME/IP message [PRS_SOMEIP_00569]
 *  @param align_bits Valid alignment (already checked by prv_is_align_valid())
 *  @return Number of padding bytes, 0 .. align_bits / 8 - 1 */
static uint32_t prv_padding_size(IN uint32_t abs_pos, IN uint16_t align_bits) {
    uint32_t align_bytes = 0U;
    uint32_t remainder = 0U;

    /* 32 bit -> 4 byte */
    align_bytes = (uint32_t)align_bits / SOMEIP_BITS_PER_BYTE;

    /* How many bytes past the last boundary */
    remainder = abs_pos % align_bytes;

    /* Already on a boundary: no padding */
    if (0U == remainder) {
        /* No padding */
        return 0U;
    }

    /* Otherwise: fill up to the next boundary */
    return align_bytes - remainder;
}

/** @brief Check an alignment value: 8, 16, 32, 64, 128 or 256 bit [PRS_SOMEIP_00613]
 *  @param align_bits Alignment in bits
 *  @retval true  Valid
 *  @retval false Invalid */
static bool prv_is_align_valid(IN uint16_t align_bits) {
    bool status = false;

    /* Accept only 8, 16, 32, 64, 128, 256 [PRS_SOMEIP_00613] */
    switch (align_bits) {
        case SOMEIP_ALIGN_8BIT:
        case SOMEIP_ALIGN_16BIT:
        case SOMEIP_ALIGN_32BIT:
        case SOMEIP_ALIGN_64BIT:
        case SOMEIP_ALIGN_128BIT:
        case SOMEIP_ALIGN_256BIT:
            status = true;

            /* Valid alignment */
            break;

        default:
            status = false;

            /* Invalid alignment */
            break;
    }

    /* Result of the check */
    return status;
}

/** @brief Check a length field size of struct / array / string
 *  @note  SOMEIP_LENGTH_FIELD_NONE is invalid here: without length field, do not call LengthBegin / End.
 *  @param lf_size Length field size
 *  @retval true  Valid (8, 16 or 32 bit)
 *  @retval false Invalid
 *  @see   [PRS_SOMEIP_00079] [PRS_SOMEIP_00093] [PRS_SOMEIP_00107] */
static bool prv_is_length_field_valid(IN someip_length_field_size_t lf_size) {
    bool status = false;

    /* Accept only 8, 16, 32 bit length fields */
    switch (lf_size) {
        case SOMEIP_LENGTH_FIELD_8BIT:
        case SOMEIP_LENGTH_FIELD_16BIT:
        case SOMEIP_LENGTH_FIELD_32BIT:
            status = true;

            /* Valid length field size */
            break;

        case SOMEIP_LENGTH_FIELD_NONE: /* no length field: do not call LengthBegin/End */
        default:
            status = false;

            /* Invalid length field size */
            break;
    }

    /* Result of the check */
    return status;
}

/** @brief Get the Byte Order Mark of a string encoding [PRS_SOMEIP_00087]
 *  @param enc        String encoding
 *  @param p_bom_size Size of the BOM in bytes, 0 for an unknown encoding
 *  @return Pointer to the BOM bytes, or NULL for an unknown encoding */
static uint8_t const *prv_get_bom(IN someip_string_encoding_t enc, OUT uint32_t *p_bom_size) {
    uint8_t const *p_bom = NULL;

    /* Select the BOM of the encoding */
    switch (enc) {
        case SOMEIP_STRING_UTF8:
            p_bom = s_bom_utf8;
            *p_bom_size = SOMEIP_BOM_UTF8_SIZE;

            /* UTF-8 BOM selected */
            break;

        case SOMEIP_STRING_UTF16BE:
            p_bom = s_bom_utf16_be;
            *p_bom_size = SOMEIP_BOM_UTF16_SIZE;

            /* UTF-16BE BOM selected */
            break;

        case SOMEIP_STRING_UTF16LE:
            p_bom = s_bom_utf16_le;
            *p_bom_size = SOMEIP_BOM_UTF16_SIZE;

            /* UTF-16LE BOM selected */
            break;

        default:
            /* Unknown encoding */
            p_bom = NULL;
            *p_bom_size = 0U;

            /* No BOM */
            break;
    }

    /* Selected BOM, or NULL */
    return p_bom;
}

/** @brief Get the size of the "\0" terminator of a string encoding [PRS_SOMEIP_00948] [PRS_SOMEIP_00084]
 *  @param enc String encoding
 *  @return Terminator size in bytes, 0 for an unknown encoding */
static uint32_t prv_get_terminator_size(IN someip_string_encoding_t enc) {
    uint32_t term_size = 0U;

    /* Select the terminator size of the encoding */
    switch (enc) {
        case SOMEIP_STRING_UTF8:
            /* 1 byte */
            term_size = SOMEIP_STR_TERM_UTF8_SIZE;

            /* UTF-8 terminator selected */
            break;

        case SOMEIP_STRING_UTF16BE:
        case SOMEIP_STRING_UTF16LE:
            /* 2 bytes */
            term_size = SOMEIP_STR_TERM_UTF16_SIZE;

            /* UTF-16 terminator selected */
            break;

        default:
            /* Unknown encoding */
            term_size = SOMEIP_STR_TERM_NONE_SIZE;

            /* No terminator */
            break;
    }

    /* Terminator size in bytes */
    return term_size;
}

/* Serializer: cursor */

someip_result_t SOMEIP_SerInit(OUT someip_ser_cursor_t *p_cur, OUT uint8_t *p_buf, IN uint32_t buf_size,
                               IN uint32_t msg_offset, IN someip_byte_order_t order) {
    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_buf)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    p_cur->p_buf = p_buf;
    p_cur->buf_size = buf_size;
    p_cur->pos = 0U;                /* nothing written yet                      */
    p_cur->msg_offset = msg_offset; /* alignment base       [PRS_SOMEIP_00569]  */
    p_cur->order = order;           /* default byte order   [PRS_SOMEIP_00369]  */
    p_cur->status = SOMEIP_RES_OK;

    /* Cursor ready */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerSetByteOrder(INOUT someip_ser_cursor_t *p_cur, IN someip_byte_order_t order) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Validate byte order */
    if ((SOMEIP_BYTE_ORDER_BIG_ENDIAN != order) && (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN != order)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid byte order */
        return p_cur->status;
    }

    /* Byte order of the next parameters [PRS_SOMEIP_00369] */
    p_cur->order = order;

    /* Byte order changed */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerGetStatus(IN someip_ser_cursor_t *p_cur) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* No cursor */
        return SOMEIP_RES_E_PARAM;
    }

    /* First error, SOMEIP_RES_OK while no error */
    return p_cur->status;
}

uint32_t SOMEIP_SerGetLength(IN someip_ser_cursor_t *p_cur) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* No cursor: nothing written */
        return 0U;
    }

    /* Bytes written so far = next write position */
    return p_cur->pos;
}

/* Serializer: basic datatypes (Table 4.6) [PRS_SOMEIP_00065] */

someip_result_t SOMEIP_SerWriteBool(INOUT someip_ser_cursor_t *p_cur, IN uint8_t value) {
    uint8_t wire_value = 0U;

    /* Any non-zero value is TRUE */
    if (0U != value) {
        wire_value = SOMEIP_BOOL_TRUE;
    }

    /* Zero is FALSE */
    else {
        wire_value = SOMEIP_BOOL_FALSE;
    }

    /* boolean = 8 bit, same as uint8 */
    return SOMEIP_SerWriteU8(p_cur, wire_value);
}

someip_result_t SOMEIP_SerWriteU8(INOUT someip_ser_cursor_t *p_cur, IN uint8_t value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 1 byte */
    res = prv_ser_reserve(p_cur, SOMEIP_U8_SIZE);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint8: 1 byte, no byte order [Table 4.6] */
    p_cur->p_buf[p_cur->pos] = value;
    p_cur->pos = p_cur->pos + SOMEIP_U8_SIZE;

    /* Written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerWriteU16(INOUT someip_ser_cursor_t *p_cur, IN uint16_t value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 2 bytes */
    res = prv_ser_reserve(p_cur, SOMEIP_U16_SIZE);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint16 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU16(&p_cur->p_buf[p_cur->pos], value, p_cur->order);
    p_cur->pos += SOMEIP_U16_SIZE;

    /* Written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerWriteU32(INOUT someip_ser_cursor_t *p_cur, IN uint32_t value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 4 bytes */
    res = prv_ser_reserve(p_cur, SOMEIP_U32_SIZE);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint32 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU32(&p_cur->p_buf[p_cur->pos], value, p_cur->order);
    p_cur->pos += SOMEIP_U32_SIZE;

    /* Written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerWriteU64(INOUT someip_ser_cursor_t *p_cur, IN uint64_t value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 8 bytes */
    res = prv_ser_reserve(p_cur, SOMEIP_U64_SIZE);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint64 in configured byte order [PRS_SOMEIP_00369] */
    SOMEIP_WriteU64(&p_cur->p_buf[p_cur->pos], value, p_cur->order);
    p_cur->pos += SOMEIP_U64_SIZE;

    /* Written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerWriteS8(INOUT someip_ser_cursor_t *p_cur, IN int8_t value) {
    /* Two's complement: same byte as uint8 */
    return SOMEIP_SerWriteU8(p_cur, (uint8_t)value);
}

someip_result_t SOMEIP_SerWriteS16(INOUT someip_ser_cursor_t *p_cur, IN int16_t value) {
    /* Two's complement: same bytes as uint16 */
    return SOMEIP_SerWriteU16(p_cur, (uint16_t)value);
}

someip_result_t SOMEIP_SerWriteS32(INOUT someip_ser_cursor_t *p_cur, IN int32_t value) {
    /* Two's complement: same bytes as uint32 */
    return SOMEIP_SerWriteU32(p_cur, (uint32_t)value);
}

someip_result_t SOMEIP_SerWriteS64(INOUT someip_ser_cursor_t *p_cur, IN int64_t value) {
    /* Two's complement: same bytes as uint64 */
    return SOMEIP_SerWriteU64(p_cur, (uint64_t)value);
}

someip_result_t SOMEIP_SerWriteF32(INOUT someip_ser_cursor_t *p_cur, IN float value) {
    uint32_t raw = 0U;

    /* float32 = IEEE 754 binary32 bit pattern (Table 4.6) */
    (void)memcpy(&raw, &value, sizeof(raw));

    /* Same 4 bytes as uint32, in the configured byte order [PRS_SOMEIP_00369] */
    return SOMEIP_SerWriteU32(p_cur, raw);
}

someip_result_t SOMEIP_SerWriteF64(INOUT someip_ser_cursor_t *p_cur, IN double value) {
    uint64_t raw = 0U;

    /* float64 = IEEE 754 binary64 bit pattern (Table 4.6) */
    (void)memcpy(&raw, &value, sizeof(raw));

    /* Same 8 bytes as uint64, in the configured byte order [PRS_SOMEIP_00369] */
    return SOMEIP_SerWriteU64(p_cur, raw);
}

/* Serializer: raw bytes / alignment */

someip_result_t SOMEIP_SerWriteBytes(INOUT someip_ser_cursor_t *p_cur, IN uint8_t *p_data,
                                     IN uint32_t data_len) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Data may be NULL only for an empty array */
    if ((NULL == p_data) && (0U != data_len)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_data pointer */
        return p_cur->status;
    }

    /* Sticky status + enough space for data_len bytes */
    res = prv_ser_reserve(p_cur, data_len);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* Raw copy, no byte order */
    if (0U != data_len) {
        (void)memcpy(&p_cur->p_buf[p_cur->pos], p_data, data_len);
        p_cur->pos += data_len;
    }

    /* Written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerAlign(INOUT someip_ser_cursor_t *p_cur, IN uint16_t align_bits) {
    uint32_t n = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* [PRS_SOMEIP_00613] */
    if (!prv_is_align_valid(align_bits)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid alignment */
        return p_cur->status;
    }

    /* [PRS_SOMEIP_00569] */
    n = prv_padding_size(p_cur->msg_offset + p_cur->pos, align_bits);

    res = prv_ser_reserve(p_cur, n);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* [PRS_SOMEIP_00611] */
    if (0U != n) {
        (void)memset(&p_cur->p_buf[p_cur->pos], 0, n);
        p_cur->pos += n;
    }

    /* Next element starts aligned */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerLengthBegin(INOUT someip_ser_cursor_t *p_cur,
                                      IN someip_length_field_size_t lf_size, OUT uint32_t *p_mark) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_mark)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Only 8, 16, 32 bit length field [PRS_SOMEIP_00079] */
    if (!prv_is_length_field_valid(lf_size)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid length field size */
        return p_cur->status;
    }

    /* Sticky status + enough space for the length field (enum value = size in bytes) */
    res = prv_ser_reserve(p_cur, (uint32_t)lf_size);

    /* Stop on sticky error or not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* Remember where the length field is, value is filled by SOMEIP_SerLengthEnd() */
    *p_mark = p_cur->pos;

    /* Placeholder 0x00, so the buffer never holds garbage */
    (void)memset(&p_cur->p_buf[p_cur->pos], 0, (size_t)lf_size);
    p_cur->pos += (uint32_t)lf_size;

    /* Length field reserved */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerLengthEnd(INOUT someip_ser_cursor_t *p_cur, IN someip_length_field_size_t lf_size,
                                    IN uint32_t mark) {
    uint32_t len = 0U;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Only 8, 16, 32 bit length field [PRS_SOMEIP_00079] */
    if (!prv_is_length_field_valid(lf_size)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid length field size */
        return p_cur->status;
    }

    /* mark + lf_size must stay inside the written part (also avoids unsigned underflow of len) */
    if ((p_cur->pos < (uint32_t)lf_size) || (mark > (p_cur->pos - (uint32_t)lf_size))) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid mark */
        return p_cur->status;
    }

    /* The length field does not count itself [PRS_SOMEIP_00377] [PRS_SOMEIP_00095] */
    len = p_cur->pos - (mark + (uint32_t)lf_size);

    /* Fill the reserved bytes in the configured byte order, pos is NOT moved */
    switch (lf_size) {
        case SOMEIP_LENGTH_FIELD_8BIT:
            /* Length does not fit in 8 bit */
            if (UINT8_MAX < len) {
                p_cur->status = SOMEIP_RES_E_NO_SPACE;

                /* Length field not written */
                return p_cur->status;
            }

            p_cur->p_buf[mark] = (uint8_t)len;

            /* 8 bit length written */
            break;

        case SOMEIP_LENGTH_FIELD_16BIT:
            /* Length does not fit in 16 bit */
            if (UINT16_MAX < len) {
                p_cur->status = SOMEIP_RES_E_NO_SPACE;

                /* Length field not written */
                return p_cur->status;
            }

            SOMEIP_WriteU16(&p_cur->p_buf[mark], (uint16_t)len, p_cur->order);

            /* 16 bit length written */
            break;

        case SOMEIP_LENGTH_FIELD_32BIT:
            /* pos is uint32_t: the length always fits */
            SOMEIP_WriteU32(&p_cur->p_buf[mark], len, p_cur->order);

            /* 32 bit length written */
            break;

        case SOMEIP_LENGTH_FIELD_NONE: /* rejected by prv_is_length_field_valid() */
        default:
            p_cur->status = SOMEIP_RES_E_PARAM;

            /* Unreachable */
            return p_cur->status;
    }

    /* Length field filled */
    return SOMEIP_RES_OK;
}

/* Serializer: strings (chapter 4.1.4.4) */

someip_result_t SOMEIP_SerWriteString(INOUT someip_ser_cursor_t *p_cur, IN someip_string_encoding_t enc,
                                      IN someip_length_field_size_t lf_size, IN uint8_t *p_str,
                                      IN uint32_t str_len) {
    uint8_t const terminator[SOMEIP_STR_TERM_UTF16_SIZE] = {0U};
    uint8_t const *p_bom = NULL;
    uint32_t bom_size = 0U;
    uint32_t term_size = 0U;
    uint32_t start_pos = 0U;
    uint32_t mark = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Characters may be NULL only for an empty string */
    if ((NULL == p_str) && (0U != str_len)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_str pointer */
        return p_cur->status;
    }

    p_bom = prv_get_bom(enc, &bom_size);

    /* Unknown encoding [PRS_SOMEIP_00372] */
    if (NULL == p_bom) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* No BOM for this encoding */
        return p_cur->status;
    }

    term_size = prv_get_terminator_size(enc);

    /* UTF-16 strings shall have an even length [PRS_SOMEIP_00085] [PRS_SOMEIP_00092] */
    if ((SOMEIP_STRING_UTF8 != enc) && (0U != (str_len % (uint32_t)SOMEIP_U16_SIZE))) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Odd number of bytes in UTF-16 */
        return p_cur->status;
    }

    /* Restored on error: the position of the cursor is never moved by a failed call */
    start_pos = p_cur->pos;

    /* Status is sticky: after the first error all following calls do nothing and End reports it */
    (void)SOMEIP_SerLengthBegin(p_cur, lf_size, &mark);       /* [PRS_SOMEIP_00089] [PRS_SOMEIP_00093] */
    (void)SOMEIP_SerWriteBytes(p_cur, p_bom, bom_size);       /* In the length [PRS_SOMEIP_00090] */
    (void)SOMEIP_SerWriteBytes(p_cur, p_str, str_len);        /* Characters, already encoded */
    (void)SOMEIP_SerWriteBytes(p_cur, terminator, term_size); /* [PRS_SOMEIP_00091] */
    res = SOMEIP_SerLengthEnd(p_cur, lf_size, mark);          /* [PRS_SOMEIP_00095] */

    /* Length field, BOM, characters or "\0" failed */
    if (SOMEIP_RES_OK != res) {
        p_cur->pos = start_pos;

        /* Nothing of this string stays in the payload */
        return res;
    }

    /* String written */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SerWriteStringFixed(INOUT someip_ser_cursor_t *p_cur,
                                           IN someip_string_encoding_t enc, IN uint32_t fixed_size,
                                           IN uint8_t *p_str, IN uint32_t str_len) {
    uint8_t *p_dst = NULL;
    uint8_t const *p_bom = NULL;
    uint32_t bom_size = 0U;
    uint32_t term_size = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following writes */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Characters may be NULL only for an empty string */
    if ((NULL == p_str) && (0U != str_len)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_str pointer */
        return p_cur->status;
    }

    p_bom = prv_get_bom(enc, &bom_size);

    /* Unknown encoding [PRS_SOMEIP_00372] */
    if (NULL == p_bom) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* No BOM for this encoding */
        return p_cur->status;
    }

    term_size = prv_get_terminator_size(enc);

    /* UTF-16 strings shall have an even length [PRS_SOMEIP_00085] */
    if ((SOMEIP_STRING_UTF8 != enc) && ((0U != (str_len % (uint32_t)SOMEIP_U16_SIZE)) ||
                                        (0U != (fixed_size % (uint32_t)SOMEIP_U16_SIZE)))) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Odd number of bytes in UTF-16 */
        return p_cur->status;
    }

    /* BOM + characters + "\0" must fit in fixed_size [PRS_SOMEIP_00374], written without overflow */
    if ((fixed_size < (bom_size + term_size)) || (str_len > (fixed_size - bom_size - term_size))) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* String too long */
        return p_cur->status;
    }

    /* Sticky status + enough space for the whole string: nothing below can fail */
    res = prv_ser_reserve(p_cur, fixed_size);

    /* Stop on not enough space */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    p_dst = &p_cur->p_buf[p_cur->pos];

    /* BOM also in fixed length strings [PRS_SOMEIP_00087] */
    (void)memcpy(p_dst, p_bom, bom_size);

    /* Characters: p_str may be NULL for an empty string */
    if (0U != str_len) {
        (void)memcpy(&p_dst[bom_size], p_str, str_len);
    }

    /* "\0" and 0x00 fill up to fixed_size [PRS_SOMEIP_00373] */
    (void)memset(&p_dst[bom_size + str_len], 0, fixed_size - bom_size - str_len);
    p_cur->pos += fixed_size;

    /* String written */
    return SOMEIP_RES_OK;
}

/* Deserializer: cursor */

someip_result_t SOMEIP_DeserInit(OUT someip_deser_cursor_t *p_cur, IN uint8_t *p_buf, IN uint32_t buf_size,
                                 IN uint32_t msg_offset, IN someip_byte_order_t order) {
    /* Validate parameters: p_buf may be NULL only for an empty payload */
    if ((NULL == p_cur) || ((NULL == p_buf) && (0U != buf_size))) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    p_cur->p_buf = p_buf;
    p_cur->buf_size = buf_size;
    p_cur->pos = 0U;                /* nothing read yet                         */
    p_cur->msg_offset = msg_offset; /* alignment base       [PRS_SOMEIP_00569]  */
    p_cur->order = order;           /* default byte order   [PRS_SOMEIP_00369]  */
    p_cur->status = SOMEIP_RES_OK;

    /* Cursor ready */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserSetByteOrder(INOUT someip_deser_cursor_t *p_cur, IN someip_byte_order_t order) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Validate byte order */
    if ((SOMEIP_BYTE_ORDER_BIG_ENDIAN != order) && (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN != order)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid byte order */
        return p_cur->status;
    }

    /* Byte order of the next parameters [PRS_SOMEIP_00369] */
    p_cur->order = order;

    /* Byte order changed */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserGetStatus(IN someip_deser_cursor_t *p_cur) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* No cursor */
        return SOMEIP_RES_E_PARAM;
    }

    /* First error, SOMEIP_RES_OK while no error */
    return p_cur->status;
}

uint32_t SOMEIP_DeserGetRemaining(IN someip_deser_cursor_t *p_cur) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* No cursor: nothing to read */
        return 0U;
    }

    /* Bytes not read yet */
    return p_cur->buf_size - p_cur->pos;
}

/* Deserializer: basic datatypes (Table 4.6) [PRS_SOMEIP_00065] */

someip_result_t SOMEIP_DeserReadBool(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_value) {
    uint8_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU8 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* boolean = 8 bit, same as uint8 (Table 4.6) */
    res = SOMEIP_DeserReadU8(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* Only the lowest bit is evaluated [PRS_SOMEIP_00615] */
    *p_value = (uint8_t)(raw & SOMEIP_BOOL_VALUE_MASK);

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadU8(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough bytes for 1 byte */
    res = prv_deser_require(p_cur, SOMEIP_U8_SIZE);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint8: 1 byte, no byte order [Table 4.6] */
    *p_value = p_cur->p_buf[p_cur->pos];
    p_cur->pos = p_cur->pos + SOMEIP_U8_SIZE;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadU16(INOUT someip_deser_cursor_t *p_cur, OUT uint16_t *p_value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough bytes for 2 bytes */
    res = prv_deser_require(p_cur, SOMEIP_U16_SIZE);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint16 in configured byte order [PRS_SOMEIP_00369] */
    *p_value = SOMEIP_ReadU16(&p_cur->p_buf[p_cur->pos], p_cur->order);
    p_cur->pos += SOMEIP_U16_SIZE;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadU32(INOUT someip_deser_cursor_t *p_cur, OUT uint32_t *p_value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough bytes for 4 bytes */
    res = prv_deser_require(p_cur, SOMEIP_U32_SIZE);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint32 in configured byte order [PRS_SOMEIP_00369] */
    *p_value = SOMEIP_ReadU32(&p_cur->p_buf[p_cur->pos], p_cur->order);
    p_cur->pos += SOMEIP_U32_SIZE;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadU64(INOUT someip_deser_cursor_t *p_cur, OUT uint64_t *p_value) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough bytes for 8 bytes */
    res = prv_deser_require(p_cur, SOMEIP_U64_SIZE);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* uint64 in configured byte order [PRS_SOMEIP_00369] */
    *p_value = SOMEIP_ReadU64(&p_cur->p_buf[p_cur->pos], p_cur->order);
    p_cur->pos += SOMEIP_U64_SIZE;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadS8(INOUT someip_deser_cursor_t *p_cur, OUT int8_t *p_value) {
    uint8_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU8 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* sint8 = same byte as uint8 */
    res = SOMEIP_DeserReadU8(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* Two's complement: same bit pattern as uint8 */
    *p_value = (int8_t)raw;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadS16(INOUT someip_deser_cursor_t *p_cur, OUT int16_t *p_value) {
    uint16_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU16 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* sint16 = same bytes as uint16, in the configured byte order [PRS_SOMEIP_00369] */
    res = SOMEIP_DeserReadU16(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* Two's complement: same bit pattern as uint16 */
    *p_value = (int16_t)raw;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadS32(INOUT someip_deser_cursor_t *p_cur, OUT int32_t *p_value) {
    uint32_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU32 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* sint32 = same bytes as uint32, in the configured byte order [PRS_SOMEIP_00369] */
    res = SOMEIP_DeserReadU32(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* Two's complement: same bit pattern as uint32 */
    *p_value = (int32_t)raw;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadS64(INOUT someip_deser_cursor_t *p_cur, OUT int64_t *p_value) {
    uint64_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU64 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* sint64 = same bytes as uint64, in the configured byte order [PRS_SOMEIP_00369] */
    res = SOMEIP_DeserReadU64(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* Two's complement: same bit pattern as uint64 */
    *p_value = (int64_t)raw;

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadF32(INOUT someip_deser_cursor_t *p_cur, OUT float *p_value) {
    uint32_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU32 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Same 4 bytes as uint32, in the configured byte order [PRS_SOMEIP_00369] */
    res = SOMEIP_DeserReadU32(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* float32 = IEEE 754 binary32 bit pattern (Table 4.6) */
    (void)memcpy(p_value, &raw, sizeof(raw));

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserReadF64(INOUT someip_deser_cursor_t *p_cur, OUT double *p_value) {
    uint64_t raw = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters: ReadU64 only sees the local raw, not p_value */
    if ((NULL == p_cur) || (NULL == p_value)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Same 8 bytes as uint64, in the configured byte order [PRS_SOMEIP_00369] */
    res = SOMEIP_DeserReadU64(p_cur, &raw);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* *p_value not changed */
        return res;
    }

    /* float64 = IEEE 754 binary64 bit pattern (Table 4.6) */
    (void)memcpy(p_value, &raw, sizeof(raw));

    /* Read */
    return SOMEIP_RES_OK;
}

/* Deserializer: raw bytes / alignment */

someip_result_t SOMEIP_DeserReadBytes(INOUT someip_deser_cursor_t *p_cur, OUT uint8_t *p_out,
                                      IN uint32_t data_len) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Destination may be NULL only for an empty read */
    if ((NULL == p_out) && (0U != data_len)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_out pointer */
        return p_cur->status;
    }

    /* Sticky status + enough bytes for data_len */
    res = prv_deser_require(p_cur, data_len);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* Raw copy, no byte order */
    if (0U != data_len) {
        (void)memcpy(p_out, &p_cur->p_buf[p_cur->pos], data_len);
        p_cur->pos += data_len;
    }

    /* Read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserSkip(INOUT someip_deser_cursor_t *p_cur, IN uint32_t skip_len) {
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough bytes to skip */
    res = prv_deser_require(p_cur, skip_len);

    /* Stop on sticky error or not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* Only the position moves, nothing is copied */
    p_cur->pos += skip_len;

    /* Skipped */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserAlign(INOUT someip_deser_cursor_t *p_cur, IN uint16_t align_bits) {
    uint32_t pad_len = 0U;
    uint32_t remaining = 0U;

    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* [PRS_SOMEIP_00613] */
    if (!prv_is_align_valid(align_bits)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid alignment */
        return p_cur->status;
    }

    /* [PRS_SOMEIP_00569] */
    pad_len = prv_padding_size(p_cur->msg_offset + p_cur->pos, align_bits);
    remaining = p_cur->buf_size - p_cur->pos;

    /* No padding behind the last element [PRS_SOMEIP_00611]: missing padding is not an error */
    if (pad_len > remaining) {
        pad_len = remaining;
    }

    /* The padding value is not defined, so it is not checked (note of [PRS_SOMEIP_00611]) */
    p_cur->pos += pad_len;

    /* Next element starts aligned */
    return SOMEIP_RES_OK;
}

/* Deserializer: length field [PRS_SOMEIP_00370] [PRS_SOMEIP_00371] [PRS_SOMEIP_00900] */

someip_result_t SOMEIP_DeserLengthBegin(INOUT someip_deser_cursor_t *p_cur,
                                        IN someip_length_field_size_t lf_size, OUT uint32_t *p_end) {
    uint8_t len_8 = 0U;
    uint16_t len_16 = 0U;
    uint32_t len = 0U;
    uint32_t start_pos = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_end)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Only 8, 16, 32 bit length field [PRS_SOMEIP_00079] */
    if (!prv_is_length_field_valid(lf_size)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid length field size */
        return p_cur->status;
    }

    /* Restored on error: the position of the cursor is never moved by a failed call */
    start_pos = p_cur->pos;

    /* Read the length field in the configured byte order */
    switch (lf_size) {
        case SOMEIP_LENGTH_FIELD_8BIT:
            res = SOMEIP_DeserReadU8(p_cur, &len_8);
            len = len_8;

            /* 8 bit length read */
            break;

        case SOMEIP_LENGTH_FIELD_16BIT:
            res = SOMEIP_DeserReadU16(p_cur, &len_16);
            len = len_16;

            /* 16 bit length read */
            break;

        case SOMEIP_LENGTH_FIELD_32BIT:
            res = SOMEIP_DeserReadU32(p_cur, &len);

            /* 32 bit length read */
            break;

        case SOMEIP_LENGTH_FIELD_NONE: /* rejected by prv_is_length_field_valid() */
        default:
            p_cur->status = SOMEIP_RES_E_PARAM;

            /* Unreachable */
            return p_cur->status;
    }

    /* Stop on not enough bytes for the length field */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    /* Length points outside the payload */
    if (len > SOMEIP_DeserGetRemaining(p_cur)) {
        p_cur->pos = start_pos;
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* *p_end not changed */
        return p_cur->status;
    }

    /* The length field does not count itself [PRS_SOMEIP_00377] [PRS_SOMEIP_00095] */
    *p_end = p_cur->pos + len;

    /* Length field read */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_DeserLengthEnd(INOUT someip_deser_cursor_t *p_cur, IN uint32_t end) {
    /* Validate parameters */
    if (NULL == p_cur) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* end is not a value from SOMEIP_DeserLengthBegin(): pos must never pass buf_size */
    if (end > p_cur->buf_size) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid end */
        return p_cur->status;
    }

    /* Members were read past the length [PRS_SOMEIP_00900] */
    if (p_cur->pos > end) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* Position not moved */
        return p_cur->status;
    }

    /* Unknown extra bytes are skipped based on the length field [PRS_SOMEIP_00371] */
    p_cur->pos = end;

    /* Struct / array closed */
    return SOMEIP_RES_OK;
}

/* Deserializer: strings (chapter 4.1.4.4) */

someip_result_t SOMEIP_DeserReadString(INOUT someip_deser_cursor_t *p_cur, IN someip_string_encoding_t enc,
                                       IN someip_length_field_size_t lf_size, OUT uint8_t *p_out,
                                       IN uint32_t out_size, OUT uint32_t *p_out_len) {
    uint8_t const terminator[SOMEIP_STR_TERM_UTF16_SIZE] = {0U};
    uint8_t const *p_bom = NULL;
    uint8_t const *p_src = NULL;
    uint32_t bom_size = 0U;
    uint32_t term_size = 0U;
    uint32_t start_pos = 0U;
    uint32_t end = 0U;
    uint32_t len = 0U;
    uint32_t chars_len = 0U;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_out_len)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Destination may be NULL only if it has no capacity */
    if ((NULL == p_out) && (0U != out_size)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_out pointer */
        return p_cur->status;
    }

    p_bom = prv_get_bom(enc, &bom_size);

    /* Unknown encoding [PRS_SOMEIP_00372] */
    if (NULL == p_bom) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* No BOM for this encoding */
        return p_cur->status;
    }

    term_size = prv_get_terminator_size(enc);

    /* Restored on error: the position of the cursor is never moved by a failed call */
    start_pos = p_cur->pos;

    /* Length field in front of the BOM, lf_size checked inside [PRS_SOMEIP_00089] [PRS_SOMEIP_00090] */
    res = SOMEIP_DeserLengthBegin(p_cur, lf_size, &end);

    /* Stop on invalid lf_size, truncated length field or length outside the payload */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    len = end - p_cur->pos;
    p_src = &p_cur->p_buf[p_cur->pos];

    /* UTF-16 with odd length: the last byte shall be ignored [PRS_SOMEIP_00086] [PRS_SOMEIP_00092] */
    if ((SOMEIP_STRING_UTF8 != enc) && (0U != (len % (uint32_t)SOMEIP_U16_SIZE))) {
        len -= 1U;
    }

    /* Too short for BOM and "\0" */
    if (len < (bom_size + term_size)) {
        p_cur->pos = start_pos;
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* No valid string */
        return p_cur->status;
    }

    /* BOM does not match the expected encoding [PRS_SOMEIP_00087] */
    if (0 != memcmp(p_src, p_bom, bom_size)) {
        p_cur->pos = start_pos;
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* Wrong encoding */
        return p_cur->status;
    }

    chars_len = len - bom_size - term_size;

    /* Longer than the type definition allows [PRS_SOMEIP_00914] */
    if (chars_len > out_size) {
        p_cur->pos = start_pos;
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* p_out not written */
        return p_cur->status;
    }

    /* Not terminated with "\0" [PRS_SOMEIP_00091] [PRS_SOMEIP_00948] [PRS_SOMEIP_00084] */
    if (0 != memcmp(&p_src[bom_size + chars_len], terminator, term_size)) {
        p_cur->pos = start_pos;
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* No terminator */
        return p_cur->status;
    }

    /* Characters only, without BOM and "\0" */
    if (0U != chars_len) {
        (void)memcpy(p_out, &p_src[bom_size], chars_len);
    }

    *p_out_len = chars_len;

    /* pos <= end here: moves to end, also over an ignored odd byte */
    return SOMEIP_DeserLengthEnd(p_cur, end);
}

someip_result_t SOMEIP_DeserReadStringFixed(INOUT someip_deser_cursor_t *p_cur,
                                            IN someip_string_encoding_t enc, IN uint32_t fixed_size,
                                            OUT uint8_t *p_out, IN uint32_t out_size,
                                            OUT uint32_t *p_out_len) {
    uint8_t const terminator[SOMEIP_STR_TERM_UTF16_SIZE] = {0U};
    uint8_t const *p_bom = NULL;
    uint8_t const *p_src = NULL;
    uint32_t bom_size = 0U;
    uint32_t term_size = 0U;
    uint32_t term_idx = 0U;
    uint32_t chars_len = 0U;
    bool is_terminated = false;
    someip_result_t res = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_cur) || (NULL == p_out_len)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Previous error: stop all following reads */
    if (SOMEIP_RES_OK != p_cur->status) {
        /* Keep reporting the first error */
        return p_cur->status;
    }

    /* Destination may be NULL only if it has no capacity */
    if ((NULL == p_out) && (0U != out_size)) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid p_out pointer */
        return p_cur->status;
    }

    p_bom = prv_get_bom(enc, &bom_size);

    /* Unknown encoding [PRS_SOMEIP_00372] */
    if (NULL == p_bom) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* No BOM for this encoding */
        return p_cur->status;
    }

    term_size = prv_get_terminator_size(enc);

    /* fixed_size from the type definition: must hold BOM and "\0", UTF-16 even [PRS_SOMEIP_00085] */
    if ((fixed_size < (bom_size + term_size)) ||
        ((SOMEIP_STRING_UTF8 != enc) && (0U != (fixed_size % (uint32_t)SOMEIP_U16_SIZE)))) {
        p_cur->status = SOMEIP_RES_E_PARAM;

        /* Invalid fixed_size */
        return p_cur->status;
    }

    /* Enough bytes for the whole string */
    res = prv_deser_require(p_cur, fixed_size);

    /* Stop on not enough bytes */
    if (SOMEIP_RES_OK != res) {
        /* Position not moved */
        return res;
    }

    p_src = &p_cur->p_buf[p_cur->pos];

    /* BOM does not match the expected encoding [PRS_SOMEIP_00087] */
    if (0 != memcmp(p_src, p_bom, bom_size)) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* Wrong encoding */
        return p_cur->status;
    }

    term_idx = bom_size;

    /* First "\0" behind the BOM, UTF-16 steps per 2 bytes so that a terminator is never split */
    while ((!is_terminated) && (term_idx <= (fixed_size - term_size))) {
        /* Terminator found at this position */
        if (0 == memcmp(&p_src[term_idx], terminator, term_size)) {
            is_terminated = true;
        }

        /* Otherwise: next character */
        else {
            term_idx += term_size;
        }
    }

    /* Not terminated inside fixed_size [PRS_SOMEIP_00373] [PRS_SOMEIP_00913] */
    if (!is_terminated) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* No terminator */
        return p_cur->status;
    }

    chars_len = term_idx - bom_size;

    /* Longer than the receiver allows [PRS_SOMEIP_00911] */
    if (chars_len > out_size) {
        p_cur->status = SOMEIP_RES_E_MALFORMED;

        /* p_out not written */
        return p_cur->status;
    }

    /* Characters only: shorter strings are accepted [PRS_SOMEIP_00912] */
    if (0U != chars_len) {
        (void)memcpy(p_out, &p_src[bom_size], chars_len);
    }

    *p_out_len = chars_len;

    /* Always fixed_size on the wire: the fill behind "\0" is skipped, its value is not checked */
    p_cur->pos += fixed_size;

    /* String read */
    return SOMEIP_RES_OK;
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
