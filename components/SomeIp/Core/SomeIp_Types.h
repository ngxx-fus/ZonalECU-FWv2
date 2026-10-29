/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_TYPES_H__
#define __SOMEIP_TYPES_H__

/*
 * SOME/IP basic definitions.
 *
 * Reference : AUTOSAR_PRS_SOMEIPProtocol
 */

/* INCLUDES *************************************************************************************************
 ************************************************************************************************************/
#include <stdint.h>
#include <assert.h>

#include "Macros.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
ZE_HEADER

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * 1. HEADER LAYOUT                                                       [PRS_SOMEIP_00030] [PRS_SOMEIP_00368]
 *  All header fields are big-endian (network byte order).
 * ======================================================================================================== */
#define SOMEIP_HEADER_SIZE                      (16u)   /* full header                                    */
#define SOMEIP_HEADER_PART1_SIZE                (8u)    /* Message ID + Length: NOT counted by Length     */
#define SOMEIP_HEADER_PART2_SIZE                (8u)    /* Request ID .. Return Code: counted by Length   */

/* Byte offsets inside the header */
#define SOMEIP_OFS_SERVICE_ID                   (0u)
#define SOMEIP_OFS_METHOD_ID                    (2u)
#define SOMEIP_OFS_LENGTH                       (4u)
#define SOMEIP_OFS_CLIENT_ID                    (8u)
#define SOMEIP_OFS_SESSION_ID                   (10u)
#define SOMEIP_OFS_PROTOCOL_VERSION             (12u)
#define SOMEIP_OFS_INTERFACE_VERSION            (13u)
#define SOMEIP_OFS_MESSAGE_TYPE                 (14u)
#define SOMEIP_OFS_RETURN_CODE                  (15u)
#define SOMEIP_OFS_PAYLOAD                      (16u)

/* Length = SOMEIP_HEADER_PART2_SIZE + payload length                                       [PRS_SOMEIP_00042] */
#define SOMEIP_LENGTH_MIN                       (SOMEIP_HEADER_PART2_SIZE)


/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_TYPES_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
