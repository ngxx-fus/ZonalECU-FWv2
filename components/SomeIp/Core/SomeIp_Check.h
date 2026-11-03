/**
 ************************************************************************************************************
 * @file    SomeIp_Check.h
 * @brief   SOME/IP message validation and error message: public API.
 *
 * @details - Check a received header in the order of Figure 4.21     [PRS_SOMEIP_00195] [PRS_SOMEIP_00576]
 *          - Decide whether an ERROR message may be sent back         [PRS_SOMEIP_00188] [PRS_SOMEIP_00189]
 *                                                                    [PRS_SOMEIP_00537] [PRS_SOMEIP_00539]
 *          - Build the header of the ERROR message                    [PRS_SOMEIP_00190]
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.2.6 (Error Handling), Figure 4.21
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_CHECK_H__
#define __SOMEIP_CHECK_H__

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include "Macros.h"
#include "SomeIp_Types.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
ZE_HEADER

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* TYPEDEFS *************************************************************************************************
 ************************************************************************************************************/

/* One method / event of a service, as configured */
typedef struct
{
    SomeIp_MethodId_t    methodId;               /* method (bit15 = 0) or event (bit15 = 1)               */
    SomeIp_MessageType_t expectedType;           /* SOMEIP_MT_REQUEST           : request/response        */
                                                 /* SOMEIP_MT_REQUEST_NO_RETURN : fire&forget             */
                                                 /* SOMEIP_MT_NOTIFICATION      : event / field notifier  */
} SomeIp_MethodConfig_t;

/* One service, as configured */
typedef struct
{
    SomeIp_ServiceId_t           serviceId;
    SomeIp_InterfaceVersion_t    interfaceVersion;   /* major version of the interface [PRS_SOMEIP_00053] */
    const SomeIp_MethodConfig_t *methods;
    uint16_t                     methodCount;
} SomeIp_ServiceConfig_t;

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/*
 * Check a received header against the configuration, in the order of Figure 4.21 (steps 1..6).
 *
 * @return SOMEIP_RES_OK             *rc and *method are valid (*rc = SOMEIP_E_OK if all checks pass)
 *         SOMEIP_RES_E_PARAM        rx, rc or method is NULL, or services is NULL with serviceCount > 0
 */
SomeIp_Result_t SomeIp_Check_Header(const SomeIp_Header_t       *rx,
                                    const SomeIp_ServiceConfig_t *services,
                                    uint16_t                      serviceCount,
                                    SomeIp_ReturnCode_t          *rc,
                                    const SomeIp_MethodConfig_t **method);

/*
 * Decide whether an ERROR message may be sent back for a received message that failed a check.
 *
 * @return 1 an ERROR message shall be sent, 0 the message shall be dropped silently (or NULL argument)
 */
uint8_t SomeIp_Check_ShouldReplyError(const SomeIp_Header_t *rx, const SomeIp_MethodConfig_t *method);

/*
 * Build the header of the ERROR message that answers rx.
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        rx or err is NULL, or rc == SOMEIP_E_OK
 */
SomeIp_Result_t SomeIp_Error_BuildHeader(const SomeIp_Header_t *rx, SomeIp_ReturnCode_t rc, SomeIp_Header_t *err);

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_CHECK_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
