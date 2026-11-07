

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Session.h"
#include "SOMEIP_Types.h"

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

someip_result_t SOMEIP_SessionInit(OUT someip_session_t *p_session, IN uint8_t active) {
    /* Check parameter */
    if (NULL == p_session) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Initialize active state */
    p_session->active = (0U != active) ? 1U : 0U;

    /* Initialize next Session ID */
    p_session->next_id = (0U != p_session->active) ? SOMEIP_SESSION_ID_MIN : SOMEIP_SESSION_ID_INACTIVE;

    /* Counter initialized */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_SessionNext(INOUT someip_session_t *p_session, OUT someip_session_id_t *p_id) {
    /* Validate parameters */
    if ((NULL == p_session) || (NULL == p_id)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Session handling is inactive */
    if (0U == p_session->active) {
        *p_id = SOMEIP_SESSION_ID_INACTIVE;

        /* Always 0x0000 */
        return SOMEIP_RES_OK;
    }

    /* Get current Session ID */
    *p_id = p_session->next_id;

    /* Advance Session ID counter */
    if ((someip_session_id_t)SOMEIP_SESSION_ID_MAX == p_session->next_id) {
        p_session->next_id = SOMEIP_SESSION_ID_MIN;
    }

    /* Not at the end: count up */
    else {
        p_session->next_id++;
    }

    /* *p_id is valid */
    return SOMEIP_RES_OK;
}

bool SOMEIP_SessionIsMatch(IN someip_header_t *p_request, IN someip_header_t *p_response) {
    /* Validate parameters */
    if ((NULL == p_request) || (NULL == p_response)) {
        /* Invalid parameter: no match */
        return false;
    }

    /* Compare Client ID and Session ID */
    if ((p_request->client_id == p_response->client_id) &&
        (p_request->session_id == p_response->session_id)) {
        /* Match */
        return true;
    }

    /* No match */
    return false;
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
