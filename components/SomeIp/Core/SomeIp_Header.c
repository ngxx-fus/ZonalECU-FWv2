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

