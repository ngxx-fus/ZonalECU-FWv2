

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Message.h"
#include "SOMEIP_Check.h"
#include "SOMEIP_Types.h"

/* FUNCTION IMPLEMENTATIONS ******************************************************************************************************
 ********************************************************************************************************************************/

someip_result_t SOMEIP_MessageBuildRequest(OUT someip_header_t *p_hdr, IN someip_service_id_t service_id,
                                           IN someip_method_id_t method_id, IN someip_client_id_t client_id,
                                           IN someip_session_id_t session_id, IN someip_interface_version_t interface_version,
                                           IN uint32_t payload_len) {
    /* Validate parameters */
    if (NULL == p_hdr) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Request/response always uses session handling [PRS_SOMEIP_00533] [PRS_SOMEIP_00933] */
    if ((someip_session_id_t)SOMEIP_SESSION_ID_INACTIVE == session_id) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Length is 32 bit: SOMEIP_HEADER_PART2_SIZE + payload_len must not overflow */
    if (payload_len > (UINT32_MAX - (uint32_t)SOMEIP_HEADER_PART2_SIZE)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Message ID: the method to call [PRS_SOMEIP_00920] */
    p_hdr->service_id = service_id;
    p_hdr->method_id = method_id;

    /* Length: from Request ID to the end of the message [PRS_SOMEIP_00042] */
    p_hdr->length = (uint32_t)SOMEIP_HEADER_PART2_SIZE + payload_len;

    /* Request ID */
    p_hdr->client_id = client_id;
    p_hdr->session_id = session_id;

    /* Versions [PRS_SOMEIP_00051] [PRS_SOMEIP_00053] */
    p_hdr->protocol_version = SOMEIP_PROTOCOL_VERSION;
    p_hdr->interface_version = interface_version;

    /* Message Type and Return Code [PRS_SOMEIP_00920] [PRS_SOMEIP_00905] */
    p_hdr->message_type = SOMEIP_MT_REQUEST;
    p_hdr->return_code = SOMEIP_E_OK;

    /* *p_hdr is valid */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_MessageBuildRequestNoReturn(OUT someip_header_t *p_hdr, IN someip_service_id_t service_id,
                                                   IN someip_method_id_t method_id, IN someip_client_id_t client_id,
                                                   IN someip_session_id_t session_id,
                                                   IN someip_interface_version_t interface_version, IN uint32_t payload_len) {
    /* Validate parameters */
    if (NULL == p_hdr) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Length is 32 bit: SOMEIP_HEADER_PART2_SIZE + payload_len must not overflow */
    if (payload_len > (UINT32_MAX - (uint32_t)SOMEIP_HEADER_PART2_SIZE)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Message ID: the method to call [PRS_SOMEIP_00924] */
    p_hdr->service_id = service_id;
    p_hdr->method_id = method_id;

    /* Length: from Request ID to the end of the message [PRS_SOMEIP_00042] */
    p_hdr->length = (uint32_t)SOMEIP_HEADER_PART2_SIZE + payload_len;

    /* Request ID */
    p_hdr->client_id = client_id;
    p_hdr->session_id = session_id;

    /* Versions [PRS_SOMEIP_00051] [PRS_SOMEIP_00053] */
    p_hdr->protocol_version = SOMEIP_PROTOCOL_VERSION;
    p_hdr->interface_version = interface_version;

    /* Message Type and Return Code [PRS_SOMEIP_00924] [PRS_SOMEIP_00905] */
    p_hdr->message_type = SOMEIP_MT_REQUEST_NO_RETURN;
    p_hdr->return_code = SOMEIP_E_OK;

    /* *p_hdr is valid */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_MessageBuildNotification(OUT someip_header_t *p_hdr, IN someip_service_id_t service_id,
                                                IN someip_event_id_t event_id, IN someip_session_id_t session_id,
                                                IN someip_interface_version_t interface_version, IN uint32_t payload_len) {
    /* Validate parameters */
    if (NULL == p_hdr) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Length is 32 bit: SOMEIP_HEADER_PART2_SIZE + payload_len must not overflow */
    if (payload_len > (UINT32_MAX - (uint32_t)SOMEIP_HEADER_PART2_SIZE)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Message ID: the event to send [PRS_SOMEIP_00925] */
    p_hdr->service_id = service_id;
    p_hdr->method_id = event_id;

    /* Length: from Request ID to the end of the message [PRS_SOMEIP_00042] */
    p_hdr->length = (uint32_t)SOMEIP_HEADER_PART2_SIZE + payload_len;

    /* Request ID: Client ID is always 0x00 [PRS_SOMEIP_00925],
     * Session ID is 0 if session handling is not active [PRS_SOMEIP_00932] [PRS_SOMEIP_00933] */
    p_hdr->client_id = 0U;
    p_hdr->session_id = session_id;

    /* Versions [PRS_SOMEIP_00051] [PRS_SOMEIP_00053] */
    p_hdr->protocol_version = SOMEIP_PROTOCOL_VERSION;
    p_hdr->interface_version = interface_version;

    /* Message Type and Return Code [PRS_SOMEIP_00925] [PRS_SOMEIP_00905] */
    p_hdr->message_type = SOMEIP_MT_NOTIFICATION;
    p_hdr->return_code = SOMEIP_E_OK;

    /* *p_hdr is valid */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_MessageBuildResponse(OUT someip_header_t *p_hdr, IN someip_header_t *p_request,
                                            IN someip_return_code_t return_code, IN uint32_t payload_len) {
    someip_message_type_t base = 0U;

    /* Validate parameters */
    if ((NULL == p_hdr) || (NULL == p_request)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Length is 32 bit: SOMEIP_HEADER_PART2_SIZE + payload_len must not overflow */
    if (payload_len > (UINT32_MAX - (uint32_t)SOMEIP_HEADER_PART2_SIZE)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Clear TP-Flag */
    base = (someip_message_type_t)(p_request->message_type & (someip_message_type_t)(~SOMEIP_MT_FLAG_TP));

    /* Only a REQUEST is answered: fire & forget and notifications have no response */
    if ((someip_message_type_t)SOMEIP_MT_REQUEST != base) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Internal only return codes are never sent, values above 0x5E are not defined (Table 4.11) */
    if (((someip_return_code_t)SOMEIP_E_NOT_REACHABLE == return_code) ||
        ((someip_return_code_t)SOMEIP_E_TIMEOUT == return_code) ||
        (return_code > (someip_return_code_t)SOMEIP_E_SERVICE_SPECIFIC_MAX)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Message ID: taken over from the request [PRS_SOMEIP_00922] */
    p_hdr->service_id = p_request->service_id;
    p_hdr->method_id = p_request->method_id;

    /* Length: 8 bytes + new payload size [PRS_SOMEIP_00922] */
    p_hdr->length = (uint32_t)SOMEIP_HEADER_PART2_SIZE + payload_len;

    /* Request ID: taken over from the request [PRS_SOMEIP_00704] */
    p_hdr->client_id = p_request->client_id;
    p_hdr->session_id = p_request->session_id;

    /* Versions: based on the header of the request [PRS_SOMEIP_00922] */
    p_hdr->protocol_version = p_request->protocol_version;
    p_hdr->interface_version = p_request->interface_version;

    /* Message Type and Return Code [PRS_SOMEIP_00922]
     * The TP-Flag is set later on the segments [PRS_SOMEIP_00722] */
    p_hdr->message_type = SOMEIP_MT_RESPONSE;
    p_hdr->return_code = return_code;

    /* *p_hdr is valid */
    return SOMEIP_RES_OK;
}

someip_result_t SOMEIP_MessageBuildError(OUT someip_header_t *p_hdr, IN someip_header_t *p_request,
                                         IN someip_return_code_t return_code, IN uint32_t payload_len) {
    someip_message_type_t base = 0U;
    someip_result_t result = SOMEIP_RES_OK;

    /* Validate parameters */
    if ((NULL == p_hdr) || (NULL == p_request)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Length is 32 bit: SOMEIP_HEADER_PART2_SIZE + payload_len must not overflow */
    if (payload_len > (UINT32_MAX - (uint32_t)SOMEIP_HEADER_PART2_SIZE)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Clear TP-Flag */
    base = (someip_message_type_t)(p_request->message_type & (someip_message_type_t)(~SOMEIP_MT_FLAG_TP));

    /* Only a REQUEST is answered with an ERROR [PRS_SOMEIP_00188] [PRS_SOMEIP_00189] */
    if ((someip_message_type_t)SOMEIP_MT_REQUEST != base) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Internal only return codes are never sent, values above 0x5E are not defined (Table 4.11) */
    if (((someip_return_code_t)SOMEIP_E_NOT_REACHABLE == return_code) ||
        ((someip_return_code_t)SOMEIP_E_TIMEOUT == return_code) ||
        (return_code > (someip_return_code_t)SOMEIP_E_SERVICE_SPECIFIC_MAX)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Copy the header fields, set Message Type and Return Code, E_OK is rejected here [PRS_SOMEIP_00190] */
    result = SOMEIP_BuildErrorHeader(p_request, return_code, p_hdr);
    if (SOMEIP_RES_OK != result) {
        /* Forward the error */
        return result;
    }

    /* Add the error specific payload, cannot overflow: checked above [PRS_SOMEIP_00903] */
    p_hdr->length += payload_len;

    /* *p_hdr is valid */
    return SOMEIP_RES_OK;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/
