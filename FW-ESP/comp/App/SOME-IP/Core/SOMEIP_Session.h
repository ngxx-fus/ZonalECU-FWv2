

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __SOMEIP_SESSION_H__
#define __SOMEIP_SESSION_H__

/** @brief   SOME/IP Session ID handling: public API (chapter 4.1.2.4, Request ID)
 *  @details - Generate Session IDs for outgoing requests / events [PRS_SOMEIP_00932] [00933] [00934] [00533]
 *             - Wrap-around 0xFFFF -> 0x0001 [PRS_SOMEIP_00521]
 *             - Match a received response with its request [PRS_SOMEIP_00739] */

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Types.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Session counter of one sender */
typedef struct st_someip_session {
    someip_session_id_t next_id; /*!< Value returned by the next SOMEIP_SessionNext() */
    uint8_t             active; /*!< 1 = session handling active, 0 = always 0x0000 */
} someip_session_t;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Initialize a session counter
 *  @details active = 1: the first SOMEIP_SessionNext() returns SOMEIP_SESSION_ID_MIN (0x0001).
 *           active = 0: SOMEIP_SessionNext() always returns SOMEIP_SESSION_ID_INACTIVE (0x0000).
 *  @param   p_session Session counter to initialize
 *  @param   active    1 = session handling active, 0 = not active, any non-zero value counts as 1
 *                     [PRS_SOMEIP_00932] [PRS_SOMEIP_00933]
 *  @retval  SOMEIP_RES_OK      Counter initialized
 *  @retval  SOMEIP_RES_E_PARAM p_session is NULL */
someip_result_t SOMEIP_SessionInit(OUT someip_session_t *p_session, IN uint8_t active);

/** @brief   Get the Session ID of the next outgoing message, then advance the counter
 *  @details Sequence when active: 0x0001, 0x0002, ..., 0xFFFE, 0xFFFF, 0x0001, ...
 *           0x0000 is never returned when active [PRS_SOMEIP_00933], the counter wraps to 0x0001
 *           [PRS_SOMEIP_00521]. When not active: always 0x0000 [PRS_SOMEIP_00932].
 *  @note    Request/Response must use session handling [PRS_SOMEIP_00533]: keep the returned ID with the
 *           pending request, SOMEIP_SessionIsMatch() needs it when the response arrives.
 *  @param   p_session Session counter
 *  @param   p_id      Session ID to put in the header of the message being sent
 *  @retval  SOMEIP_RES_OK      *p_id is valid
 *  @retval  SOMEIP_RES_E_PARAM p_session or p_id is NULL */
someip_result_t SOMEIP_SessionNext(INOUT someip_session_t *p_session, OUT someip_session_id_t *p_id);

/** @brief   Check whether a received response belongs to a sent request
 *  @details The response carries the same Request ID (Client ID + Session ID) as the request
 *           [PRS_SOMEIP_00704]; a client ignores a response whose Session ID does not match [00739].
 *  @note    Service ID / Method ID matching and Message Type checks are done by the RPC layer.
 *  @param   p_request  Header of the request that was sent
 *  @param   p_response Header of the received RESPONSE / ERROR
 *  @retval  true  Client ID and Session ID match
 *  @retval  false No match, or p_request / p_response is NULL */
bool SOMEIP_SessionIsMatch(IN someip_header_t *p_request, IN someip_header_t *p_response);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __SOMEIP_SESSION_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/