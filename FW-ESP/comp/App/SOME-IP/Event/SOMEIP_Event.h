

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __SOMEIP_EVENT_H__
#define __SOMEIP_EVENT_H__

/** @brief   SOME/IP events and field notifiers: public API (chapter 4.2.4, 4.2.5)
 *  @details The publisher keeps the subscribers of every configured event and sends NOTIFICATION (0x02)
 *           messages to them. Subscriptions come from a static configuration now and from SOME/IP-SD
 *           later: SOME/IP only transports the value, publish and subscribe belong to SD (chapter 4.2.4).
 *           The getter and setter of a field are normal methods of SOMEIP_Server [PRS_SOMEIP_00181]
 *           [PRS_SOMEIP_00182]; this module covers the notifier [PRS_SOMEIP_00909] [PRS_SOMEIP_00183].
 *  @note    Strategies (chapter 4.2.4.1): cyclic update is done by SOMEIP_EventMainFunction(), update on
 *           change and epsilon change are decided by the application, which then calls SOMEIP_EventNotify(). */

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Serialization.h"
#include "SOMEIP_Server.h"
#include "SOMEIP_Session.h"
#include "SOMEIP_Types.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Function that writes the current value of one event into the payload [PRS_SOMEIP_00926]
 *  @note    The same function can be used by the getter of a field, so the getter and the notifier
 *           always send the same value.
 *  @param   p_payload Write cursor placed right after the header
 *  @param   p_ctx     someip_publisher_cfg_t.p_ctx
 *  @retval  SOMEIP_RES_OK Payload written
 *  @retval  Other         Value not available, nothing is sent */
typedef someip_result_t (*someip_event_serialize_t)(INOUT someip_ser_cursor_t *p_payload, INOUT void *p_ctx);

/** @brief Configuration of one event or field notifier */
typedef struct st_someip_event_cfg {
    someip_event_serialize_t   serialize;
    uint32_t                   cycle_ms; /*!< 0 = no cyclic update, else period of the cyclic update (chapter 4.2.4.1) */
    someip_service_id_t        service_id;
    someip_event_id_t          event_id; /*!< Bit 15 = 1, see SOMEIP_EVENT_ID_FLAG */
    someip_interface_version_t interface_version; /*!< Major version of the service [PRS_SOMEIP_00925] */
    uint8_t                    is_field; /*!< 1 = notifier of a field: send the value on subscribe [PRS_SOMEIP_00909] */
    uint8_t                    session_active; /*!< 1 = Session ID + 1 per transmission [PRS_SOMEIP_00925], 0 = always 0 */
} someip_event_cfg_t;

/** @brief Run time state of one event, one entry per someip_event_cfg_t */
typedef struct st_someip_event_state {
    someip_session_t session;
    uint32_t         last_cycle_ms; /*!< Time of the last cyclic update */
} someip_event_state_t;

/** @brief One subscription: one peer receives one event */
typedef struct st_someip_subscriber {
    someip_endpoint_t peer;
    uint16_t          event_idx; /*!< Index in someip_publisher_cfg_t.p_events */
    uint8_t           used; /*!< 1 = entry holds a subscription, 0 = free */
} someip_subscriber_t;

/** @brief   Configuration of a publisher
 *  @note    All tables and p_tx_buf must stay valid while the publisher is used. p_tx_buf must not be the
 *           buffer of a server, so the server and the publisher can run in different tasks. */
typedef struct st_someip_publisher_cfg {
    someip_event_cfg_t const *p_events;
    someip_event_state_t *p_states; /*!< event_len entries */
    someip_subscriber_t *p_subs;    /*!< sub_len entries */
    uint8_t *p_tx_buf;              /*!< Holds one notification: header + payload */
    someip_server_send_t send;
    void *p_ctx;          /*!< Passed to send() and to every serialize() */
    uint32_t             tx_buf_size; /*!< At least SOMEIP_HEADER_SIZE */
    uint16_t             event_len; /*!< Number of entries in p_events and p_states */
    uint16_t             sub_len; /*!< Number of entries in p_subs */
} someip_publisher_cfg_t;

