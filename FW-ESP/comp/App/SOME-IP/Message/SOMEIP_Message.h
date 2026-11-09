

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_MESSAGE_H__
#define __SOMEIP_MESSAGE_H__

/** @brief SOME/IP message headers: public API (chapter 4.2.2 .. 4.2.4, 4.2.6)
 *  @details Each function only fills a someip_header_t, nothing is sent.
 *  @note    The TP-Flag is never set here: SOME/IP-TP sets it on the segments [PRS_SOMEIP_00722]. */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/** @brief Fill the header of a REQUEST (0x00), the method answers with RESPONSE or ERROR [PRS_SOMEIP_00920]
 *  @param p_hdr             Header to fill
 *  @param service_id        Service of the method
 *  @param method_id         Method to call
 *  @param client_id         Client ID of the caller [PRS_SOMEIP_00702]
 *  @param session_id        From SOMEIP_SessionNext(), must not be 0 [PRS_SOMEIP_00533] [PRS_SOMEIP_00933]
 *  @param interface_version Major version of the service interface [PRS_SOMEIP_00053]
 *  @param payload_len       Size of the serialized arguments in bytes
 *  @retval SOMEIP_RES_OK      *p_hdr is valid
 *  @retval SOMEIP_RES_E_PARAM p_hdr is NULL, session_id is 0, or payload_len does not fit in Length */
someip_result_t SOMEIP_MessageBuildRequest(OUT someip_header_t *p_hdr, IN someip_service_id_t service_id,
                                           IN someip_method_id_t method_id, IN someip_client_id_t client_id,
                                           IN someip_session_id_t session_id,
                                           IN someip_interface_version_t interface_version,
                                           IN uint32_t payload_len);

/** @brief Fill the header of a REQUEST_NO_RETURN (0x01), fire & forget [PRS_SOMEIP_00924]
 *  @param p_hdr             Header to fill
 *  @param service_id        Service of the method
 *  @param method_id         Method to call
 *  @param client_id         Client ID of the caller [PRS_SOMEIP_00702]
 *  @param session_id        0 if session handling is not active [PRS_SOMEIP_00932]
 *  @param interface_version Major version of the service interface [PRS_SOMEIP_00053]
 *  @param payload_len       Size of the serialized arguments in bytes
 *  @retval SOMEIP_RES_OK      *p_hdr is valid
 *  @retval SOMEIP_RES_E_PARAM p_hdr is NULL, or payload_len does not fit in Length */
someip_result_t SOMEIP_MessageBuildRequestNoReturn(
    OUT someip_header_t *p_hdr, IN someip_service_id_t service_id, IN someip_method_id_t method_id,
    IN someip_client_id_t client_id, IN someip_session_id_t session_id,
    IN someip_interface_version_t interface_version, IN uint32_t payload_len);

/** @brief Fill the header of a NOTIFICATION (0x02), Client ID is always 0 [PRS_SOMEIP_00925]
 *  @param p_hdr             Header to fill
 *  @param service_id        Service of the event
 *  @param event_id          Event to send
 *  @param session_id        0 if session handling is not active, else incremented per transmission
 *                           [PRS_SOMEIP_00932] [PRS_SOMEIP_00933] [PRS_SOMEIP_00521]
 *  @param interface_version Major version of the service interface [PRS_SOMEIP_00053]
 *  @param payload_len       Size of the serialized event in bytes [PRS_SOMEIP_00926]
 *  @retval SOMEIP_RES_OK      *p_hdr is valid
 *  @retval SOMEIP_RES_E_PARAM p_hdr is NULL, or payload_len does not fit in Length */
someip_result_t
SOMEIP_MessageBuildNotification(OUT someip_header_t *p_hdr, IN someip_service_id_t service_id,
                                IN someip_event_id_t event_id, IN someip_session_id_t session_id,
                                IN someip_interface_version_t interface_version, IN uint32_t payload_len);

/** @brief Fill the header of a RESPONSE (0x80) that answers a received REQUEST [PRS_SOMEIP_00922]
 *  @note  An application error keeps Message Type 0x80 (note of [PRS_SOMEIP_00901]).
 *         p_request and p_hdr may point to the same header.
 *  @param p_hdr       Header to fill
 *  @param p_request   Header of the received REQUEST (0x00, or 0x20 with TP-Flag)
 *  @param return_code Return Code of Table 4.11, SOMEIP_E_OK if no error occurred
 *  @param payload_len Size of the serialized output arguments in bytes [PRS_SOMEIP_00923]
 *  @retval SOMEIP_RES_OK      *p_hdr is valid
 *  @retval SOMEIP_RES_E_PARAM A pointer is NULL, p_request is not a REQUEST, return_code cannot be sent, or
 *                             payload_len does not fit in Length */
someip_result_t SOMEIP_MessageBuildResponse(OUT someip_header_t *p_hdr, IN someip_header_t *p_request,
                                            IN someip_return_code_t return_code, IN uint32_t payload_len);

/** @brief Fill the header of an ERROR (0x81) with payload, answers a received REQUEST [PRS_SOMEIP_00903]
 *  @note  Use SOMEIP_CheckShouldReplyError() first: not every message may be answered with an ERROR.
 *         payload_len is the error specific content, the request payload is never copied
 *         [PRS_SOMEIP_00190]. p_request and p_hdr may point to the same header.
 *  @param p_hdr       Header to fill
 *  @param p_request   Header of the received REQUEST (0x00, or 0x20 with TP-Flag)
 *  @param return_code Return Code of Table 4.11, must not be SOMEIP_E_OK (Table 4.5)
 *  @param payload_len Size of the error specific payload in bytes, 0 if there is none
 *  @retval SOMEIP_RES_OK      *p_hdr is valid
 *  @retval SOMEIP_RES_E_PARAM A pointer is NULL, p_request is not a REQUEST, return_code cannot be sent, or
 *                             payload_len does not fit in Length */
someip_result_t SOMEIP_MessageBuildError(OUT someip_header_t *p_hdr, IN someip_header_t *p_request,
                                         IN someip_return_code_t return_code, IN uint32_t payload_len);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_MESSAGE_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
