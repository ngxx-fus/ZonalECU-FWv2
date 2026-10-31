/**
 ************************************************************************************************************
 * @file    SomeIp_Header.c
 * @brief   SOME/IP header codec.
 *
 * @details This file provides the functions to:
 *          - Decode / encode the 16-byte SOME/IP header             [PRS_SOMEIP_00030] [PRS_SOMEIP_00368]
 *          - Split one UDP datagram / TCP chunk into SOME/IP messages [PRS_SOMEIP_00535] [PRS_SOMEIP_00910]
 *          - Decode / encode the 4-byte SOME/IP-TP header             [PRS_SOMEIP_00723] [PRS_SOMEIP_00931]
 *
 *          Scope: STRUCTURE only (sizes, Length field consistency).
 *          Protocol Version / Service / Method / Message Type checks belong to SomeIp_Validate (Figure 4.21).
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.2 / 4.2.1 / 4.2.1.4
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "SomeIp_Header.h"
#include "SomeIp_Endian.h"
#include "SomeIp_Types.h"
#include <stdint.h>

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

/* PUBLIC VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS *********************************************************************************
 ************************************************************************************************************/

/**
 * @brief   Decode the 16-byte SOME/IP header into a SomeIp_Header_t.
 *
 * @details Reads every field at its fixed offset (SOMEIP_OFS_*) and converts the 16/32-bit fields from
 *          network byte order (big-endian) to host byte order. Access is byte-wise, so @p buf may be
 *          unaligned [PRS_SOMEIP_00142].
 *
 * @param[in]  buf  Pointer to the first byte of the SOME/IP header.
 * @param[in]  len  Number of valid bytes available at @p buf.
 * @param[out] hdr  Decoded header (host byte order).
 *
 * @retval SOMEIP_RES_OK             Header decoded successfully.
 * @retval SOMEIP_RES_E_PARAM        @p buf or @p hdr is NULL.
 * @retval SOMEIP_RES_E_SHORT_INPUT  @p len is less than SOMEIP_HEADER_SIZE (16).
 *
 * @note    Field values are NOT validated (Protocol Version, Message Type, ...). That is the job of
 *          SomeIp_Validate (Figure 4.21), so that an error response can still echo the Request ID
 *          [PRS_SOMEIP_00704].
 * @note    The Length field is NOT checked against @p len; see SomeIp_Message_Next().
 *
 * @see     [PRS_SOMEIP_00030] [PRS_SOMEIP_00031] [PRS_SOMEIP_00368], Figure 4.1
 */
