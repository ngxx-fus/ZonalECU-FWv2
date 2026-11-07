

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_ENDIAN_H__
#define __SOMEIP_ENDIAN_H__

/** @brief SOME/IP byte order helpers (chapter 4.1.3 / 4.1.4.1)
 *  @note  Header and TP header are always big-endian [PRS_SOMEIP_00368] [PRS_SOMEIP_00931].
 *         Payload byte order is configurable per parameter [PRS_SOMEIP_00369], Table 4.6.
 *         Receivers must handle unaligned messages, so every access is byte-wise [PRS_SOMEIP_00142]. */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Types.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/** @brief Sizes of the multi-byte integers on the wire */
enum e_someip_int_size {
    SOMEIP_U16_SIZE = 2,
    SOMEIP_U32_SIZE = 4,
    SOMEIP_U64_SIZE = 8,
};

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/* Big-endian: header, TP header, default payload */

/** @brief Read a big-endian uint16, e.g. {0x12, 0x34} -> 0x1234
 *  @param p_buf Source, at least 2 bytes, any alignment
 *  @return Decoded value in host byte order */
uint16_t SOMEIP_ReadU16Be(IN uint8_t *p_buf);

/** @brief Read a big-endian uint32, e.g. {0xDE, 0xAD, 0xBE, 0xEF} -> 0xDEADBEEF
 *  @param p_buf Source, at least 4 bytes, any alignment
 *  @return Decoded value in host byte order */
uint32_t SOMEIP_ReadU32Be(IN uint8_t *p_buf);

/** @brief Read a big-endian uint64, e.g. {0x01, 0x02, ..., 0x08} -> 0x0102030405060708
 *  @param p_buf Source, at least 8 bytes, any alignment
 *  @return Decoded value in host byte order */
uint64_t SOMEIP_ReadU64Be(IN uint8_t *p_buf);

/** @brief Write a uint16 as big-endian, e.g. 0x1234 -> {0x12, 0x34}
 *  @param p_buf Destination, at least 2 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU16Be(OUT uint8_t *p_buf, IN uint16_t value);

/** @brief Write a uint32 as big-endian, e.g. 0xDEADBEEF -> {0xDE, 0xAD, 0xBE, 0xEF}
 *  @param p_buf Destination, at least 4 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU32Be(OUT uint8_t *p_buf, IN uint32_t value);

/** @brief Write a uint64 as big-endian, e.g. 0x0102030405060708 -> {0x01, 0x02, ..., 0x08}
 *  @param p_buf Destination, at least 8 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU64Be(OUT uint8_t *p_buf, IN uint64_t value);

/* Little-endian: payload parameters configured as little-endian [PRS_SOMEIP_00369] */

/** @brief Read a little-endian uint16, e.g. {0x34, 0x12} -> 0x1234
 *  @param p_buf Source, at least 2 bytes, any alignment
 *  @return Decoded value in host byte order */
uint16_t SOMEIP_ReadU16Le(IN uint8_t *p_buf);

/** @brief Read a little-endian uint32, e.g. {0xEF, 0xBE, 0xAD, 0xDE} -> 0xDEADBEEF
 *  @param p_buf Source, at least 4 bytes, any alignment
 *  @return Decoded value in host byte order */
uint32_t SOMEIP_ReadU32Le(IN uint8_t *p_buf);

/** @brief Read a little-endian uint64, e.g. {0x08, 0x07, ..., 0x01} -> 0x0102030405060708
 *  @param p_buf Source, at least 8 bytes, any alignment
 *  @return Decoded value in host byte order */
uint64_t SOMEIP_ReadU64Le(IN uint8_t *p_buf);

/** @brief Write a uint16 as little-endian, e.g. 0x1234 -> {0x34, 0x12}
 *  @param p_buf Destination, at least 2 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU16Le(OUT uint8_t *p_buf, IN uint16_t value);

/** @brief Write a uint32 as little-endian, e.g. 0xDEADBEEF -> {0xEF, 0xBE, 0xAD, 0xDE}
 *  @param p_buf Destination, at least 4 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU32Le(OUT uint8_t *p_buf, IN uint32_t value);

/** @brief Write a uint64 as little-endian, e.g. 0x0102030405060708 -> {0x08, 0x07, ..., 0x01}
 *  @param p_buf Destination, at least 8 bytes, any alignment
 *  @param value Value in host byte order */
void SOMEIP_WriteU64Le(OUT uint8_t *p_buf, IN uint64_t value);

/* Runtime byte order: the order comes from the parameter configuration */

/** @brief Read a uint16 in the given byte order
 *  @param p_buf Source, at least 2 bytes, any alignment
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is read as big-endian
 *  @return Decoded value in host byte order */
uint16_t SOMEIP_ReadU16(IN uint8_t *p_buf, IN someip_byte_order_t order);

/** @brief Read a uint32 in the given byte order
 *  @param p_buf Source, at least 4 bytes, any alignment
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is read as big-endian
 *  @return Decoded value in host byte order */
uint32_t SOMEIP_ReadU32(IN uint8_t *p_buf, IN someip_byte_order_t order);

/** @brief Read a uint64 in the given byte order
 *  @param p_buf Source, at least 8 bytes, any alignment
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is read as big-endian
 *  @return Decoded value in host byte order */
uint64_t SOMEIP_ReadU64(IN uint8_t *p_buf, IN someip_byte_order_t order);

/** @brief Write a uint16 in the given byte order
 *  @param p_buf Destination, at least 2 bytes, any alignment
 *  @param value Value in host byte order
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is written as big-endian */
void SOMEIP_WriteU16(OUT uint8_t *p_buf, IN uint16_t value, IN someip_byte_order_t order);

/** @brief Write a uint32 in the given byte order
 *  @param p_buf Destination, at least 4 bytes, any alignment
 *  @param value Value in host byte order
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is written as big-endian */
void SOMEIP_WriteU32(OUT uint8_t *p_buf, IN uint32_t value, IN someip_byte_order_t order);

/** @brief Write a uint64 in the given byte order
 *  @param p_buf Destination, at least 8 bytes, any alignment
 *  @param value Value in host byte order
 *  @param order SOMEIP_BYTE_ORDER_LITTLE_ENDIAN, anything else is written as big-endian */
void SOMEIP_WriteU64(OUT uint8_t *p_buf, IN uint64_t value, IN someip_byte_order_t order);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_ENDIAN_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
