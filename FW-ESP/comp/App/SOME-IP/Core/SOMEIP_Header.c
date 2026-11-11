

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Header.h"
#include "SOMEIP_Endian.h"
#include "SOMEIP_Types.h"

/* FUNCTION IMPLEMENTATIONS ******************************************************************************************************
 ********************************************************************************************************************************/

someip_result_t SOMEIP_HeaderDecode(IN uint8_t *p_buf, IN uint32_t buf_len, OUT someip_header_t *p_hdr) {
    /* Check NULL */
    if ((NULL == p_buf) || (NULL == p_hdr)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check Length of Header layout */
    if ((uint32_t)SOMEIP_HEADER_SIZE > buf_len) {
        /* Input too short */
        return SOMEIP_RES_E_SHORT_INPUT;
    }

    /* Decode Service ID */
    p_hdr->service_id = SOMEIP_ReadU16Be(&p_buf[SOMEIP_OFS_SERVICE_ID]);

    /* Decode Method ID */
    p_hdr->method_id = SOMEIP_ReadU16Be(&p_buf[SOMEIP_OFS_METHOD_ID]);

    /* Decode Length */
    p_hdr->length = SOMEIP_ReadU32Be(&p_buf[SOMEIP_OFS_LENGTH]);

    /* Decode Client ID */
    p_hdr->client_id = SOMEIP_ReadU16Be(&p_buf[SOMEIP_OFS_CLIENT_ID]);

    /* Decode Session ID */
    p_hdr->session_id = SOMEIP_ReadU16Be(&p_buf[SOMEIP_OFS_SESSION_ID]);

    /* Decode Protocol Version */
    p_hdr->protocol_version = p_buf[SOMEIP_OFS_PROTOCOL_VERSION];

    /* Decode Interface Version */
    p_hdr->interface_version = p_buf[SOMEIP_OFS_INTERFACE_VERSION];

    /* Decode Message Type */
    p_hdr->message_type = p_buf[SOMEIP_OFS_MESSAGE_TYPE];

    /* Decode Return Code */
    p_hdr->return_code = p_buf[SOMEIP_OFS_RETURN_CODE];

    /* Header decoded */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_HeaderEncode(IN someip_header_t *p_hdr, OUT uint8_t *p_buf, IN uint32_t buf_size) {
    /* Check NULL */
    if ((NULL == p_hdr) || (NULL == p_buf)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check SOME/IP length */
    if ((uint32_t)SOMEIP_LENGTH_MIN > p_hdr->length) {
        /* Invalid Length field */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check buffer capacity */
    if ((uint32_t)SOMEIP_HEADER_SIZE > buf_size) {
        /* Buffer too small */
        return SOMEIP_RES_E_NO_SPACE;
    }

    /* Encode Service ID */
    SOMEIP_WriteU16Be(&p_buf[SOMEIP_OFS_SERVICE_ID], p_hdr->service_id);

    /* Encode Method ID */
    SOMEIP_WriteU16Be(&p_buf[SOMEIP_OFS_METHOD_ID], p_hdr->method_id);

    /* Encode Length */
    SOMEIP_WriteU32Be(&p_buf[SOMEIP_OFS_LENGTH], p_hdr->length);

    /* Encode Client ID */
    SOMEIP_WriteU16Be(&p_buf[SOMEIP_OFS_CLIENT_ID], p_hdr->client_id);

    /* Encode Session ID */
    SOMEIP_WriteU16Be(&p_buf[SOMEIP_OFS_SESSION_ID], p_hdr->session_id);

    /* Encode Protocol Version */
    p_buf[SOMEIP_OFS_PROTOCOL_VERSION] = p_hdr->protocol_version;

    /* Encode Interface Version */
    p_buf[SOMEIP_OFS_INTERFACE_VERSION] = p_hdr->interface_version;

    /* Encode Message Type */
    p_buf[SOMEIP_OFS_MESSAGE_TYPE] = p_hdr->message_type;

    /* Encode Return Code */
    p_buf[SOMEIP_OFS_RETURN_CODE] = p_hdr->return_code;

    /* Header encoded */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_MessageNext(IN uint8_t *p_buf, IN uint32_t buf_len, INOUT uint32_t *p_pos, OUT someip_message_t *p_msg) {
    someip_result_t res = SOMEIP_RES_OK;
    uint32_t remaining = 0U;

    /* Check parameters */
    if ((NULL == p_buf) || (NULL == p_pos) || (NULL == p_msg) || (*p_pos > buf_len)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Calculate remaining bytes */
    remaining = buf_len - *p_pos;

    /* Check minimum Header size */
    if ((uint32_t)SOMEIP_HEADER_SIZE > remaining) {
        /* Malformed message */
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Decode SOME/IP Header */
    res = SOMEIP_HeaderDecode(&p_buf[*p_pos], remaining, &p_msg->header);

    /* Stop on decode error */
    if (SOMEIP_RES_OK != res) {
        /* Propagate the error */
        return res;
    }

    /* Check minimum Length */
    if ((uint32_t)SOMEIP_LENGTH_MIN > p_msg->header.length) {
        /* Malformed message */
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Check message size */
    if (p_msg->header.length > (remaining - (uint32_t)SOMEIP_HEADER_PART1_SIZE)) {
        /* Malformed message */
        return SOMEIP_RES_E_MALFORMED;
    }

    /* Set Payload pointer and length */
    p_msg->p_payload = &p_buf[*p_pos + (uint32_t)SOMEIP_OFS_PAYLOAD];
    p_msg->payload_len = p_msg->header.length - (uint32_t)SOMEIP_HEADER_PART2_SIZE;

    /* Update position */
    *p_pos += (uint32_t)SOMEIP_HEADER_PART1_SIZE + p_msg->header.length;

    /* Message extracted */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_TpHeaderDecode(IN uint8_t *p_buf, IN uint32_t buf_len, OUT someip_tp_header_t *p_tp) {
    uint32_t raw = 0U;

    /* Check parameters */
    if ((NULL == p_buf) || (NULL == p_tp)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check input length */
    if ((uint32_t)SOMEIP_TP_HEADER_SIZE > buf_len) {
        /* Input too short */
        return SOMEIP_RES_E_SHORT_INPUT;
    }

    /* Read 32-bit TP Header */
    raw = SOMEIP_ReadU32Be(p_buf);

    /* Extract Offset */
    p_tp->offset = raw & SOMEIP_TP_OFFSET_MASK;

    /* Extract More Segments */
    p_tp->more_segments = (0U != (raw & SOMEIP_TP_MORE_SEGMENTS_MASK)) ? 1U : 0U;

    /* TP header decoded */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_TpHeaderEncode(IN someip_tp_header_t *p_tp, OUT uint8_t *p_buf, IN uint32_t buf_size) {
    uint32_t raw = 0U;

    /* Parameter validation */
    if ((NULL == p_tp) || (NULL == p_buf)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Validate offset: must be a multiple of 16 */
    if (0U != (p_tp->offset & ~SOMEIP_TP_OFFSET_MASK)) {
        /* Invalid offset */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check buffer capacity */
    if ((uint32_t)SOMEIP_TP_HEADER_SIZE > buf_size) {
        /* Buffer too small */
        return SOMEIP_RES_E_NO_SPACE;
    }

    /* Build 32-bit TP header */
    raw = p_tp->offset | ((0U != p_tp->more_segments) ? SOMEIP_TP_MORE_SEGMENTS_MASK : 0U);

    /* Write as Big Endian */
    SOMEIP_WriteU32Be(p_buf, raw);

    /* TP header encoded */
    return SOMEIP_RES_OK;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/