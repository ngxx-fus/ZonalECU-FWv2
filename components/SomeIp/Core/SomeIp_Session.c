/**
 ************************************************************************************************************
 * @file    SomeIp_Session.c
 * @brief   SOME/IP Session ID handling.
 *
 * @details This file provides the functions to:
 *          - Generate Session IDs for outgoing messages              [PRS_SOMEIP_00932] [PRS_SOMEIP_00933]
 *          - Wrap-around 0xFFFF -> 0x0001                             [PRS_SOMEIP_00521]
 *          - Match a received response with its request              [PRS_SOMEIP_00739]
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.2.4
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "SomeIp_Session.h"
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
 * @brief   Initialize a session counter.
 *
 * @details active = 1: the first SomeIp_Session_Next() returns SOMEIP_SESSION_ID_MIN (0x0001).
 *          active = 0: SomeIp_Session_Next() always returns SOMEIP_SESSION_ID_INACTIVE (0x0000).
 *
 * @param[out] s       Session counter to initialize.
 * @param[in]  active  1 = session handling active, 0 = not active. Any non-zero value counts as 1.
 *
 * @retval SOMEIP_RES_OK       Counter initialized.
 * @retval SOMEIP_RES_E_PARAM  @p s is NULL.
 *
 * @see     [PRS_SOMEIP_00932] [PRS_SOMEIP_00933]
 */
SomeIp_Result_t SomeIp_Session_Init(SomeIp_Session_t *s, uint8_t active)
{
    /* Check parameter */
    if (s == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Initialize active state */
    s->active = (active != 0u) ? 1u : 0u;

    /* Initialize next Session ID */
    s->nextId = (s->active != 0u) ? SOMEIP_SESSION_ID_MIN : SOMEIP_SESSION_ID_INACTIVE;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Get the Session ID for the next outgoing message, then advance the counter.
 *
 * @details Sequence when active:  0x0001, 0x0002, ..., 0xFFFE, 0xFFFF, 0x0001, ...
 *          0x0000 is never returned when active [PRS_SOMEIP_00933], the counter wraps to 0x0001
 *          [PRS_SOMEIP_00521].
 *          When not active: always 0x0000 [PRS_SOMEIP_00932].
 *
 * @param[in,out] s   Session counter.
 * @param[out]    id  Session ID to put in the header of the message being sent.
 *
 * @retval SOMEIP_RES_OK       @p id is valid.
 * @retval SOMEIP_RES_E_PARAM  @p s or @p id is NULL.
 *
 * @note    Request/Response must use session handling [PRS_SOMEIP_00533]: keep the returned id with
 *          the pending request, it is needed by SomeIp_Session_IsMatch() when the response arrives.
 *
 * @see     [PRS_SOMEIP_00521] [PRS_SOMEIP_00533] [PRS_SOMEIP_00932] [PRS_SOMEIP_00933] [PRS_SOMEIP_00934]
 */
SomeIp_Result_t SomeIp_Session_Next(SomeIp_Session_t *s, SomeIp_SessionId_t *id)
{
    /* Validate parameters */
    if (s == NULL || id == NULL)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Session handling is inactive */
    if (s->active == 0u)
    {
        *id = SOMEIP_SESSION_ID_INACTIVE;
        return SOMEIP_RES_OK;
    }

    /* Get current Session ID */
    *id = s->nextId;

    /* Advance Session ID counter */
    if (s->nextId == SOMEIP_SESSION_ID_MAX)
    {
        s->nextId = SOMEIP_SESSION_ID_MIN;
    }
    else
    {
        s->nextId++;
    }

    return SOMEIP_RES_OK;
}

/**
 * @brief   Check whether a received response belongs to a sent request.
 *
 * @details The response must carry the same Request ID (Client ID + Session ID) as the request
 *          [PRS_SOMEIP_00704]. A client shall ignore a response whose Session ID does not match
 *          [PRS_SOMEIP_00739].
 *
 * @param[in]  request   Header of the request that was sent.
 * @param[in]  response  Header of the received RESPONSE / ERROR.
 *
 * @retval 1  Client ID and Session ID match.
 * @retval 0  No match, or @p request / @p response is NULL.
 *
 * @note    Service ID / Method ID matching and Message Type checks are done by the Rpc layer.
 *
 * @see     [PRS_SOMEIP_00704] [PRS_SOMEIP_00739]
 */
uint8_t SomeIp_Session_IsMatch(const SomeIp_Header_t *request, const SomeIp_Header_t *response)
{
    /* Validate parameters */
    if (request == NULL || response == NULL)
    {
        return 0u;
    }

    /* Compare Client ID and Session ID */
    if ((request->clientId == response->clientId) && (request->sessionId == response->sessionId))
    {
        return 1u;
    }
    
    return 0u;
}

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
