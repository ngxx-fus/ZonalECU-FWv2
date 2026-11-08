

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include "SOMEIP_Endian.h"

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/** @brief Bit shift that moves a byte to position 1..3 of a uint32, and the high half of a uint64 */
#define SOMEIP_SHIFT_BYTE1    (8U)
#define SOMEIP_SHIFT_BYTE2    (16U)
#define SOMEIP_SHIFT_BYTE3    (24U)
#define SOMEIP_SHIFT_HIGH_U32 (32U)

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

/* Big-endian */

uint16_t SOMEIP_ReadU16Be(IN uint8_t *p_buf) {
    /* Most significant byte first */
    return (uint16_t)(((uint16_t)p_buf[0] << SOMEIP_SHIFT_BYTE1) | (uint16_t)p_buf[1]);
}

uint32_t SOMEIP_ReadU32Be(IN uint8_t *p_buf) {
    /* Most significant byte first */
    return ((uint32_t)p_buf[0] << SOMEIP_SHIFT_BYTE3) | ((uint32_t)p_buf[1] << SOMEIP_SHIFT_BYTE2) |
           ((uint32_t)p_buf[2] << SOMEIP_SHIFT_BYTE1) | ((uint32_t)p_buf[3]);
}

uint64_t SOMEIP_ReadU64Be(IN uint8_t *p_buf) {
    /* High half in bytes 0..3; cast to uint64_t before shifting by 32 */
    return ((uint64_t)SOMEIP_ReadU32Be(&p_buf[0]) << SOMEIP_SHIFT_HIGH_U32) |
           ((uint64_t)SOMEIP_ReadU32Be(&p_buf[SOMEIP_U32_SIZE]));
}

void SOMEIP_WriteU16Be(OUT uint8_t *p_buf, IN uint16_t value) {
    p_buf[0] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE1);
    p_buf[1] = (uint8_t)(value);
}

void SOMEIP_WriteU32Be(OUT uint8_t *p_buf, IN uint32_t value) {
    p_buf[0] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE3);
    p_buf[1] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE2);
    p_buf[2] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE1);
    p_buf[3] = (uint8_t)(value);
}

void SOMEIP_WriteU64Be(OUT uint8_t *p_buf, IN uint64_t value) {
    SOMEIP_WriteU32Be(&p_buf[0], (uint32_t)(value >> SOMEIP_SHIFT_HIGH_U32));
    SOMEIP_WriteU32Be(&p_buf[SOMEIP_U32_SIZE], (uint32_t)(value));
}

/* Little-endian */

uint16_t SOMEIP_ReadU16Le(IN uint8_t *p_buf) {
    /* Least significant byte first */
    return (uint16_t)(((uint16_t)p_buf[1] << SOMEIP_SHIFT_BYTE1) | (uint16_t)p_buf[0]);
}

uint32_t SOMEIP_ReadU32Le(IN uint8_t *p_buf) {
    /* Least significant byte first */
    return ((uint32_t)p_buf[3] << SOMEIP_SHIFT_BYTE3) | ((uint32_t)p_buf[2] << SOMEIP_SHIFT_BYTE2) |
           ((uint32_t)p_buf[1] << SOMEIP_SHIFT_BYTE1) | ((uint32_t)p_buf[0]);
}

uint64_t SOMEIP_ReadU64Le(IN uint8_t *p_buf) {
    /* High half in bytes 4..7 */
    return ((uint64_t)SOMEIP_ReadU32Le(&p_buf[SOMEIP_U32_SIZE]) << SOMEIP_SHIFT_HIGH_U32) |
           ((uint64_t)SOMEIP_ReadU32Le(&p_buf[0]));
}

void SOMEIP_WriteU16Le(OUT uint8_t *p_buf, IN uint16_t value) {
    p_buf[0] = (uint8_t)(value);
    p_buf[1] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE1);
}

void SOMEIP_WriteU32Le(OUT uint8_t *p_buf, IN uint32_t value) {
    p_buf[0] = (uint8_t)(value);
    p_buf[1] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE1);
    p_buf[2] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE2);
    p_buf[3] = (uint8_t)(value >> SOMEIP_SHIFT_BYTE3);
}

void SOMEIP_WriteU64Le(OUT uint8_t *p_buf, IN uint64_t value) {
    SOMEIP_WriteU32Le(&p_buf[0], (uint32_t)(value));
    SOMEIP_WriteU32Le(&p_buf[SOMEIP_U32_SIZE], (uint32_t)(value >> SOMEIP_SHIFT_HIGH_U32));
}

/* Runtime byte order */

uint16_t SOMEIP_ReadU16(IN uint8_t *p_buf, IN someip_byte_order_t order) {
    /* Dispatch on the configured byte order, big-endian is the default */
    return (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) ? SOMEIP_ReadU16Le(p_buf) : SOMEIP_ReadU16Be(p_buf);
}

uint32_t SOMEIP_ReadU32(IN uint8_t *p_buf, IN someip_byte_order_t order) {
    /* Dispatch on the configured byte order, big-endian is the default */
    return (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) ? SOMEIP_ReadU32Le(p_buf) : SOMEIP_ReadU32Be(p_buf);
}

uint64_t SOMEIP_ReadU64(IN uint8_t *p_buf, IN someip_byte_order_t order) {
    /* Dispatch on the configured byte order, big-endian is the default */
    return (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) ? SOMEIP_ReadU64Le(p_buf) : SOMEIP_ReadU64Be(p_buf);
}

void SOMEIP_WriteU16(OUT uint8_t *p_buf, IN uint16_t value, IN someip_byte_order_t order) {
    /* Parameter configured as little-endian */
    if (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) {
        SOMEIP_WriteU16Le(p_buf, value);
    }

    /* Default: big-endian, same as the header */
    else {
        SOMEIP_WriteU16Be(p_buf, value);
    }
}

void SOMEIP_WriteU32(OUT uint8_t *p_buf, IN uint32_t value, IN someip_byte_order_t order) {
    /* Parameter configured as little-endian */
    if (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) {
        SOMEIP_WriteU32Le(p_buf, value);
    }

    /* Default: big-endian, same as the header */
    else {
        SOMEIP_WriteU32Be(p_buf, value);
    }
}

void SOMEIP_WriteU64(OUT uint8_t *p_buf, IN uint64_t value, IN someip_byte_order_t order) {
    /* Parameter configured as little-endian */
    if (SOMEIP_BYTE_ORDER_LITTLE_ENDIAN == order) {
        SOMEIP_WriteU64Le(p_buf, value);
    }

    /* Default: big-endian, same as the header */
    else {
        SOMEIP_WriteU64Be(p_buf, value);
    }
}

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
