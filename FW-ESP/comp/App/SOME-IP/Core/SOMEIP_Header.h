

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_HEADER_H__
#define __SOMEIP_HEADER_H__

/** @brief SOME/IP header codec: public API (chapter 4.1.2 / 4.2.1 / 4.2.1.4)
 *  @details - Header (16 bytes) <-> someip_header_t [PRS_SOMEIP_00030] [PRS_SOMEIP_00368]
 *           - Split one UDP datagram / TCP chunk into SOME/IP messages [PRS_SOMEIP_00535] [PRS_SOMEIP_00910]
 *           - TP header (4 bytes) <-> someip_tp_header_t [PRS_SOMEIP_00723] [PRS_SOMEIP_00931]
 *  @note    Every access is byte-wise, so all buffers may be unaligned [PRS_SOMEIP_00142]. */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SomeIp_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/** @brief Decode the 16-byte SOME/IP header into host byte order
 *  @details Every field is read at its fixed offset (SOMEIP_OFS_*), the 16/32-bit fields are converted from
 *           big-endian to host byte order [PRS_SOMEIP_00030] [PRS_SOMEIP_00031] [PRS_SOMEIP_00368].
 *  @param p_buf   First byte of the SOME/IP header
 *  @param buf_len Number of valid bytes at p_buf
 *  @param p_hdr   Decoded header
 *  @retval SOMEIP_RES_OK            Header decoded
 *  @retval SOMEIP_RES_E_PARAM       p_buf or p_hdr is NULL
 *  @retval SOMEIP_RES_E_SHORT_INPUT buf_len < SOMEIP_HEADER_SIZE (16) */
someip_result_t SOMEIP_HeaderDecode(IN uint8_t *p_buf, IN uint32_t buf_len, OUT someip_header_t *p_hdr);

/** @brief Encode a header into 16 bytes (big-endian), the payload is NOT written
 *  @note  All checks are done before the first byte is written, so p_buf is untouched on error
 *         [PRS_SOMEIP_00030] [PRS_SOMEIP_00042] [PRS_SOMEIP_00368].
 *  @param p_hdr    Header to encode, p_hdr->length = SOMEIP_HEADER_PART2_SIZE + payload length
 *  @param p_buf    Destination
 *  @param buf_size Capacity of p_buf in bytes
 *  @retval SOMEIP_RES_OK         16 bytes written
 *  @retval SOMEIP_RES_E_PARAM    p_hdr or p_buf is NULL, or p_hdr->length < SOMEIP_LENGTH_MIN (8)
 *  @retval SOMEIP_RES_E_NO_SPACE buf_size < SOMEIP_HEADER_SIZE (16) */
someip_result_t SOMEIP_HeaderEncode(IN someip_header_t *p_hdr, OUT uint8_t *p_buf, IN uint32_t buf_size);

/** @brief Get the next SOME/IP message from a buffer that may hold several messages back to back
 *  @details One UDP datagram / TCP segment may carry more than one message [PRS_SOMEIP_00535], the end of
 *           each message is found with its Length field [PRS_SOMEIP_00140]. Checks [PRS_SOMEIP_00910]:
 *           - remaining bytes >= SOMEIP_HEADER_SIZE
 *           - Length >= SOMEIP_LENGTH_MIN
 *           - Length <= remaining - SOMEIP_HEADER_PART1_SIZE (written as a subtraction: no overflow)
 *  @param p_buf   Received datagram / stream chunk
 *  @param buf_len Number of valid bytes in p_buf
 *  @param p_pos   In: offset of the message to read. Out: offset of the next message (success only)
 *  @param p_msg   Decoded header, payload pointer (into p_buf) and payload length
 *  @retval SOMEIP_RES_OK          Message extracted, *p_pos advanced by SOMEIP_HEADER_PART1_SIZE + Length
 *  @retval SOMEIP_RES_E_PARAM     p_buf, p_pos or p_msg is NULL, or *p_pos > buf_len
 *  @retval SOMEIP_RES_E_MALFORMED One of the [PRS_SOMEIP_00910] checks failed */
someip_result_t SOMEIP_MessageNext(IN uint8_t *p_buf, IN uint32_t buf_len, INOUT uint32_t *p_pos,
                                   OUT someip_message_t *p_msg);

/** @brief Decode the 4-byte SOME/IP-TP header at the start of the payload of a TP segment
 *  @details Only for messages with the TP-Flag (0x20) set in the Message Type [PRS_SOMEIP_00367].
 *           Layout, big-endian [PRS_SOMEIP_00931], bits from highest to lowest [PRS_SOMEIP_00723]:
 *           @verbatim
 *            31                                4   3   2   1   0
 *           +-----------------------------------+---+---+---+---+
 *           |          Offset [28 bit]          | R | R | R | M |
 *           +-----------------------------------+---+---+---+---+
 *           @endverbatim
 *           - offset = raw & SOMEIP_TP_OFFSET_MASK, already in bytes, no shift [PRS_SOMEIP_00724] [00725]
 *           - more_segments = raw & SOMEIP_TP_MORE_SEGMENTS_MASK [PRS_SOMEIP_00727]
 *           - Reserved bits are ignored [PRS_SOMEIP_00726]
 *  @param p_buf   TP header (= p_payload of the segment)
 *  @param buf_len Number of valid bytes at p_buf (= payload_len)
 *  @param p_tp    Decoded TP header
 *  @retval SOMEIP_RES_OK            TP header decoded
 *  @retval SOMEIP_RES_E_PARAM       p_buf or p_tp is NULL
 *  @retval SOMEIP_RES_E_SHORT_INPUT buf_len < SOMEIP_TP_HEADER_SIZE (4) */
someip_result_t SOMEIP_TpHeaderDecode(IN uint8_t *p_buf, IN uint32_t buf_len, OUT someip_tp_header_t *p_tp);

/** @brief Encode a TP header into 4 bytes (big-endian)
 *  @details raw = offset | (more_segments ? SOMEIP_TP_MORE_SEGMENTS_MASK : 0)
 *           - offset is written as is, in bytes, no shift [PRS_SOMEIP_00725]
 *           - Reserved bits are always written as 0 [PRS_SOMEIP_00726]
 *           - More Segments = 1 for all segments but the last one [PRS_SOMEIP_00727]
 *  @note    All checks are done before writing, so p_buf is untouched on error.
 *  @param p_tp     TP header to encode, p_tp->offset must be a multiple of 16
 *  @param p_buf    Destination (normally SOMEIP_OFS_PAYLOAD of the segment buffer)
 *  @param buf_size Capacity of p_buf in bytes
 *  @retval SOMEIP_RES_OK         4 bytes written
 *  @retval SOMEIP_RES_E_PARAM    p_tp or p_buf is NULL, or offset not a multiple of 16 [PRS_SOMEIP_00724]
 *  @retval SOMEIP_RES_E_NO_SPACE buf_size < SOMEIP_TP_HEADER_SIZE (4) */
someip_result_t SOMEIP_TpHeaderEncode(IN someip_tp_header_t *p_tp, OUT uint8_t *p_buf,
                                      IN uint32_t buf_size);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_HEADER_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
