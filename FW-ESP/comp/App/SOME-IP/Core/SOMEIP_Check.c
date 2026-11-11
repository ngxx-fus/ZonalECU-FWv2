

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "SOMEIP_Check.h"
#include "SOMEIP_Types.h"

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

static bool prv_is_type_supported(IN someip_message_type_t msg_type);
static bool prv_is_type_as_configured(IN someip_message_type_t msg_type, IN someip_message_type_t expected_type);
static someip_service_cfg_t const *prv_find_service(IN someip_service_cfg_t *p_services, IN uint16_t service_len,
                                                    IN someip_service_id_t service_id);
static someip_method_cfg_t const *prv_find_method(IN someip_service_cfg_t *p_service, IN someip_method_id_t method_id);

/* FUNCTION IMPLEMENTATIONS ******************************************************************************************************
 ********************************************************************************************************************************/

/* Private helpers */

/** @brief Check whether a Message Type is one of Table 4.4 (step 5 of Figure 4.21)
 *  @details Supported: REQUEST 0x00, REQUEST_NO_RETURN 0x01, NOTIFICATION 0x02, RESPONSE 0x80, ERROR 0x81
 *           and the same with the TP-Flag: 0x20, 0x21, 0x22, 0xA0, 0xA1 [PRS_SOMEIP_00055] [PRS_SOMEIP_00367]
 *  @param msg_type Received Message Type
 *  @retval true  Supported
 *  @retval false Not supported (e.g. 0x03, 0x40, 0xFF) */
static bool prv_is_type_supported(IN someip_message_type_t msg_type) {
    someip_message_type_t base = 0U;
    bool status = false;

    /* Clear TP-Flag */
    base = (someip_message_type_t)(msg_type & (someip_message_type_t)(~SOMEIP_MT_FLAG_TP));

    /* Base types of Table 4.4 */
    switch (base) {
        case SOMEIP_MT_REQUEST:
        case SOMEIP_MT_REQUEST_NO_RETURN:
        case SOMEIP_MT_NOTIFICATION:
        case SOMEIP_MT_RESPONSE:
        case SOMEIP_MT_ERROR:
            status = true;

            /* Supported */
            break;

        default:
            status = false;

            /* Not supported */
            break;
    }

    /* Result of the check */
    return status;
}

/** @brief Check whether a Message Type matches the configuration of the method (step 6 of Figure 4.21)
 *  @details The TP-Flag is ignored. Accepted combinations:
 *           @verbatim
 *           expected_type (config)       accepted received base type
 *           ---------------------------  ------------------------------------
 *           SOMEIP_MT_REQUEST            REQUEST, RESPONSE, ERROR
 *           SOMEIP_MT_REQUEST_NO_RETURN  REQUEST_NO_RETURN
 *           SOMEIP_MT_NOTIFICATION       NOTIFICATION
 *           @endverbatim
 *           RESPONSE / ERROR are accepted for a request/response method because the client receives them
 *           for the same Message ID.
 *  @param msg_type      Received Message Type
 *  @param expected_type Configured type of the method
 *  @retval true  As configured
 *  @retval false Not as configured */
static bool prv_is_type_as_configured(IN someip_message_type_t msg_type, IN someip_message_type_t expected_type) {
    someip_message_type_t base = 0U;
    bool status = false;

    /* Clear TP-Flag */
    base = (someip_message_type_t)(msg_type & (someip_message_type_t)(~SOMEIP_MT_FLAG_TP));

    /* Accepted types depend on the configured type */
    switch (expected_type) {
        case SOMEIP_MT_REQUEST:
            /* Request/response method: REQUEST, RESPONSE or ERROR */
            if (((someip_message_type_t)SOMEIP_MT_REQUEST == base) || ((someip_message_type_t)SOMEIP_MT_RESPONSE == base) ||
                ((someip_message_type_t)SOMEIP_MT_ERROR == base)) {
                status = true;
            }

            /* Request/response checked */
            break;

        case SOMEIP_MT_REQUEST_NO_RETURN:
            /* Fire & forget method: REQUEST_NO_RETURN only */
            if ((someip_message_type_t)SOMEIP_MT_REQUEST_NO_RETURN == base) {
                status = true;
            }

            /* Fire & forget checked */
            break;

        case SOMEIP_MT_NOTIFICATION:
            /* Event: NOTIFICATION only */
            if ((someip_message_type_t)SOMEIP_MT_NOTIFICATION == base) {
                status = true;
            }

            /* Event checked */
            break;

        default:
            /* Invalid configuration */
            break;
    }

    /* Result of the check */
    return status;
}

/** @brief Find a service in the configuration table
 *  @param p_services  Configuration table, may be NULL if service_len is 0
 *  @param service_len Number of entries in p_services
 *  @param service_id  Service ID to look for
 *  @return Pointer to the entry, or NULL if not found */
static someip_service_cfg_t const *prv_find_service(IN someip_service_cfg_t *p_services, IN uint16_t service_len,
                                                    IN someip_service_id_t service_id) {
    uint16_t i = 0U;

    for (i = 0U; i < service_len; i++) {
        /* Compare Service ID */
        if (service_id == p_services[i].service_id) {
            /* Found */
            return &p_services[i];
        }
    }

    /* Not found */
    return NULL;
}

/** @brief Find a method / event in the configuration of one service
 *  @param p_service Service entry
 *  @param method_id Method ID / Event ID to look for
 *  @return Pointer to the entry, or NULL if not found (or p_service / p_service->p_methods is NULL) */
