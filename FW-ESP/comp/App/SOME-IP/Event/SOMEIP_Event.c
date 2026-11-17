

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Event.h"
#include "SOMEIP_Header.h"
#include "SOMEIP_Message.h"
#include "SOMEIP_Serialization.h"
#include "SOMEIP_Session.h"
#include "SOMEIP_Types.h"

/* PRIVATE FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

static uint16_t prv_find_event(IN someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                               IN someip_event_id_t event_id);
static bool prv_is_same_peer(IN someip_endpoint_t *p_a, IN someip_endpoint_t *p_b);
static someip_result_t prv_send_event(INOUT someip_publisher_t *p_pub, IN uint16_t event_idx,
                                      IN someip_endpoint_t *p_peer);

static uint16_t prv_find_event(IN someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                               IN someip_event_id_t event_id) {
    someip_event_cfg_t const *p_event = NULL;
    uint16_t idx;

    /* Search the event table */
    for (idx = 0U; idx < p_pub->cfg.event_len; idx++) {
        p_event = &p_pub->cfg.p_events[idx];

        /* Same Message ID */
        if ((service_id == p_event->service_id) && (event_id == p_event->event_id)) {
            /* Event found */
            return idx;
        }
    }

    /* Not configured: event_len is never a valid index */
    return p_pub->cfg.event_len;
}

static bool prv_is_same_peer(IN someip_endpoint_t *p_a, IN someip_endpoint_t *p_b) {
    uint8_t idx;

    /* Different transport or port */
    if ((p_a->transport != p_b->transport) || (p_a->port != p_b->port)) {
        /* Other peer */
        return false;
    }

    /* Compare the IPv4 address */
    for (idx = 0U; idx < (uint8_t)SOMEIP_IPV4_ADDR_SIZE; idx++) {
        /* One byte differs */
        if (p_a->ipv4[idx] != p_b->ipv4[idx]) {
            /* Other peer */
            return false;
        }
    }

    /* Same endpoint */
    return true;
}

static someip_result_t prv_send_event(INOUT someip_publisher_t *p_pub, IN uint16_t event_idx,
                                      IN someip_endpoint_t *p_peer) {
    someip_event_cfg_t const *p_event = &p_pub->cfg.p_events[event_idx];
    someip_subscriber_t const *p_sub = NULL;
    someip_ser_cursor_t cursor;
    someip_header_t tx_hdr;
    someip_session_id_t session_id;
    someip_result_t res;
    someip_result_t final_res = SOMEIP_RES_OK;
    uint32_t payload_len;
    uint32_t msg_len;
    uint16_t idx;
    uint16_t sub_cnt = 0U;

    /* One peer given: only this peer gets the message (initial value of a field) */
    if (NULL != p_peer) {
        sub_cnt = 1U;
    }
    /* No peer given: count the subscribers of the event */
    else {
        for (idx = 0U; idx < p_pub->cfg.sub_len; idx++) {
            /* Entry belongs to this event */
            if ((0U != p_pub->cfg.p_subs[idx].used) && (event_idx == p_pub->cfg.p_subs[idx].event_idx)) {
                sub_cnt++;
            }
        }
    }

    /* Nobody listens: no transmission, so the Session ID stays [PRS_SOMEIP_00925] */
    if (0U == sub_cnt) {
        /* Nothing to do */
        return SOMEIP_RES_OK;
    }

    /* Payload is written right after the space of the header */
    res = SOMEIP_SerInit(&cursor, &p_pub->cfg.p_tx_buf[SOMEIP_HEADER_SIZE],
                         p_pub->cfg.tx_buf_size - (uint32_t)SOMEIP_HEADER_SIZE,
                         (uint32_t)SOMEIP_OFS_PAYLOAD, SOMEIP_BYTE_ORDER_BIG_ENDIAN);

    /* Cursor could not be initialized */
    if (SOMEIP_RES_OK != res) {
        /* Forward the init error */
        return res;
    }

    /* Payload is the serialized value of the event [PRS_SOMEIP_00926] */
    res = p_event->serialize(&cursor, p_pub->cfg.p_ctx);

    /* Application has no value to send */
    if (SOMEIP_RES_OK != res) {
        /* Forward the application error */
        return res;
    }

    /* Value bigger than the transmit buffer, the payload is incomplete */
    if (SOMEIP_RES_OK != SOMEIP_SerGetStatus(&cursor)) {
        /* Do not send a cut value */
        return SOMEIP_RES_E_NO_SPACE;
    }

    payload_len = SOMEIP_SerGetLength(&cursor);

    /* Session ID counts once per transmission [PRS_SOMEIP_00925] [PRS_SOMEIP_00932] [PRS_SOMEIP_00933]
     * [PRS_SOMEIP_00521] */
    res = SOMEIP_SessionNext(&p_pub->cfg.p_states[event_idx].session, &session_id);

    /* Session ID could not be taken */
    if (SOMEIP_RES_OK != res) {
        /* Forward the session error */
        return res;
    }

    res = SOMEIP_MessageBuildNotification(&tx_hdr, p_event->service_id, p_event->event_id, session_id,
                                          p_event->interface_version, payload_len);

    /* Header could not be built */
    if (SOMEIP_RES_OK != res) {
        /* Forward the build error */
        return res;
    }

    /* Write the 16 header bytes in front of the payload */
    res = SOMEIP_HeaderEncode(&tx_hdr, p_pub->cfg.p_tx_buf, p_pub->cfg.tx_buf_size);

    /* Header could not be encoded */
    if (SOMEIP_RES_OK != res) {
        /* Forward the encode error */
        return res;
    }

    msg_len = (uint32_t)SOMEIP_HEADER_SIZE + payload_len;

    /* One peer given: send to it only */
    if (NULL != p_peer) {
        /* Return the result of the transport */
        return p_pub->cfg.send(p_peer, p_pub->cfg.p_tx_buf, msg_len, p_pub->cfg.p_ctx);
    }

    /* Same message to every subscriber, each peer is stored once [PRS_SOMEIP_00930] */
    for (idx = 0U; idx < p_pub->cfg.sub_len; idx++) {
        p_sub = &p_pub->cfg.p_subs[idx];

        /* Entry belongs to this event */
        if ((0U != p_sub->used) && (event_idx == p_sub->event_idx)) {
            res = p_pub->cfg.send(&p_sub->peer, p_pub->cfg.p_tx_buf, msg_len, p_pub->cfg.p_ctx);

            /* Send failed: remember it, the next subscribers still get the message */
            if (SOMEIP_RES_OK != res) {
                final_res = res;
            }
        }
    }

    /* All subscribers processed */
    return final_res;
}

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

