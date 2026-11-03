/**
 ************************************************************************************************************
 * @file    SomeIp_Check.c
 * @brief   SOME/IP message validation and error message.
 *
 * @details This file provides the functions to:
 *          - Check a received header in the order of Figure 4.21     [PRS_SOMEIP_00195] [PRS_SOMEIP_00576]
 *          - Decide whether an ERROR message may be sent back         [PRS_SOMEIP_00188] [PRS_SOMEIP_00189]
 *                                                                    [PRS_SOMEIP_00537] [PRS_SOMEIP_00539]
 *          - Build the header of the ERROR message                    [PRS_SOMEIP_00190]
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.2.6, Figure 4.21
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include "SomeIp_Check.h"
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
static uint8_t SomeIp_Check_IsTypeSupported(SomeIp_MessageType_t mt);
static uint8_t SomeIp_Check_IsTypeAsConfigured(SomeIp_MessageType_t mt, SomeIp_MessageType_t expectedType);
static const SomeIp_ServiceConfig_t *SomeIp_Check_FindService(const SomeIp_ServiceConfig_t *services,
                                                              uint16_t serviceCount, SomeIp_ServiceId_t serviceId);
static const SomeIp_MethodConfig_t *SomeIp_Check_FindMethod(const SomeIp_ServiceConfig_t *service,
                                                            SomeIp_MethodId_t methodId);

/* PUBLIC VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS *********************************************************************************
 ************************************************************************************************************/

/**
 * @brief   Check whether a Message Type is one of the values of Table 4.4 (step 5 of Figure 4.21).
 *
 * @details Supported values:
 *          REQUEST 0x00, REQUEST_NO_RETURN 0x01, NOTIFICATION 0x02, RESPONSE 0x80, ERROR 0x81
 *          and the same with the TP-Flag: 0x20, 0x21, 0x22, 0xA0, 0xA1           [PRS_SOMEIP_00367]
 *
 * @param[in]  mt  Received Message Type.
 *
 * @retval 1  Supported.
 * @retval 0  Not supported (e.g. 0x03, 0x40, 0xFF).
 *
 * @see     [PRS_SOMEIP_00055] Table 4.4
 */
static uint8_t SomeIp_Check_IsTypeSupported(SomeIp_MessageType_t mt)
{
    SomeIp_MessageType_t base;
    uint8_t status;

    /* Clear TP-Flag */
    base = (SomeIp_MessageType_t)(mt & (SomeIp_MessageType_t)(~SOMEIP_MT_FLAG_TP));

    switch (base)
    {
        case SOMEIP_MT_REQUEST:
        case SOMEIP_MT_REQUEST_NO_RETURN:
        case SOMEIP_MT_NOTIFICATION:
        case SOMEIP_MT_RESPONSE:
        case SOMEIP_MT_ERROR:
            status = 1u;
            break;

        default:
            status = 0u;
            break;
    }

    return status;
}

/**
 * @brief   Check whether a Message Type matches the configuration of the method (step 6 of Figure 4.21).
 *
 * @details The TP-Flag is ignored. Accepted combinations:
 *          @verbatim
 *          expectedType (config)        accepted received base type
 *          ---------------------------  ------------------------------------
 *          SOMEIP_MT_REQUEST            REQUEST, RESPONSE, ERROR
 *          SOMEIP_MT_REQUEST_NO_RETURN  REQUEST_NO_RETURN
 *          SOMEIP_MT_NOTIFICATION       NOTIFICATION
 *          @endverbatim
 *          RESPONSE / ERROR are accepted for a request/response method because the client receives them
 *          for the same Message ID.
 *
 * @param[in]  mt            Received Message Type.
 * @param[in]  expectedType  Configured type of the method.
 *
 * @retval 1  As configured.
 * @retval 0  Not as configured.
 *
 * @see     Figure 4.21 ("Message Type as Configured?")
 */
