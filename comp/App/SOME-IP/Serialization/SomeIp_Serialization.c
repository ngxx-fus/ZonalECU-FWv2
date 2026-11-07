/**
 ************************************************************************************************************
 * @file    SomeIp_Serialization.c
 * @brief   SOME/IP payload serialization / deserialization.
 *
 * @details This file provides the functions to:
 *          - Write / read basic datatypes with configurable byte order    [PRS_SOMEIP_00065] [PRS_SOMEIP_00369]
 *          - Insert / skip padding behind variable length data            [PRS_SOMEIP_00611] [PRS_SOMEIP_00569]
 *          - Write / read the length field of a struct or array           [PRS_SOMEIP_00370] [PRS_SOMEIP_00371]
 *                                                                         [PRS_SOMEIP_00376] [PRS_SOMEIP_00377]
 *          - Write / read fixed and dynamic length strings                [PRS_SOMEIP_00087] [PRS_SOMEIP_00089]
 *
 *          Every public function:
 *          1. returns cur->status at once if a previous call already failed (sticky status),
 *          2. on a new error stores it in cur->status and returns it,
 *          3. never moves cur->pos on error.
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.3 / 4.1.4 (4.1.4.1 .. 4.1.4.7)
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "SomeIp_Serialization.h"
#include "SomeIp_Endian.h"
#include "SomeIp_Types.h"
#include <stdint.h>
#include <string.h>

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* EXPORTED VARIABLES ***************************************************************************************
 ************************************************************************************************************/

/* PRIVATE VARIABLES ****************************************************************************************
 ************************************************************************************************************/

/* PRIVATE FUNCTIONS ****************************************************************************************
 ************************************************************************************************************/
static SomeIp_Result_t SomeIp_Ser_Reserve(SomeIp_SerCursor_t *cur, uint32_t len);
static SomeIp_Result_t SomeIp_Deser_Require(SomeIp_DeserCursor_t *cur, uint32_t len);
static uint32_t SomeIp_PaddingSize(uint32_t absPos, uint16_t alignBits);
static uint8_t SomeIp_IsAlignValid(uint16_t alignBits);
static uint8_t SomeIp_IsLengthFieldValid(SomeIp_LengthFieldSize_t lfSize);
static const uint8_t *SomeIp_GetBom(SomeIp_StringEncoding_t enc, uint32_t *bomSize);
static uint32_t SomeIp_GetTerminatorSize(SomeIp_StringEncoding_t enc);

/* PUBLIC VARIABLES *****************************************************************************************
 ************************************************************************************************************/
/* String Byte Order Marks                                                                [PRS_SOMEIP_00087] */
static const uint8_t SomeIp_BomUtf8[SOMEIP_BOM_UTF8_SIZE] =
{
    SOMEIP_BOM_UTF8_0, SOMEIP_BOM_UTF8_1, SOMEIP_BOM_UTF8_2
};
static const uint8_t SomeIp_BomUtf16Be[SOMEIP_BOM_UTF16_SIZE] =
{
    SOMEIP_BOM_UTF16BE_0, SOMEIP_BOM_UTF16BE_1
};
static const uint8_t SomeIp_BomUtf16Le[SOMEIP_BOM_UTF16_SIZE] =
{
    SOMEIP_BOM_UTF16LE_0, SOMEIP_BOM_UTF16LE_1
};
/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS *********************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * PRIVATE HELPERS
 * ======================================================================================================== */

/**
 * @brief   Check that @p len more bytes can be written.
 *
 * @param[in,out] cur  Write cursor (not NULL, checked by the caller).
 * @param[in]     len  Number of bytes the caller wants to write.
 *
 * @retval SOMEIP_RES_OK         Enough space.
 * @retval SOMEIP_RES_E_NO_SPACE Buffer too small (or previous sticky error).
 */
