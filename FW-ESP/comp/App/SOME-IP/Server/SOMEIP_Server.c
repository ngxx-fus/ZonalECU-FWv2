

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Server.h"
#include "SOMEIP_Check.h"
#include "SOMEIP_Header.h"
#include "SOMEIP_Message.h"
#include "SOMEIP_Serialization.h"
#include "SOMEIP_Types.h"

/* PRIVATE FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

static someip_handler_cfg_t const *prv_find_handler(IN someip_server_t *p_server,
                                                    IN someip_service_id_t service_id,
                                                    IN someip_method_id_t method_id);
static someip_result_t prv_send_message(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                        IN someip_header_t *p_hdr);
static someip_result_t prv_reply_error(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                       IN someip_header_t *p_rx, IN someip_method_cfg_t *p_method,
                                       IN someip_return_code_t return_code);
static someip_result_t prv_handle_message(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                          IN someip_message_t *p_msg);

static someip_handler_cfg_t const *prv_find_handler(IN someip_server_t *p_server,
                                                    IN someip_service_id_t service_id,
                                                    IN someip_method_id_t method_id) {
    someip_handler_cfg_t const *p_entry = NULL;
    uint16_t idx;

    /* Search the handler table */
    for (idx = 0U; idx < p_server->cfg.handler_len; idx++) {
        p_entry = &p_server->cfg.p_handlers[idx];

        /* Same method, and a function is registered */
        if ((service_id == p_entry->service_id) && (method_id == p_entry->method_id) &&
            (NULL != p_entry->handler)) {
            /*Handler found */
            return p_entry;
        }
    }

    /* No handler for this method */
    return NULL;
}

static someip_result_t prv_send_message(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                        IN someip_header_t *p_hdr) {
    someip_result_t res;
    uint32_t msg_len;

    /* Write the 16 header bytes in front of the payload */
    res = SOMEIP_HeaderEncode(p_hdr, p_server->cfg.p_tx_buf, p_server->cfg.tx_buf_size);

    /* Header could not be encoded */
    if (SOMEIP_RES_OK != res) {
        /* Forward the encode error */
        return res;
    }

    /* Length counts from Request ID [PRS_SOMEIP_00042], the whole message must fit in p_tx_buf */
    if (p_hdr->length > (p_server->cfg.tx_buf_size - (uint32_t)SOMEIP_HEADER_PART1_SIZE)) {
        /* Message bigger than the transmit buffer */
        return SOMEIP_RES_E_NO_SPACE;
    }

    msg_len = (uint32_t)SOMEIP_HEADER_PART1_SIZE + p_hdr->length;

    /* Return the result of the transport */
    return p_server->cfg.send(p_peer, p_server->cfg.p_tx_buf, msg_len, p_server->cfg.p_ctx);
}

static someip_result_t prv_reply_error(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                       IN someip_header_t *p_rx, IN someip_method_cfg_t *p_method,
                                       IN someip_return_code_t return_code) {
    someip_header_t tx_hdr;
    someip_result_t res;

    /* Only a REQUEST expecting a response may get an ERROR [PRS_SOMEIP_00188] [PRS_SOMEIP_00189]
     * [PRS_SOMEIP_00537] [PRS_SOMEIP_00539] */
    if (0U == SOMEIP_CheckShouldReplyError(p_rx, p_method)) {
        /* Drop silently: not answering is the expected behavior, not a failure */
        return SOMEIP_RES_OK;
    }

    /* ERROR header without payload: the request payload is never copied [PRS_SOMEIP_00190] */
    res = SOMEIP_MessageBuildError(&tx_hdr, p_rx, return_code, 0U);

    /* Header could not be built */
    if (SOMEIP_RES_OK != res) {
        /* Forward the build error */
        return res;
    }

    /* Return the result of the send */
    return prv_send_message(p_server, p_peer, &tx_hdr);
}