static uint8_t SomeIp_Check_IsTypeAsConfigured(SomeIp_MessageType_t mt, SomeIp_MessageType_t expectedType)
{
    SomeIp_MessageType_t base;
    uint8_t status = 0u;

    /* Clear TP-Flag */
    base = (SomeIp_MessageType_t)(mt & (SomeIp_MessageType_t)(~SOMEIP_MT_FLAG_TP));

    switch (expectedType)
    {
        case SOMEIP_MT_REQUEST:
            if ((base == SOMEIP_MT_REQUEST) || (base == SOMEIP_MT_RESPONSE) || (base == SOMEIP_MT_ERROR))
            {
                status = 1u;
            }
            break;
        
        case SOMEIP_MT_REQUEST_NO_RETURN:
            if (base == SOMEIP_MT_REQUEST_NO_RETURN)
            {
                status = 1u;
            }
            break;
        
        case SOMEIP_MT_NOTIFICATION:
            if (base == SOMEIP_MT_NOTIFICATION)
            {
                status = 1u;
            }
            break;

        default:
            /* Invalid configuration */
            break;
    }

    return status;
}

/**
 * @brief   Find a service in the configuration table.
 *
 * @param[in]  services      Configuration table (may be NULL if @p serviceCount is 0).
 * @param[in]  serviceCount  Number of entries in @p services.
 * @param[in]  serviceId     Service ID to look for.
 *
 * @return  Pointer to the entry, or NULL if not found.
 */
static const SomeIp_ServiceConfig_t *SomeIp_Check_FindService(const SomeIp_ServiceConfig_t *services,
                                                              uint16_t serviceCount, SomeIp_ServiceId_t serviceId)
{
    uint16_t i;

    for (i = 0u; i < serviceCount; i++)
    {
        if (services[i].serviceId == serviceId)
        {
            return &services[i];
        }
    }

    return NULL;
}

/**
 * @brief   Find a method / event in the configuration of one service.
 *
 * @param[in]  service   Service entry (never NULL here, checked by the caller).
 * @param[in]  methodId  Method ID / Event ID to look for.
 *
 * @return  Pointer to the entry, or NULL if not found (or service->methods is NULL).
 */
static const SomeIp_MethodConfig_t *SomeIp_Check_FindMethod(const SomeIp_ServiceConfig_t *service,
                                                            SomeIp_MethodId_t methodId)
{
    uint16_t i;

    /* Check parameters */
    if ((service == NULL) || (service->methods == NULL))
    {
        return NULL;
    }

    for (i = 0u; i < service->methodCount; i++)
    {
        if (service->methods[i].methodId == methodId)
        {
            return &service->methods[i];
        }
    }

    return NULL;
}

/**
 * @brief   Check a received header against the configuration (steps 1..6 of Figure 4.21).
 *
 * @details The checks are done in this exact order, the first failing check gives the Return Code:
 *          @verbatim
 *          #  check                                  failed -> *rc
 *          -  -------------------------------------  --------------------------------
 *          1  Protocol Version == 0x01               SOMEIP_E_WRONG_PROTOCOL_VERSION
 *          2  Service ID known                       SOMEIP_E_UNKNOWN_SERVICE
 *          3  Interface Version == configured        SOMEIP_E_WRONG_INTERFACE_VERSION
 *          4  Method ID known                        SOMEIP_E_UNKNOWN_METHOD
 *          5  Message Type supported (Table 4.4)     SOMEIP_E_WRONG_MESSAGE_TYPE
 *          6  Message Type as configured             SOMEIP_E_WRONG_MESSAGE_TYPE
 *          @endverbatim
 *          All checks pass -> *rc = SOMEIP_E_OK.
 *
 * @param[in]  rx            Received header.
 * @param[in]  services      Configuration table (may be NULL if @p serviceCount is 0).
 * @param[in]  serviceCount  Number of entries in @p services.
 * @param[out] rc            SOMEIP_E_OK or the Return Code of the first failing check.
 * @param[out] method        Configured method if step 4 passed, otherwise NULL.
 *                           Needed by SomeIp_Check_ShouldReplyError() [PRS_SOMEIP_00537].
 *
 * @retval SOMEIP_RES_OK       *rc and *method are valid.
 * @retval SOMEIP_RES_E_PARAM  @p rx, @p rc or @p method is NULL, or @p services is NULL with
 *                             @p serviceCount > 0.
 *
 * @note    SOMEIP_RES_OK does NOT mean the message is valid: always look at *rc.
 *
 * @see     [PRS_SOMEIP_00195] [PRS_SOMEIP_00576] Figure 4.21
 */
