/**
 ************************************************************************************************************
 * @file    SomeIp_Session.h
 * @brief   SOME/IP Session ID handling: public API.
 *
 * @details - Generate Session IDs for outgoing requests / events     [PRS_SOMEIP_00932] [PRS_SOMEIP_00933]
 *                                                                    [PRS_SOMEIP_00934] [PRS_SOMEIP_00533]
 *          - Wrap-around 0xFFFF -> 0x0001                            [PRS_SOMEIP_00521]
 *          - Match a received response with its request              [PRS_SOMEIP_00739]
 *
 * @see     AUTOSAR_PRS_SOMEIPProtocol (R22-11), chapter 4.1.2.4 (Request ID)
 ************************************************************************************************************
 */

/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_SESSION_H__
#define __SOMEIP_SESSION_H__

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

/* Session counter of one sender */
typedef struct
{
    SomeIp_SessionId_t nextId;                   /* value returned by the next SomeIp_Session_Next()      */
    uint8_t            active;                   /* 1 = session handling active, 0 = always 0x0000        */
} SomeIp_Session_t;

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/*
 * Initialize a session counter.
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        s is NULL
 */
SomeIp_Result_t SomeIp_Session_Init(SomeIp_Session_t *s, uint8_t active);

/*
 * Get the Session ID for the next outgoing message, then advance the counter.
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        s or id is NULL
 */
SomeIp_Result_t SomeIp_Session_Next(SomeIp_Session_t *s, SomeIp_SessionId_t *id);

/*
 * Check whether a received response belongs to a sent request (same Request ID).
 *
 * @return 1 match, 0 no match (or NULL argument)
 */
uint8_t SomeIp_Session_IsMatch(const SomeIp_Header_t *request, const SomeIp_Header_t *response);

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_SESSION_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