SomeIp_Result_t SomeIp_Header_Decode(const uint8_t *buf, uint32_t len, SomeIp_Header_t *hdr)
{
    /* Check NULL */
    if (buf == NULL || hdr == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Check Length of Header layout */
    if (len < SOMEIP_HEADER_SIZE)
    {
        return SOMEIP_RES_E_SHORT_INPUT;
    }

    /* Decode Service ID */
    hdr->serviceId = SomeIp_ReadU16_BE(&buf[SOMEIP_OFS_SERVICE_ID]);

    /* Decode Method ID */
    hdr->methodId = SomeIp_ReadU16_BE(&buf[SOMEIP_OFS_METHOD_ID]);

    /* Decode Length */
    hdr->length = SomeIp_ReadU32_BE(&buf[SOMEIP_OFS_LENGTH]);

    /* Decode Client ID */
    hdr->clientId = SomeIp_ReadU16_BE(&buf[SOMEIP_OFS_CLIENT_ID]);

    /* Decode Session ID */
    hdr->sessionId = SomeIp_ReadU16_BE(&buf[SOMEIP_OFS_SESSION_ID]);

    /* Decode Protocol Version */
    hdr->protocolVersion = buf[SOMEIP_OFS_PROTOCOL_VERSION];

    /* Decode Interface Version */
    hdr->interfaceVersion = buf[SOMEIP_OFS_INTERFACE_VERSION];

    /* Decode Message Type */
    hdr->messageType = buf[SOMEIP_OFS_MESSAGE_TYPE];

    /* Decode Return Code */
    hdr->returnCode = buf[SOMEIP_OFS_RETURN_CODE];

    return SOMEIP_RES_OK;
}

/**
 * @brief   Encode a SomeIp_Header_t into 16 bytes (big-endian).
 *
 * @details Mirror of SomeIp_Header_Decode(): every field is written at its fixed offset (SOMEIP_OFS_*),
 *          16/32-bit fields in network byte order. The payload is NOT written.
 *          All checks are done before the first byte is written, so @p buf is untouched on error.
 *
 * @param[in]  hdr  Header to encode. hdr->length must be SOMEIP_HEADER_PART2_SIZE + payload length.
 * @param[out] buf  Destination buffer (may be unaligned).
 * @param[in]  cap  Capacity of @p buf in bytes.
 *
 * @retval SOMEIP_RES_OK          Header encoded successfully (16 bytes written).
 * @retval SOMEIP_RES_E_PARAM     @p hdr or @p buf is NULL, or hdr->length < SOMEIP_LENGTH_MIN (8).
 * @retval SOMEIP_RES_E_NO_SPACE  @p cap is less than SOMEIP_HEADER_SIZE (16).
 *
 * @note    Field values are written as given. Setting the correct Protocol Version, Message Type,
 *          Return Code and copying the Request ID [PRS_SOMEIP_00704] is the caller's responsibility.
 *
 * @see     [PRS_SOMEIP_00030] [PRS_SOMEIP_00042] [PRS_SOMEIP_00368]
 */
SomeIp_Result_t SomeIp_Header_Encode(const SomeIp_Header_t *hdr, uint8_t *buf, uint32_t cap)
{
    /* Check NULL */
    if (hdr == NULL || buf == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Check SOME/IP length */
    if (hdr->length < SOMEIP_LENGTH_MIN)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Check buffer capacity */
    if (cap < SOMEIP_HEADER_SIZE)
    {
        return SOMEIP_RES_E_NO_SPACE;
    }

    /* Encode Service ID */
    SomeIp_WriteU16_BE(&buf[SOMEIP_OFS_SERVICE_ID], hdr->serviceId);

    /* Encode Method ID */
    SomeIp_WriteU16_BE(&buf[SOMEIP_OFS_METHOD_ID], hdr->methodId);

    /* Encode Length */
    SomeIp_WriteU32_BE(&buf[SOMEIP_OFS_LENGTH], hdr->length);

    /* Encode Client ID */
    SomeIp_WriteU16_BE(&buf[SOMEIP_OFS_CLIENT_ID], hdr->clientId);

    /* Encode Session ID */
    SomeIp_WriteU16_BE(&buf[SOMEIP_OFS_SESSION_ID], hdr->sessionId);

    /* Encode Protocol Version */
    buf[SOMEIP_OFS_PROTOCOL_VERSION] = hdr->protocolVersion;

    /* Encode Interface Version */
    buf[SOMEIP_OFS_INTERFACE_VERSION] = hdr->interfaceVersion;

    /* Encode Message Type */
    buf[SOMEIP_OFS_MESSAGE_TYPE] = hdr->messageType;

    /* Encode Return Code */
    buf[SOMEIP_OFS_RETURN_CODE] = hdr->returnCode;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Get the next SOME/IP message from a buffer that may hold several messages back to back.
 *
 * @details One UDP datagram / TCP segment may carry more than one SOME/IP message [PRS_SOMEIP_00535].
 *          The end of each message is found with its Length field [PRS_SOMEIP_00140].
 *          Checks performed [PRS_SOMEIP_00910]:
 *          - remaining bytes >= SOMEIP_HEADER_SIZE
 *          - Length          >= SOMEIP_LENGTH_MIN
 *          - Length          <= remaining - SOMEIP_HEADER_PART1_SIZE  (written as a subtraction: no overflow)
 *
 *          Usage:
 *          @code
 *          uint32_t pos = 0u;
 *          while (pos < len)
 *          {
 *              SomeIp_Message_t msg;
 *              if (SomeIp_Message_Next(buf, len, &pos, &msg) != SOMEIP_RES_OK)
 *              {
 *                  break;      // malformed: drop the rest of the datagram
 *              }
 *              // ... process msg ...
 *          }
 *          @endcode
 *
 * @param[in]     buf  Received datagram / stream chunk.
 * @param[in]     len  Number of valid bytes in @p buf.
 * @param[in,out] pos  In: offset of the message to read. Out: offset of the next message (success only).
 * @param[out]    msg  Decoded header, payload pointer and payload length.
 *
 * @retval SOMEIP_RES_OK           Message extracted, @p pos advanced by SOMEIP_HEADER_PART1_SIZE + Length.
 * @retval SOMEIP_RES_E_PARAM      @p buf, @p pos or @p msg is NULL, or *pos > @p len.
 * @retval SOMEIP_RES_E_MALFORMED  One of the [PRS_SOMEIP_00910] checks failed.
 *
 * @note    Zero-copy: msg->payload points INTO @p buf and is only valid while @p buf is alive.
 *          When msg->payloadLength is 0, msg->payload must not be dereferenced.
 * @note    On error *pos is unchanged; msg content must not be used.
 *
 * @see     [PRS_SOMEIP_00140] [PRS_SOMEIP_00142] [PRS_SOMEIP_00535] [PRS_SOMEIP_00910]
 */
SomeIp_Result_t SomeIp_Message_Next(const uint8_t *buf, uint32_t len, uint32_t *pos, SomeIp_Message_t *msg)
{
    SomeIp_Result_t res;
    uint32_t remaining;

    /* Check parameters */
    if (buf == NULL || pos == NULL || msg == NULL || *pos > len)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Calculate remaining bytes */
    remaining = len - *pos;

    /* Check minimum Header size */
    if (remaining < SOMEIP_HEADER_SIZE)
    {
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Decode SOME/IP Header */
    res = SomeIp_Header_Decode(&buf[*pos], remaining, &msg->header);

    if (res != SOMEIP_RES_OK)
    {
        return res;
    }

    /* Check minimum Length */
    if (msg->header.length < SOMEIP_LENGTH_MIN)
    {
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Check message size */
    if (msg->header.length > (remaining - SOMEIP_HEADER_PART1_SIZE))
    {
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Set Payload pointer and length */
    msg->payload = &buf[*pos + SOMEIP_OFS_PAYLOAD];

    msg->payloadLength = msg->header.length - SOMEIP_HEADER_PART2_SIZE;

    /* Update position */
    *pos += SOMEIP_HEADER_PART1_SIZE + msg->header.length;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Decode the 4-byte SOME/IP-TP header found at the start of the payload of a TP segment.
 *
 * @details Only called for messages with the TP-Flag (0x20) set in the Message Type [PRS_SOMEIP_00367].
 *          Layout (big-endian [PRS_SOMEIP_00931]), bits from highest to lowest [PRS_SOMEIP_00723]:
 *          @verbatim
 *           31                                4   3   2   1   0
 *          +-----------------------------------+---+---+---+---+
 *          |          Offset [28 bit]          | R | R | R | M |
 *          +-----------------------------------+---+---+---+---+
 *          @endverbatim
 *          - offset       = raw & SOMEIP_TP_OFFSET_MASK, already in bytes, no shift  [PRS_SOMEIP_00724] [PRS_SOMEIP_00725]
 *          - moreSegments = raw & SOMEIP_TP_MORE_SEGMENTS_MASK                      [PRS_SOMEIP_00727]
 *          - Reserved bits are ignored and not checked                              [PRS_SOMEIP_00726]
 *
 * @param[in]  p    Pointer to the TP header (= msg->payload of the segment).
 * @param[in]  len  Number of valid bytes available at @p p (= msg->payloadLength).
 * @param[out] tp   Decoded TP header.
 *
 * @retval SOMEIP_RES_OK             TP header decoded successfully.
 * @retval SOMEIP_RES_E_PARAM        @p p or @p tp is NULL.
 * @retval SOMEIP_RES_E_SHORT_INPUT  @p len is less than SOMEIP_TP_HEADER_SIZE (4).
 *
 * @note    tp->offset is not range-checked here. The reassembly module must check
 *          offset + segment length against its buffer size before copying.
 *
 * @see     [PRS_SOMEIP_00723] .. [PRS_SOMEIP_00727] [PRS_SOMEIP_00931], Figure 4.16
 */
SomeIp_Result_t SomeIp_TpHeader_Decode(const uint8_t *p, uint32_t len, SomeIp_TpHeader_t *tp)
{
    uint32_t raw;

    /* Check parameters */
    if (p == NULL || tp == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Check input length */
    if (len < SOMEIP_TP_HEADER_SIZE)
    {
        return SOMEIP_RES_E_SHORT_INPUT;
    }

    /* Read 32-bit TP Header */
    raw = SomeIp_ReadU32_BE(p);

    /* Extract Offset */
    tp->offset = raw & SOMEIP_TP_OFFSET_MASK;

    /* Extract More Segments */
    tp->moreSegments = ((raw & SOMEIP_TP_MORE_SEGMENTS_MASK) != 0u) ? 1u : 0u;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Encode a SomeIp_TpHeader_t into 4 bytes (big-endian).
 *
 * @details raw = offset | (moreSegments ? SOMEIP_TP_MORE_SEGMENTS_MASK : 0)
 *          - offset is written as is (in bytes, no shift)          [PRS_SOMEIP_00725]
 *          - Reserved bits are always written as 0                  [PRS_SOMEIP_00726]
 *          - More Segments = 1 for all segments but the last one    [PRS_SOMEIP_00727]
 *          All checks are done before writing, so @p p is untouched on error.
 *
 * @param[in]  tp   TP header to encode. tp->offset must be a multiple of 16.
 * @param[out] p    Destination (normally SOMEIP_OFS_PAYLOAD of the segment buffer).
 * @param[in]  cap  Capacity of @p p in bytes.
 *
 * @retval SOMEIP_RES_OK          TP header encoded successfully (4 bytes written).
 * @retval SOMEIP_RES_E_PARAM     @p tp or @p p is NULL, or tp->offset is not a multiple of 16 [PRS_SOMEIP_00724].
 * @retval SOMEIP_RES_E_NO_SPACE  @p cap is less than SOMEIP_TP_HEADER_SIZE (4).
 *
 * @see     [PRS_SOMEIP_00723] .. [PRS_SOMEIP_00729] [PRS_SOMEIP_00931], Figure 4.16
 */
SomeIp_Result_t SomeIp_TpHeader_Encode(const SomeIp_TpHeader_t *tp, uint8_t *p, uint32_t cap)
{
    uint32_t raw;

    /* Parameter validation */
    if (tp == NULL || p == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Validate offset: must be a multiple of 16 */
    if ((tp->offset & ~SOMEIP_TP_OFFSET_MASK) != 0u)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Check buffer capacity */
    if (cap < SOMEIP_TP_HEADER_SIZE)
    {
        return SOMEIP_RES_E_NO_SPACE;
    }

    /* Build 32-bit TP header */
    raw = tp->offset | (tp->moreSegments ? SOMEIP_TP_MORE_SEGMENTS_MASK : 0u);

    /* Write as Big Endian */
    SomeIp_WriteU32_BE(p, raw);
    
    return SOMEIP_RES_OK;
}

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/

/* EOF ******************************************************************************************************
 ************************************************************************************************************/