SomeIp_Result_t SomeIp_Check_Header(const SomeIp_Header_t       *rx,
                                    const SomeIp_ServiceConfig_t *services,
                                    uint16_t                      serviceCount,
                                    SomeIp_ReturnCode_t          *rc,
                                    const SomeIp_MethodConfig_t **method)
{
    const SomeIp_ServiceConfig_t *svc;
    const SomeIp_MethodConfig_t *m;

    /* Validate parameters */
    if ((rx == NULL) || (rc == NULL) || (method == NULL) || ((services == NULL) && (serviceCount > 0u)))
    {
        return SOMEIP_RES_E_PARAM;
    }
    
    /* Initialize output parameter */
    *method = NULL;

    /* Check Protocol Version */
    if (rx->protocolVersion != SOMEIP_PROTOCOL_VERSION)
    {
        *rc = SOMEIP_E_WRONG_PROTOCOL_VERSION;
        return SOMEIP_RES_OK;
    }

    /* Find Service */
    svc = SomeIp_Check_FindService(services, serviceCount, rx->serviceId);
    if (svc == NULL)
    {
        *rc = SOMEIP_E_UNKNOWN_SERVICE;
        return SOMEIP_RES_OK;
    }

    /* Check Interface Version */
    if (rx->interfaceVersion != svc->interfaceVersion)
    {
        *rc = SOMEIP_E_WRONG_INTERFACE_VERSION;
        return SOMEIP_RES_OK;
    }

    /* Find Method */
    m = SomeIp_Check_FindMethod(svc, rx->methodId);
    if (m == NULL)
    {
        *rc = SOMEIP_E_UNKNOWN_METHOD;
        return SOMEIP_RES_OK;
    }

    *method = m;

    /* Check whether Message Type is supported */
    if (SomeIp_Check_IsTypeSupported(rx->messageType) == 0u)
    {
        *rc = SOMEIP_E_WRONG_MESSAGE_TYPE;
        return SOMEIP_RES_OK;
    }

    /* Check Message Type against configuration */
    if (SomeIp_Check_IsTypeAsConfigured(rx->messageType, m->expectedType) == 0u)
    {
        *rc = SOMEIP_E_WRONG_MESSAGE_TYPE;
        return SOMEIP_RES_OK;
    }

    /* All checks passed */
    *rc = SOMEIP_E_OK;

    return SOMEIP_RES_OK;
}

/**
 * @brief   Decide whether an ERROR message may be sent back for a message that failed a check.
 *
 * @details Only a REQUEST (0x00, or 0x20 with TP-Flag) is answered with an ERROR message:
 *          @verbatim
 *          received                                       reply ERROR?   spec
 *          ---------------------------------------------  -------------  ------------------
 *          NOTIFICATION                                   no             [PRS_SOMEIP_00188]
 *          REQUEST_NO_RETURN                              no             [PRS_SOMEIP_00189]
 *          RESPONSE / ERROR                               no             [PRS_SOMEIP_00539]
 *          REQUEST, but method configured as event or     no             [PRS_SOMEIP_00537]
 *            fire&forget (method->expectedType != REQUEST)
 *          REQUEST, method unknown (method == NULL)       yes            Figure 4.21
 *          REQUEST, method configured as REQUEST          yes            Figure 4.21
 *          @endverbatim
 *
 * @param[in]  rx      Received header.
 * @param[in]  method  Configured method from SomeIp_Check_Header() (NULL if not found).
 *
 * @retval 1  Send an ERROR message.
 * @retval 0  Drop silently, or @p rx is NULL.
 *
 * @see     [PRS_SOMEIP_00188] [PRS_SOMEIP_00189] [PRS_SOMEIP_00537] [PRS_SOMEIP_00539] Figure 4.21
 */
