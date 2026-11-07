

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

/** @brief Size of the "\0" terminator per string encoding [PRS_SOMEIP_00084] */
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

/** @brief Get the size of the "\0" terminator of a string encoding [PRS_SOMEIP_00084]
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

    /* Raw copy, no byte order [PRS_SOMEIP_00099] */
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

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