someip_result_t SOMEIP_PublisherInit(OUT someip_publisher_t *p_pub, IN someip_publisher_cfg_t *p_cfg) {
    someip_result_t res;
    uint16_t idx;

    /* Publisher, configuration and transport are mandatory */
    if ((NULL == p_pub) || (NULL == p_cfg) || (NULL == p_cfg->send)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* A table may only be NULL when it is empty */
    if ((((NULL == p_cfg->p_events) || (NULL == p_cfg->p_states)) && (0U < p_cfg->event_len)) ||
        ((NULL == p_cfg->p_subs) && (0U < p_cfg->sub_len))) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Transmit buffer must hold at least a header without payload */
    if ((NULL == p_cfg->p_tx_buf) || ((uint32_t)SOMEIP_HEADER_SIZE > p_cfg->tx_buf_size)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check every event and reset its state */
    for (idx = 0U; idx < p_cfg->event_len; idx++) {
        /* Value of the event cannot be read, or the ID is in the method range (note of chapter 4.1.2.1) */
        if ((NULL == p_cfg->p_events[idx].serialize) ||
            (0U == (p_cfg->p_events[idx].event_id & (someip_event_id_t)SOMEIP_EVENT_ID_FLAG))) {
            /* Invalid parameter */
            return SOMEIP_RES_E_PARAM;
        }

        res = SOMEIP_SessionInit(&p_cfg->p_states[idx].session, p_cfg->p_events[idx].session_active);

        /* Session could not be initialized */
        if (SOMEIP_RES_OK != res) {
            /* Forward the session error */
            return res;
        }

        p_cfg->p_states[idx].last_cycle_ms = 0U;
    }

    /* No subscriber before the first SOMEIP_EventSubscribe() */
    for (idx = 0U; idx < p_cfg->sub_len; idx++) {
        p_cfg->p_subs[idx].used = 0U;
    }

    p_pub->cfg = *p_cfg;

    /* Publisher ready */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_EventSubscribe(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                      IN someip_event_id_t event_id, IN someip_endpoint_t *p_peer) {
    someip_subscriber_t *p_sub = NULL;
    someip_subscriber_t *p_free = NULL;
    uint16_t event_idx;
    uint16_t idx;

    /* Publisher and peer are mandatory */
    if ((NULL == p_pub) || (NULL == p_peer)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    event_idx = prv_find_event(p_pub, service_id, event_id);

    /* Event is not in the configuration */
    if (p_pub->cfg.event_len == event_idx) {
        /* Unknown event */
        return SOMEIP_RES_E_NOT_FOUND;
    }

    /* Look for the same subscription and for a free entry */
    for (idx = 0U; idx < p_pub->cfg.sub_len; idx++) {
        p_sub = &p_pub->cfg.p_subs[idx];

        /* Free entry: keep the first one */
        if (0U == p_sub->used) {
            /* First free entry */
            if (NULL == p_free) {
                p_free = p_sub;
            }
        }
        /* Peer already receives this event: do not add it twice [PRS_SOMEIP_00930] */
        else if ((event_idx == p_sub->event_idx) && prv_is_same_peer(&p_sub->peer, p_peer)) {
            break;
        }
        /* Other subscription */
        else {
            /* Nothing to do */
        }
    }

    /* New subscription */
    if (idx == p_pub->cfg.sub_len) {
        /* Subscriber table is full */
        if (NULL == p_free) {
            /* No entry left */
            return SOMEIP_RES_E_NO_SPACE;
        }

        p_free->peer = *p_peer;
        p_free->event_idx = event_idx;
        p_free->used = 1U;
    }

    /* Field: the subscriber gets the current value at once [PRS_SOMEIP_00909] */
    if (0U != p_pub->cfg.p_events[event_idx].is_field) {
        /* Return the result of the initial notification */
        return prv_send_event(p_pub, event_idx, p_peer);
    }

    /* Event: the first value comes with the next notification */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_EventUnsubscribe(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                        IN someip_event_id_t event_id, IN someip_endpoint_t *p_peer) {
    someip_subscriber_t *p_sub = NULL;
    uint16_t event_idx;
    uint16_t idx;

    /* Publisher and peer are mandatory */
    if ((NULL == p_pub) || (NULL == p_peer)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    event_idx = prv_find_event(p_pub, service_id, event_id);

    /* Event is not in the configuration */
    if (p_pub->cfg.event_len == event_idx) {
        /* Unknown event */
        return SOMEIP_RES_E_NOT_FOUND;
    }

    /* Search the subscription */
    for (idx = 0U; idx < p_pub->cfg.sub_len; idx++) {
        p_sub = &p_pub->cfg.p_subs[idx];

        /* Same event and same peer */
        if ((0U != p_sub->used) && (event_idx == p_sub->event_idx) &&
            prv_is_same_peer(&p_sub->peer, p_peer)) {
            p_sub->used = 0U;

            /* Subscription removed */
            return SOMEIP_RES_OK;
        }
    }

    /* Peer was not subscribed */
    return SOMEIP_RES_E_NOT_FOUND;
}

someip_result_t SOMEIP_EventNotify(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                   IN someip_event_id_t event_id) {
    uint16_t event_idx;

    /* Publisher is mandatory */
    if (NULL == p_pub) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    event_idx = prv_find_event(p_pub, service_id, event_id);

    /* Event is not in the configuration */
    if (p_pub->cfg.event_len == event_idx) {
        /* Unknown event */
        return SOMEIP_RES_E_NOT_FOUND;
    }

    /* Return the result of the send to all subscribers */
    return prv_send_event(p_pub, event_idx, NULL);
}

someip_result_t SOMEIP_EventMainFunction(INOUT someip_publisher_t *p_pub, IN uint32_t now_ms) {
    someip_event_state_t *p_state = NULL;
    someip_result_t res;
    someip_result_t final_res = SOMEIP_RES_OK;
    uint16_t idx;

    /* Publisher is mandatory */
    if (NULL == p_pub) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Check every event */
    for (idx = 0U; idx < p_pub->cfg.event_len; idx++) {
        p_state = &p_pub->cfg.p_states[idx];

        /* Cyclic update configured and its period is over, unsigned subtraction handles the wrap around */
        if ((0U != p_pub->cfg.p_events[idx].cycle_ms) &&
            ((now_ms - p_state->last_cycle_ms) >= p_pub->cfg.p_events[idx].cycle_ms)) {
            p_state->last_cycle_ms = now_ms;
            res = prv_send_event(p_pub, idx, NULL);

            /* Send failed: remember it, the next events are still sent */
            if (SOMEIP_RES_OK != res) {
                final_res = res;
            }
        }
    }

    /* All events checked */
    return final_res;
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