static someip_result_t prv_handle_message(IN someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                          IN someip_message_t *p_msg) {
    someip_method_cfg_t const *p_method = NULL;
    someip_handler_cfg_t const *p_handler = NULL;
    someip_ser_cursor_t cursor;
    someip_header_t tx_hdr;
    someip_return_code_t rc = SOMEIP_E_OK;
    someip_message_type_t msg_type;
    someip_result_t res;
    uint32_t payload_len;

    /* Check the header against the configuration (Figure 4.21) [PRS_SOMEIP_00195] */
    res = SOMEIP_CheckHeader(&p_msg->header, p_server->cfg.p_services, p_server->cfg.service_len, &rc,
                             &p_method);

    /* Check could not run */
    if (SOMEIP_RES_OK != res) {
        /* Forward the check error */
        return res;
    }

    /* Header failed a check */
    if (SOMEIP_E_OK != rc) {
        /* Answer with the Return Code of the first failing check */
        return prv_reply_error(p_server, p_peer, &p_msg->header, p_method, rc);
    }

    msg_type = p_msg->header.message_type;

    /* SOME/IP-TP segment: reassembly is not supported yet (Figure 4.21) */
    if (0U != (msg_type & (someip_message_type_t)SOMEIP_MT_FLAG_TP)) {
        /* Reject the segment */
        return prv_reply_error(p_server, p_peer, &p_msg->header, p_method, SOMEIP_E_WRONG_MESSAGE_TYPE);
    }

    /* A server only serves REQUEST and REQUEST_NO_RETURN */
    if (((someip_message_type_t)SOMEIP_MT_REQUEST != msg_type) &&
        ((someip_message_type_t)SOMEIP_MT_REQUEST_NO_RETURN != msg_type)) {
        /* Nothing to do, drop the message */
        return SOMEIP_RES_OK;
    }

    p_handler = prv_find_handler(p_server, p_msg->header.service_id, p_msg->header.method_id);

    /* Method is configured but no application function is registered (Table 4.11) */
    if (NULL == p_handler) {
        /* Application not running */
        return prv_reply_error(p_server, p_peer, &p_msg->header, p_method, SOMEIP_E_NOT_READY);
    }

    /* Output arguments are written right after the space of the header */
    res = SOMEIP_SerInit(&cursor, &p_server->cfg.p_tx_buf[SOMEIP_HEADER_SIZE],
                         p_server->cfg.tx_buf_size - (uint32_t)SOMEIP_HEADER_SIZE,
                         (uint32_t)SOMEIP_OFS_PAYLOAD, SOMEIP_BYTE_ORDER_BIG_ENDIAN);

    /* Cursor could not be initialized */
    if (SOMEIP_RES_OK != res) {
        /* Forward the init error */
        return res;
    }

    rc = p_handler->handler(p_msg, &cursor, p_server->cfg.p_ctx);

    /* Fire & forget is never answered, not even with an error [PRS_SOMEIP_00171] [PRS_SOMEIP_00189] */
    if ((someip_message_type_t)SOMEIP_MT_REQUEST_NO_RETURN == msg_type) {
        /* Done */
        return SOMEIP_RES_OK;
    }

    /* Handler wrote more than the buffer, the payload is incomplete */
    if (SOMEIP_RES_OK != SOMEIP_SerGetStatus(&cursor)) {
        /* Unspecified error on the server side */
        return prv_reply_error(p_server, p_peer, &p_msg->header, p_method, SOMEIP_E_NOT_OK);
    }

    payload_len = SOMEIP_SerGetLength(&cursor);

    /* Request payload cannot be deserialized (Figure 4.21) */
    if (SOMEIP_E_MALFORMED_MESSAGE == rc) {
        /* ERROR without payload: what the handler wrote is not error specific payload */
        res = SOMEIP_MessageBuildError(&tx_hdr, &p_msg->header, rc, 0U);
    }
    /* Application error configured to use an ERROR message [PRS_SOMEIP_00903] */
    else if ((SOMEIP_E_OK != rc) && (0U != p_handler->use_error_message)) {
        /* ERROR with the error specific payload */
        res = SOMEIP_MessageBuildError(&tx_hdr, &p_msg->header, rc, payload_len);
    }
    /* Success, or application error carried by the RESPONSE (note of [PRS_SOMEIP_00901]) */
    else {
        /* RESPONSE with the output arguments */
        res = SOMEIP_MessageBuildResponse(&tx_hdr, &p_msg->header, rc, payload_len);
    }

    /* Header could not be built */
    if (SOMEIP_RES_OK != res) {
        /* Forward the build error */
        return res;
    }

    /* Return the result of the send */
    return prv_send_message(p_server, p_peer, &tx_hdr);
}

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

someip_result_t SOMEIP_ServerInit(OUT someip_server_t *p_server, IN someip_server_cfg_t *p_cfg) {
    /* Server, configuration and transport are mandatory */
    if ((NULL == p_server) || (NULL == p_cfg) || (NULL == p_cfg->send)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* A table may only be NULL when it is empty */
    if (((NULL == p_cfg->p_services) && (0U < p_cfg->service_len)) ||
        ((NULL == p_cfg->p_handlers) && (0U < p_cfg->handler_len))) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Transmit buffer must hold at least a header without payload */
    if ((NULL == p_cfg->p_tx_buf) || ((uint32_t)SOMEIP_HEADER_SIZE > p_cfg->tx_buf_size)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    p_server->cfg = *p_cfg;

    /* Server ready */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_ServerProcess(INOUT someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                     IN uint8_t *p_buf, IN uint32_t buf_len) {
    someip_message_t msg;
    someip_header_t rx;
    someip_result_t res;
    someip_result_t final_res = SOMEIP_RES_OK;
    uint32_t pos = 0U;
    uint32_t remaining;

    /* Server, sender and datagram are mandatory */
    if ((NULL == p_server) || (NULL == p_peer) || (NULL == p_buf)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* One datagram may hold several messages, each ends at its Length field [PRS_SOMEIP_00535]
     * [PRS_SOMEIP_00140] */
    while (pos < buf_len) {
        remaining = buf_len - pos;
        res = SOMEIP_MessageNext(p_buf, buf_len, &pos, &msg);

        /* [PRS_SOMEIP_00910] failed: the end of this message is unknown */
        if (SOMEIP_RES_OK != res) {
            /* Answer only if there is a full header to copy the IDs from */
            if (((uint32_t)SOMEIP_HEADER_SIZE <= remaining) &&
                (SOMEIP_RES_OK == SOMEIP_HeaderDecode(&p_buf[pos], remaining, &rx))) {
                /* Malformed error, a failed send changes nothing: the datagram is dropped anyway */
                (void)prv_reply_error(p_server, p_peer, &rx, NULL, SOMEIP_E_MALFORMED_MESSAGE);
            }

            /* The rest of the datagram cannot be split, drop it */
            return SOMEIP_RES_E_MALFORMED;
        }

        /* pos is already on the next message */
        res = prv_handle_message(p_server, p_peer, &msg);

        /* Answer failed: remember it, the next messages are still processed */
        if (SOMEIP_RES_OK != res) {
            final_res = res;
        }
    }

    /* All messages processed */
    return final_res;
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