static SomeIp_Result_t SomeIp_Ser_Reserve(SomeIp_SerCursor_t *cur, uint32_t len)
{
    /* Previous error: stop all following writes */
    if (cur->status != SOMEIP_RES_OK)
    {
        return cur->status;
    }

    /* Not enough space: keep the error */
    if ((cur->size - cur->pos) < len)
    {
        cur->status = SOMEIP_RES_E_NO_SPACE;
        return cur->status;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Check that @p len more bytes can be read.
 *
 * @param[in,out] cur  Read cursor (not NULL, checked by the caller).
 * @param[in]     len  Number of bytes the caller wants to read.
 *
 * @retval SOMEIP_RES_OK          Enough bytes.
 * @retval SOMEIP_RES_E_MALFORMED Not enough bytes (or previous sticky error).
 */
static SomeIp_Result_t SomeIp_Deser_Require(SomeIp_DeserCursor_t *cur, uint32_t len)
{
    /* Previous error: stop all following reads */
    if (cur->status != SOMEIP_RES_OK)
    {
        return cur->status;
    }

    /* Not enough bytes: keep the error */
    if ((cur->size - cur->pos) < len)
    {
        cur->status = SOMEIP_RES_E_MALFORMED;
        return cur->status;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Number of padding bytes needed so that the next element starts aligned.
 *
 * @param[in]  absPos     Position from the start of the SOME/IP message.
 * @param[in]  alignBits  8, 16, 32, 64, 128 or 256 (already checked by SomeIp_IsAlignValid()).
 *
 * @return  Number of padding bytes (0 .. alignBits/8 - 1).
 */
static uint32_t SomeIp_PaddingSize(uint32_t absPos, uint16_t alignBits)
{
    uint32_t alignBytes;
    uint32_t remainder;

    /* 32 bit -> 4 byte */
    alignBytes = (uint32_t)alignBits / 8u;

    /* How many bytes past the last boundary */
    remainder = absPos % alignBytes;

    /* Already on a boundary: no padding */
    if (remainder == 0u)
    {
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
static uint8_t SomeIp_IsAlignValid(uint16_t alignBits)
{
    uint8_t status;

    switch (alignBits)
    {
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
static uint8_t SomeIp_IsLengthFieldValid(SomeIp_LengthFieldSize_t lfSize)
{
    uint8_t status;

    switch (lfSize)
    {
        case SOMEIP_LENGTH_FIELD_8BIT:
        case SOMEIP_LENGTH_FIELD_16BIT:
        case SOMEIP_LENGTH_FIELD_32BIT:
            status = 1u;
            break;

        case SOMEIP_LENGTH_FIELD_NONE:      /* no length field: do not call LengthBegin/End */
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
 * @param[out] bomSize  Size of the BOM in bytes.
 *
 * @return  Pointer to the BOM bytes, or NULL.
 *
 * @see     [PRS_SOMEIP_00087]
 */
static const uint8_t *SomeIp_GetBom(SomeIp_StringEncoding_t enc, uint32_t *bomSize)
{
    const uint8_t *bom;

    switch (enc)
    {
        case SOMEIP_STRING_UTF8:
            bom = SomeIp_BomUtf8;
            *bomSize = SOMEIP_BOM_UTF8_SIZE;
            break;
        
        case SOMEIP_STRING_UTF16BE:
            bom = SomeIp_BomUtf16Be;
            *bomSize = SOMEIP_BOM_UTF16_SIZE;
            break;

        case SOMEIP_STRING_UTF16LE:
            bom = SomeIp_BomUtf16Le;
            *bomSize = SOMEIP_BOM_UTF16_SIZE;
            break;
        
        default:
            /* Unknown encoding */
            bom = NULL;
            *bomSize = 0u;
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
static uint32_t SomeIp_GetTerminatorSize(SomeIp_StringEncoding_t enc)
{
    uint32_t termSize;

    switch (enc)
    {
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
 * @param[out] cur        Cursor to initialize.
 * @param[in]  buf        Output buffer.
 * @param[in]  size       Capacity of @p buf.
 * @param[in]  msgOffset  Offset of buf[0] from the start of the SOME/IP message, used for alignment
 *                        [PRS_SOMEIP_00569]. Use SOMEIP_OFS_PAYLOAD if @p buf is the payload.
 * @param[in]  order      Initial byte order [PRS_SOMEIP_00369].
 *
 * @retval SOMEIP_RES_OK       Cursor ready.
 * @retval SOMEIP_RES_E_PARAM  @p cur or @p buf is NULL.
 */
SomeIp_Result_t SomeIp_Ser_Init(SomeIp_SerCursor_t *cur, uint8_t *buf, uint32_t size, uint32_t msgOffset,
                                SomeIp_ByteOrder_t order)
{
    /* Validate parameters */
    if ((cur == NULL) || (buf == NULL))
    {
        return SOMEIP_RES_E_PARAM;
    }

    cur->buf = buf;
    cur->size = size;
    cur->pos = 0u;              /* nothing written yet                      */
    cur->msgOffset = msgOffset; /* alignment base       [PRS_SOMEIP_00569]  */
    cur->order = order;         /* default byte order   [PRS_SOMEIP_00369]  */
    cur->status = SOMEIP_RES_OK;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Change the byte order for the next parameters.
 *
 * @param[in,out] cur    Write cursor.
 * @param[in]     order  New byte order.
 *
 * @retval SOMEIP_RES_OK
 * @retval SOMEIP_RES_E_PARAM  @p cur is NULL.
 */
SomeIp_Result_t SomeIp_Ser_SetByteOrder(SomeIp_SerCursor_t *cur, SomeIp_ByteOrder_t order)
{
    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    if ((order != SOMEIP_BYTE_ORDER_BIG_ENDIAN) && (order != SOMEIP_BYTE_ORDER_LITTLE_ENDIAN))
    {
        cur->status = SOMEIP_RES_E_PARAM;
        return cur->status;
    }

    /* Byte order of the next parameters [PRS_SOMEIP_00369] */
    cur->order = order;

    return SOMEIP_RES_OK;
}


/**
 * @brief   Get the sticky status of a write cursor.
 *
 * @param[in]  cur  Write cursor.
 *
 * @return  cur->status, or SOMEIP_RES_E_PARAM if @p cur is NULL.
 */
SomeIp_Result_t SomeIp_Ser_GetStatus(const SomeIp_SerCursor_t *cur)
{
    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* First error, SOMEIP_RES_OK while no error */
    return cur->status;
}


/**
 * @brief   Number of bytes written so far (= payload length for the header Length field).
 *
 * @param[in]  cur  Write cursor.
 *
 * @return  cur->pos, or 0 if @p cur is NULL.
 */
uint32_t SomeIp_Ser_GetLength(const SomeIp_SerCursor_t *cur)
{
    /* Validate parameters */
    if (cur == NULL)
    {
        return 0u;
    }

    /* Bytes written so far = next write position */
    return cur->pos;
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
SomeIp_Result_t SomeIp_Ser_WriteBool(SomeIp_SerCursor_t *cur, uint8_t v)
{
    uint8_t wireValue;

    if (v != 0u)
    {
        wireValue = SOMEIP_BOOL_TRUE;
    }
    else 
    {
        wireValue = SOMEIP_BOOL_FALSE;
    }

    /* boolean = 8 bit, same as uint8 */
    return SomeIp_Ser_WriteU8(cur, wireValue);
}

/**
 * @brief   Write a uint8.
 */
SomeIp_Result_t SomeIp_Ser_WriteU8(SomeIp_SerCursor_t *cur, uint8_t v)
{
    SomeIp_Result_t res;

    /* Validate parameters */
    if (cur == NULL)
    {
        return  SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 1 byte */
    res = SomeIp_Ser_Reserve(cur, 1u);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* uint8: 1 byte, no byte order [Table 4.6] */
    cur->buf[cur->pos] = v;
    cur->pos = cur->pos + 1u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Write a uint16 in cur->order.
 *
 * @details Byte-wise access, so no alignment needed in buf.
 */
SomeIp_Result_t SomeIp_Ser_WriteU16(SomeIp_SerCursor_t *cur, uint16_t v)
{
    SomeIp_Result_t res;

    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 2 bytes */
    res = SomeIp_Ser_Reserve(cur, 2u);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* uint16 in configured byte order [PRS_SOMEIP_00369] */
    SomeIp_WriteU16(&cur->buf[cur->pos], v, cur->order);
    cur->pos += 2u;

    return SOMEIP_RES_OK;
}


/**
 * @brief   Write a uint32 in cur->order.
 *
 */
SomeIp_Result_t SomeIp_Ser_WriteU32(SomeIp_SerCursor_t *cur, uint32_t v)
{
   SomeIp_Result_t res;

    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 4 bytes */
    res = SomeIp_Ser_Reserve(cur, 4u);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* uint32 in configured byte order [PRS_SOMEIP_00369] */
    SomeIp_WriteU32(&cur->buf[cur->pos], v, cur->order);
    cur->pos += 4u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Write a uint64 in cur->order.
 *
 * @details TODO: same as WriteU16 with 8 bytes and SomeIp_WriteU64().
 */
SomeIp_Result_t SomeIp_Ser_WriteU64(SomeIp_SerCursor_t *cur, uint64_t v)
{
   SomeIp_Result_t res;

    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Sticky status + enough space for 8 bytes */
    res = SomeIp_Ser_Reserve(cur, 8u);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* uint64 in configured byte order [PRS_SOMEIP_00369] */
    SomeIp_WriteU64(&cur->buf[cur->pos], v, cur->order);
    cur->pos += 8u;

    return SOMEIP_RES_OK;
}


SomeIp_Result_t SomeIp_Ser_WriteS8(SomeIp_SerCursor_t *cur, int8_t v)
{
    return SomeIp_Ser_WriteU8(cur, (uint8_t)v);
}

SomeIp_Result_t SomeIp_Ser_WriteS16(SomeIp_SerCursor_t *cur, int16_t v)
{
    return SomeIp_Ser_WriteU16(cur, (uint16_t)v);
}

SomeIp_Result_t SomeIp_Ser_WriteS32(SomeIp_SerCursor_t *cur, int32_t v)
{
    return SomeIp_Ser_WriteU32(cur, (uint32_t)v);
}

SomeIp_Result_t SomeIp_Ser_WriteS64(SomeIp_SerCursor_t *cur, int64_t v)
{
    return SomeIp_Ser_WriteU64(cur, (uint64_t)v);
}

SomeIp_Result_t SomeIp_Ser_WriteF32(SomeIp_SerCursor_t *cur, float v)
{
    uint32_t raw;

    /* float32 = IEEE 754 binary32 bit pattern [Table 4.6] */
    (void)memcpy(&raw, &v, sizeof(raw));

    /* Same 4 bytes as uint32, in configured byte order [PRS_SOMEIP_00369] */
    return SomeIp_Ser_WriteU32(cur, raw);
}

SomeIp_Result_t SomeIp_Ser_WriteF64(SomeIp_SerCursor_t *cur, double v)
{
    uint64_t raw;

    /* float64 = IEEE 754 binary64 bit pattern [Table 4.6] */
    (void)memcpy(&raw, &v, sizeof(raw));

    /* Same 8 bytes as uint64, in configured byte order [PRS_SOMEIP_00369] */
    return SomeIp_Ser_WriteU64(cur, raw);
}


/* ==========================================================================================================
 * SERIALIZER - RAW BYTES / ALIGNMENT
 * ======================================================================================================== */

SomeIp_Result_t SomeIp_Ser_WriteBytes(SomeIp_SerCursor_t *cur, const uint8_t *data, uint32_t len)
{
    SomeIp_Result_t res;

    /* Validate parameters */
    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Data may be NULL only for an emty array */
    if ((data == NULL) && (len != 0u))
    {
        cur->status = SOMEIP_RES_E_PARAM;
        return cur->status;
    }

    /* Sticky status + enough space for len bytes */
    res = SomeIp_Ser_Reserve(cur, len);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* Raw copy, no byte order [PRS_SOMEIP_00099] */
    if (len != 0u)
    {
        (void)memcpy(&cur->buf[cur->pos], data, len);
        cur->pos += len;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Insert padding so that the next element is aligned to @p alignBits.
 *
 * @param[in,out] cur        Write cursor.
 * @param[in]     alignBits  8, 16, 32, 64, 128 or 256.
 */
SomeIp_Result_t SomeIp_Ser_Align(SomeIp_SerCursor_t *cur, uint16_t alignBits)
{
    uint32_t n;
    SomeIp_Result_t res;

    if (cur == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* [PRS_SOMEIP_00613] */
    if (SomeIp_IsAlignValid(alignBits) == 0u)
    {
        cur->status = SOMEIP_RES_E_PARAM;
        return cur->status;
    }

    /* [PRS_SOMEIP_00569] */
    n = SomeIp_PaddingSize(cur->msgOffset + cur->pos, alignBits);

    res = SomeIp_Ser_Reserve(cur, n);
    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* [PRS_SOMEIP_00611] */
    if (n != 0u)
    {
        (void)memset(&cur->buf[cur->pos], 0, n);
    }

    return SOMEIP_RES_OK;
}



/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
