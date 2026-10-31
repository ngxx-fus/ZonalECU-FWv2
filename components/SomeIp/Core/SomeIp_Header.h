/* GUARD/BEGIN **********************************************************************************************
 ************************************************************************************************************/
#ifndef __SOMEIP_HEADER_H__
#define __SOMEIP_HEADER_H__

/*
 * SOME/IP header codec.
 *
 *  - Header (16 bytes) <-> SomeIp_Header_t                                    [PRS_SOMEIP_00030] [PRS_SOMEIP_00368]
 *  - Split one UDP datagram / TCP chunk into SOME/IP messages                 [PRS_SOMEIP_00535] [PRS_SOMEIP_00910]
 *  - TP header (4 bytes) <-> SomeIp_TpHeader_t                                [PRS_SOMEIP_00723] [PRS_SOMEIP_00931]
 *
 * Reference : AUTOSAR_PRS_SOMEIPProtocol, chapter 4.1.2 / 4.2.1 / 4.2.1.4 / 4.2.6.3
 */

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

/* PUBLIC FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/*
 * Decode the 16-byte SOME/IP header at buf into *hdr (host byte order).
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        buf or hdr is NULL
 *         SOMEIP_RES_E_SHORT_INPUT  len < SOMEIP_HEADER_SIZE
 */
SomeIp_Result_t SomeIp_Header_Decode(const uint8_t *buf, uint32_t len, SomeIp_Header_t *hdr);

/*
 * Encode *hdr into 16 bytes at buf (big-endian).
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        hdr or buf is NULL, or hdr->length < SOMEIP_LENGTH_MIN
 *         SOMEIP_RES_E_NO_SPACE     cap < SOMEIP_HEADER_SIZE
 */
SomeIp_Result_t SomeIp_Header_Encode(const SomeIp_Header_t *hdr, uint8_t *buf, uint32_t cap);

/*
 * Get the next SOME/IP message from a buffer that may hold several messages back to back [00535].
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        buf, pos or msg is NULL, or *pos > len
 *         SOMEIP_RES_E_MALFORMED    one of the [00910] checks failed
 */
SomeIp_Result_t SomeIp_Message_Next(const uint8_t *buf, uint32_t len, uint32_t *pos, SomeIp_Message_t *msg);

/*
 * Decode the 4-byte TP header found at the start of the payload of a TP segment (TP-Flag set).
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        p or tp is NULL
 *         SOMEIP_RES_E_SHORT_INPUT  len < SOMEIP_TP_HEADER_SIZE
 */
SomeIp_Result_t SomeIp_TpHeader_Decode(const uint8_t *p, uint32_t len, SomeIp_TpHeader_t *tp);

/*
 * Encode *tp into 4 bytes at p. Reserved bits are written as 0 [00726].
 *
 * @return SOMEIP_RES_OK
 *         SOMEIP_RES_E_PARAM        tp or p is NULL, or tp->offset is not a multiple of 16 [00724]
 *         SOMEIP_RES_E_NO_SPACE     cap < SOMEIP_TP_HEADER_SIZE
 */
SomeIp_Result_t SomeIp_TpHeader_Encode(const SomeIp_TpHeader_t *tp, uint8_t *p, uint32_t cap);

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_HEADER_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