static someip_method_cfg_t const *prv_find_method(IN someip_service_cfg_t *p_service, IN someip_method_id_t method_id) {
    uint16_t i = 0U;

    /* Check parameters */
    if ((NULL == p_service) || (NULL == p_service->p_methods)) {
        /* No method table */
        return NULL;
    }

    for (i = 0U; i < p_service->method_len; i++) {
        /* Compare Method ID */
        if (method_id == p_service->p_methods[i].method_id) {
            /* Found */
            return &p_service->p_methods[i];
        }
    }

    /* Not found */
    return NULL;
}

/* Public functions */

someip_result_t SOMEIP_CheckHeader(IN someip_header_t *p_rx, IN someip_service_cfg_t *p_services, IN uint16_t service_len,
                                   OUT someip_return_code_t *p_rc, OUT someip_method_cfg_t const **pp_method) {
    someip_service_cfg_t const *p_svc = NULL;
    someip_method_cfg_t const *p_m = NULL;

    /* Validate parameters */
    if ((NULL == p_rx) || (NULL == p_rc) || (NULL == pp_method) || ((NULL == p_services) && (0U < service_len))) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Initialize output parameter */
    *pp_method = NULL;

    /* Check Protocol Version */
    if ((someip_protocol_version_t)SOMEIP_PROTOCOL_VERSION != p_rx->protocol_version) {
        *p_rc = SOMEIP_E_WRONG_PROTOCOL_VERSION;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    /* Find Service */
    p_svc = prv_find_service(p_services, service_len, p_rx->service_id);

    /* Service unknown */
    if (NULL == p_svc) {
        *p_rc = SOMEIP_E_UNKNOWN_SERVICE;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    /* Check Interface Version */
    if (p_svc->interface_version != p_rx->interface_version) {
        *p_rc = SOMEIP_E_WRONG_INTERFACE_VERSION;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    /* Find Method */
    p_m = prv_find_method(p_svc, p_rx->method_id);

    /* Method unknown */
    if (NULL == p_m) {
        *p_rc = SOMEIP_E_UNKNOWN_METHOD;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    *pp_method = p_m;

    /* Check whether Message Type is supported */
    if (!prv_is_type_supported(p_rx->message_type)) {
        *p_rc = SOMEIP_E_WRONG_MESSAGE_TYPE;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    /* Check Message Type against configuration */
    if (!prv_is_type_as_configured(p_rx->message_type, p_m->expected_type)) {
        *p_rc = SOMEIP_E_WRONG_MESSAGE_TYPE;

        /* Check done, result in *p_rc */
        return SOMEIP_RES_OK;
    }

    /* All checks passed */
    *p_rc = SOMEIP_E_OK;

    /* Check done, result in *p_rc */
    return SOMEIP_RES_OK;
}

uint8_t SOMEIP_CheckShouldReplyError(IN someip_header_t *p_rx, IN someip_method_cfg_t *p_method) {
    someip_message_type_t base = 0U;

    /* Validate parameter */
    if (NULL == p_rx) {
        /* Invalid parameter: do not reply */
        return 0U;
    }

    /* Clear TP-Flag */
    base = (someip_message_type_t)(p_rx->message_type & (someip_message_type_t)(~SOMEIP_MT_FLAG_TP));

    /* Only REQUEST may receive an ERROR response */
    if ((someip_message_type_t)SOMEIP_MT_REQUEST != base) {
        /* Drop silently */
        return 0U;
    }

    /* Check configured method type */
    if ((NULL != p_method) && ((someip_message_type_t)SOMEIP_MT_REQUEST != p_method->expected_type)) {
        /* Drop silently */
        return 0U;
    }

    /* Send ERROR */
    return 1U;
}

someip_result_t SOMEIP_BuildErrorHeader(IN someip_header_t *p_rx, IN someip_return_code_t rc, OUT someip_header_t *p_err) {
    someip_service_id_t service_id = 0U;
    someip_method_id_t method_id = 0U;
    someip_client_id_t client_id = 0U;
    someip_session_id_t session_id = 0U;
    someip_interface_version_t interface_version = 0U;
    someip_protocol_version_t protocol_version = 0U;

    /* Validate parameters */
    if ((NULL == p_rx) || (NULL == p_err) || ((someip_return_code_t)SOMEIP_E_OK == rc)) {
        /* Invalid parameter */
        return SOMEIP_RES_E_PARAM;
    }

    /* Save fields before writing. This also supports p_rx == p_err. */
    service_id = p_rx->service_id;
    method_id = p_rx->method_id;
    client_id = p_rx->client_id;
    session_id = p_rx->session_id;
    interface_version = p_rx->interface_version;
    protocol_version = p_rx->protocol_version;

    /* Copy Message ID and Request ID */
    p_err->service_id = service_id;
    p_err->method_id = method_id;
    p_err->client_id = client_id;
    p_err->session_id = session_id;
    p_err->interface_version = interface_version;

    /* Set Protocol Version */
    if ((someip_return_code_t)SOMEIP_E_WRONG_PROTOCOL_VERSION == rc) {
        p_err->protocol_version = SOMEIP_PROTOCOL_VERSION;
    }

    /* Keep the Protocol Version of the request */
    else {
        p_err->protocol_version = protocol_version;
    }

    /* Set Message Type */
    p_err->message_type = SOMEIP_MT_ERROR;

    /* Set Return Code */
    p_err->return_code = rc;

    /* Set Length: no payload */
    p_err->length = SOMEIP_HEADER_PART2_SIZE;

    /* ERROR header ready */
    return SOMEIP_RES_OK;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/