/** @brief Publisher instance */
typedef struct st_someip_publisher {
    someip_publisher_cfg_t cfg;
} someip_publisher_t;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Initialize a publisher: reset every Session ID and remove every subscription
 *  @param   p_pub Publisher to initialize
 *  @param   p_cfg Configuration, copied into *p_pub
 *  @retval  SOMEIP_RES_OK      Publisher ready
 *  @retval  SOMEIP_RES_E_PARAM A pointer is NULL, send is NULL, a table is NULL with a length > 0,
 *                              tx_buf_size < SOMEIP_HEADER_SIZE, an event has no serialize function,
 *                              or an Event ID has bit 15 = 0 */
someip_result_t SOMEIP_PublisherInit(OUT someip_publisher_t *p_pub, IN someip_publisher_cfg_t *p_cfg);

/** @brief   Add a subscriber to one event
 *  @details A peer that already receives the event is not added twice, so every notification reaches it
 *           once [PRS_SOMEIP_00930]. For a field, the current value is sent to this peer right away
 *           [PRS_SOMEIP_00909], also when the peer was already subscribed.
 *  @param   p_pub      Publisher
 *  @param   service_id Service of the event
 *  @param   event_id   Event to receive
 *  @param   p_peer     Endpoint the notifications are sent to
 *  @retval  SOMEIP_RES_OK          Peer subscribed, initial value sent for a field
 *  @retval  SOMEIP_RES_E_PARAM     A pointer is NULL
 *  @retval  SOMEIP_RES_E_NOT_FOUND Event is not configured
 *  @retval  SOMEIP_RES_E_NO_SPACE  Subscriber table is full, or the field value does not fit in p_tx_buf
 *  @retval  Other                  Peer subscribed, but the initial value could not be sent */
someip_result_t SOMEIP_EventSubscribe(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                      IN someip_event_id_t event_id, IN someip_endpoint_t *p_peer);

/** @brief   Remove a subscriber from one event
 *  @param   p_pub      Publisher
 *  @param   service_id Service of the event
 *  @param   event_id   Event to stop
 *  @param   p_peer     Endpoint given to SOMEIP_EventSubscribe()
 *  @retval  SOMEIP_RES_OK          Subscription removed
 *  @retval  SOMEIP_RES_E_PARAM     A pointer is NULL
 *  @retval  SOMEIP_RES_E_NOT_FOUND Event is not configured, or the peer is not subscribed */
someip_result_t SOMEIP_EventUnsubscribe(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                        IN someip_event_id_t event_id, IN someip_endpoint_t *p_peer);

/** @brief   Send the current value of one event to every subscriber [PRS_SOMEIP_00925] [PRS_SOMEIP_00183]
 *  @details serialize() is called once, and all subscribers get the same message with the same Session ID.
 *           Without subscribers nothing is sent and the Session ID does not change.
 *  @warning Not reentrant: all notifications use the same p_tx_buf.
 *  @param   p_pub      Publisher
 *  @param   service_id Service of the event
 *  @param   event_id   Event to send
 *  @retval  SOMEIP_RES_OK          Sent to every subscriber, or there is no subscriber
 *  @retval  SOMEIP_RES_E_PARAM     p_pub is NULL
 *  @retval  SOMEIP_RES_E_NOT_FOUND Event is not configured
 *  @retval  SOMEIP_RES_E_NO_SPACE  Value does not fit in p_tx_buf
 *  @retval  SOMEIP_RES_E_TRANSPORT A send failed, the other subscribers still get the message
 *  @retval  Other                  Error returned by serialize() */
someip_result_t SOMEIP_EventNotify(INOUT someip_publisher_t *p_pub, IN someip_service_id_t service_id,
                                   IN someip_event_id_t event_id);

/** @brief   Cyclic update of every event with cycle_ms > 0 (chapter 4.2.4.1)
 *  @details An event is sent when cycle_ms has passed since its last cyclic update. now_ms may wrap around.
 *  @param   p_pub  Publisher
 *  @param   now_ms Current time in milliseconds
 *  @retval  SOMEIP_RES_OK      Every due event was sent
 *  @retval  SOMEIP_RES_E_PARAM p_pub is NULL
 *  @retval  Other              Last error of SOMEIP_EventNotify(), the other events are still sent */
someip_result_t SOMEIP_EventMainFunction(INOUT someip_publisher_t *p_pub, IN uint32_t now_ms);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __SOMEIP_EVENT_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/