uint8_t SomeIp_Check_ShouldReplyError(const SomeIp_Header_t *rx, const SomeIp_MethodConfig_t *method)
{
    SomeIp_MessageType_t base;
    /* Validate parameter */
    if (rx == NULL)
    {
        return 0u; 
    }

    /* Clear TP-Flag */
    base = (SomeIp_MessageType_t)(rx->messageType & (SomeIp_MessageType_t)(~SOMEIP_MT_FLAG_TP));

    /* Only REQUEST may receive an ERROR response */
    if (base != SOMEIP_MT_REQUEST)
    {
        return 0u;
    }

    /* Check configured method type */
    if ((method != NULL) && (method->expectedType != SOMEIP_MT_REQUEST))
    {
        return 0u;
    }

    /* Send ERROR */
    return 1u;
}

/**
 * @brief   Build the header of the ERROR message that answers a received request.
 *
 * @details Copied from @p rx [PRS_SOMEIP_00190]:
 *          - Message ID (serviceId, methodId)
 *          - Request ID (clientId, sessionId)                          [PRS_SOMEIP_00704]
 *          - Interface Version
 *          Set:
 *          - Protocol Version = rx->protocolVersion, except for SOMEIP_E_WRONG_PROTOCOL_VERSION where it is
 *            SOMEIP_PROTOCOL_VERSION (0x01) (Figure 4.21)
 *          - Message Type     = SOMEIP_MT_ERROR (0x81), TP-Flag never set
 *          - Return Code      = @p rc
 *          - Length           = SOMEIP_HEADER_PART2_SIZE (8): the payload is NOT copied [PRS_SOMEIP_00190]
 *
 * @param[in]  rx   Header of the received request.
 * @param[in]  rc   Return Code to send (must not be SOMEIP_E_OK).
 * @param[out] err  ERROR header, ready for SomeIp_Header_Encode().
 *
 * @retval SOMEIP_RES_OK       @p err is valid.
 * @retval SOMEIP_RES_E_PARAM  @p rx or @p err is NULL, or @p rc is SOMEIP_E_OK.
 *
 * @see     [PRS_SOMEIP_00190] [PRS_SOMEIP_00704] [PRS_SOMEIP_00904] Figure 4.21
 */
SomeIp_Result_t SomeIp_Error_BuildHeader(const SomeIp_Header_t *rx, SomeIp_ReturnCode_t rc, SomeIp_Header_t *err)
{
    SomeIp_ServiceId_t serviceId;
    SomeIp_MethodId_t methodId;
    SomeIp_ClientId_t clientId;
    SomeIp_SessionId_t sessionId;
    SomeIp_InterfaceVersion_t interfaceVersion;
    SomeIp_ProtocolVersion_t protocolVersion;

    if (rx == NULL || err == NULL || rc == SOMEIP_E_OK)
    {
        return SOMEIP_RES_E_PARAM;
    }

    /* Save fields before writing. This also supports rx == err. */
    serviceId        = rx->serviceId;
    methodId         = rx->methodId;
    clientId         = rx->clientId;
    sessionId        = rx->sessionId;
    interfaceVersion = rx->interfaceVersion;
    protocolVersion  = rx->protocolVersion;

    /* Copy Message ID and Request ID */
    err->serviceId        = serviceId;
    err->methodId         = methodId;
    err->clientId         = clientId;
    err->sessionId        = sessionId;
    err->interfaceVersion = interfaceVersion;

    /* Set Protocol Version */
    if (rc == SOMEIP_E_WRONG_PROTOCOL_VERSION)
    {
        err->protocolVersion = SOMEIP_PROTOCOL_VERSION;
    }
    else 
    {
        err->protocolVersion = protocolVersion;
    }

    /* Set Message Type */
    err->messageType = SOMEIP_MT_ERROR;

    /* Set Return Code */
    err->returnCode = rc;

    /* Set Length: no payload */
    err->length = SOMEIP_HEADER_PART2_SIZE;

    return SOMEIP_RES_OK;
}

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
