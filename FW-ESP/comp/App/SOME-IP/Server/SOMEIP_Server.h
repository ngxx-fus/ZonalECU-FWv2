

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_SERVER_H__
#define __SOMEIP_SERVER_H__

/** @brief SOME/IP server dispatch: public API (chapter 4.2.2, 4.2.3, 4.2.6)
 *  @details Takes one received datagram, checks every message in it, calls the handler of the method and
 *           sends the RESPONSE or ERROR. The transport is behind someip_server_send_t, so this module
 *           has no dependency on the Ethernet driver.
 *  @note    SOME/IP-TP is not supported yet: a segment gets E_WRONG_MESSAGE_TYPE (Figure 4.21). */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Check.h"
#include "SOMEIP_Serialization.h"
#include "SOMEIP_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/** @brief Function that implements one method of a service
 *  @note  Fire & forget: *p_response and the returned value are ignored [PRS_SOMEIP_00171].
 *  @param p_request  Received message, the payload is only valid during this call
 *  @param p_response Write cursor for the output arguments [PRS_SOMEIP_00923]
 *  @param p_ctx      someip_server_cfg_t.p_ctx
 *  @return SOMEIP_E_OK, SOMEIP_E_MALFORMED_MESSAGE if the payload cannot be deserialized (Figure 4.21), or
 *          another Return Code of Table 4.11 */
typedef someip_return_code_t (*someip_method_handler_t)(IN someip_message_t *p_request,
                                                        INOUT someip_ser_cursor_t *p_response,
                                                        INOUT void *p_ctx);

/** @brief Function that sends one complete SOME/IP message, provided by the transport layer
 *  @param p_peer  Sender of the request, the answer goes back to it
 *  @param p_msg   Header and payload, ready to send
 *  @param msg_len Number of bytes in p_msg
 *  @param p_ctx   someip_server_cfg_t.p_ctx
 *  @retval SOMEIP_RES_OK          Message accepted by the transport
 *  @retval SOMEIP_RES_E_TRANSPORT Message could not be sent */
typedef someip_result_t (*someip_server_send_t)(IN someip_endpoint_t *p_peer, IN uint8_t *p_msg,
                                                IN uint32_t msg_len, INOUT void *p_ctx);

/** @brief Links one configured method to its handler */
typedef struct st_someip_handler_cfg {
    someip_method_handler_t handler;
    someip_service_id_t service_id;
    someip_method_id_t method_id;
    uint8_t use_error_message; /*!< Return Code != E_OK: 1 = send ERROR (0x81), 0 = send RESPONSE (0x80) */
} someip_handler_cfg_t;

/** @brief Configuration of a server
 *  @note  p_services, p_handlers and p_tx_buf must stay valid while the server is used. */
typedef struct st_someip_server_cfg {
    someip_service_cfg_t const *p_services;
    someip_handler_cfg_t const *p_handlers;
    uint8_t *p_tx_buf; /*!< Holds the answer: header + payload */
    someip_server_send_t send;
    void *p_ctx;          /*!< Passed to send() and to every handler */
    uint32_t tx_buf_size; /*!< At least SOMEIP_HEADER_SIZE */
    uint16_t service_len; /*!< Number of entries in p_services */
    uint16_t handler_len; /*!< Number of entries in p_handlers */
} someip_server_cfg_t;

/** @brief Server instance */
typedef struct st_someip_server {
    someip_server_cfg_t cfg;
} someip_server_t;

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/** @brief Initialize a server
 *  @param p_server Server to initialize
 *  @param p_cfg    Configuration, copied into *p_server
 *  @retval SOMEIP_RES_OK      Server ready
 *  @retval SOMEIP_RES_E_PARAM A pointer is NULL, send is NULL, a table is NULL with a length > 0, or
 *                             tx_buf_size < SOMEIP_HEADER_SIZE */
someip_result_t SOMEIP_ServerInit(OUT someip_server_t *p_server, IN someip_server_cfg_t *p_cfg);

/** @brief Process every SOME/IP message of one received datagram [PRS_SOMEIP_00535] [PRS_SOMEIP_00140]
 *  @warning Not reentrant: all answers use the same p_tx_buf.
 *  @param p_server Server
 *  @param p_peer   Sender of the datagram
 *  @param p_buf    Received datagram
 *  @param buf_len  Number of valid bytes in p_buf
 *  @retval SOMEIP_RES_OK          All messages processed, dropping or answering with ERROR is also OK
 *  @retval SOMEIP_RES_E_PARAM     A pointer is NULL
 *  @retval SOMEIP_RES_E_MALFORMED A [PRS_SOMEIP_00910] check failed, the rest of the datagram is dropped
 *  @retval SOMEIP_RES_E_TRANSPORT An answer could not be sent, the other messages are still processed */
someip_result_t SOMEIP_ServerProcess(INOUT someip_server_t *p_server, IN someip_endpoint_t *p_peer,
                                     IN uint8_t *p_buf, IN uint32_t buf_len);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_SERVER_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
