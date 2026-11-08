

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __SOMEIP_TYPES_H__
#define __SOMEIP_TYPES_H__

/** @brief SOME/IP basic definitions: header layout, wire constants and common types
 *  @see   AUTOSAR_PRS_SOMEIPProtocol (FO R22-11) */

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

HEADER_CPP

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/** @brief Supported protocol version [PRS_SOMEIP_00051] */
#define SOMEIP_PROTOCOL_VERSION (0x01U)

/** @brief Build Message ID (Service ID << 16 | Method ID) and Request ID (Client ID << 16 | Session ID)
 *  @see   [PRS_SOMEIP_00034] */
#define SOMEIP_ID_HIGH_SHIFT (16U)
#define SOMEIP_MESSAGE_ID(service_id, method_id)                                                           \
    ((someip_message_id_t)(((uint32_t)(service_id) << SOMEIP_ID_HIGH_SHIFT) | (uint32_t)(method_id)))
#define SOMEIP_REQUEST_ID(client_id, session_id)                                                           \
    ((someip_request_id_t)(((uint32_t)(client_id) << SOMEIP_ID_HIGH_SHIFT) | (uint32_t)(session_id)))

/** @brief Message Type flag bits: response bit and "TP-Flag"                  [PRS_SOMEIP_00055] [00367] */
#define SOMEIP_MT_FLAG_RESPONSE (0x80U)
#define SOMEIP_MT_FLAG_TP       (0x20U)

/** @brief SOME/IP-TP header bit fields, 4 bytes big-endian right after the SOME/IP header
 *  @note  | Offset [31:4] (unit = 16 bytes) | Reserved [3:1] | More Segments [0] |
 *         [PRS_SOMEIP_00723] [00724] [00726] [00727] [00931] */
#define SOMEIP_TP_OFFSET_MASK        (0xFFFFFFF0U)
#define SOMEIP_TP_RESERVED_MASK      (0x0000000EU) /*!< Sender writes 0, receiver ignores */
#define SOMEIP_TP_MORE_SEGMENTS_MASK (0x00000001U)

/** @brief Boolean on the wire is 8 bit, only bit 0 is evaluated [PRS_SOMEIP_00615] */
#define SOMEIP_BOOL_VALUE_MASK (0x01U)

/** @brief Union type field value reserved for NULL (empty union) [PRS_SOMEIP_00907] */
#define SOMEIP_UNION_TYPE_NULL (0U)

#define SOMEIP_IPV4_ADDR_SIZE (4U)

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/** @brief Header sizes; all header fields are big-endian           [PRS_SOMEIP_00030] [00368] [00042]
 *  @note  Part 1 (Message ID + Length) is NOT counted by Length, part 2 (Request ID .. Return Code) is.
 *         Length = SOMEIP_HEADER_PART2_SIZE + payload length. */
enum e_someip_header_size {
    SOMEIP_HEADER_SIZE = 16,
    SOMEIP_HEADER_PART1_SIZE = 8,
    SOMEIP_HEADER_PART2_SIZE = 8,
    SOMEIP_LENGTH_MIN = SOMEIP_HEADER_PART2_SIZE,
};

/** @brief Byte offsets of the fields inside the SOME/IP header */
enum e_someip_header_offset {
    SOMEIP_OFS_SERVICE_ID = 0,
    SOMEIP_OFS_METHOD_ID = 2,
    SOMEIP_OFS_LENGTH = 4,
    SOMEIP_OFS_CLIENT_ID = 8,
    SOMEIP_OFS_SESSION_ID = 10,
    SOMEIP_OFS_PROTOCOL_VERSION = 12,
    SOMEIP_OFS_INTERFACE_VERSION = 13,
    SOMEIP_OFS_MESSAGE_TYPE = 14,
    SOMEIP_OFS_RETURN_CODE = 15,
    SOMEIP_OFS_PAYLOAD = 16,
};

/** @brief Method ID ranges: bit 15 = 0 method (0x0000-0x7FFF), 1 event/notification (0x8000-0xFFFF) */
enum e_someip_method_id {
    SOMEIP_METHOD_ID_MAX = 0x7FFF,
    SOMEIP_EVENT_ID_FLAG = 0x8000,
};

/** @brief Session ID values                                          [PRS_SOMEIP_00932] [00933] [00521] */
enum e_someip_session_id {
    SOMEIP_SESSION_ID_INACTIVE = 0x0000, /*!< Session handling not active */
    SOMEIP_SESSION_ID_MIN = 0x0001,      /*!< First value, and value after wrap-around */
    SOMEIP_SESSION_ID_MAX = 0xFFFF,
};

/** @brief Message Type values (Table 4.4)                                        [PRS_SOMEIP_00055] [00367]
 *  @note  Stored on the wire / in structs as someip_message_type_t (uint8_t), never as this enum. */
enum e_someip_message_type {
    SOMEIP_MT_REQUEST = 0x00,           /*!< Request expecting a response (even void) */
    SOMEIP_MT_REQUEST_NO_RETURN = 0x01, /*!< Fire & forget request */
    SOMEIP_MT_NOTIFICATION = 0x02,      /*!< Event / field notification */
    SOMEIP_MT_TP_REQUEST = 0x20,
    SOMEIP_MT_TP_REQUEST_NO_RETURN = 0x21,
    SOMEIP_MT_TP_NOTIFICATION = 0x22,
    SOMEIP_MT_RESPONSE = 0x80,
    SOMEIP_MT_ERROR = 0x81, /*!< Response containing an error */
    SOMEIP_MT_TP_RESPONSE = 0xA0,
    SOMEIP_MT_TP_ERROR = 0xA1,
};

/** @brief Return Code values (Table 4.11) [PRS_SOMEIP_00191]
 *  @note  REQUEST / REQUEST_NO_RETURN / NOTIFICATION: always E_OK (Table 4.5).
 *         RESPONSE: any code (normally E_OK). ERROR: any code except E_OK.
 *         Stored as someip_return_code_t (uint8_t), never as this enum. */
enum e_someip_return_code {
    SOMEIP_E_OK = 0x00,
    SOMEIP_E_NOT_OK = 0x01, /*!< Unspecified error */
    SOMEIP_E_UNKNOWN_SERVICE = 0x02,
    SOMEIP_E_UNKNOWN_METHOD = 0x03,
    SOMEIP_E_NOT_READY = 0x04,     /*!< Service/method known, application not running */
    SOMEIP_E_NOT_REACHABLE = 0x05, /*!< Internal only, never sent */
    SOMEIP_E_TIMEOUT = 0x06,       /*!< Internal only, never sent */
    SOMEIP_E_WRONG_PROTOCOL_VERSION = 0x07,
    SOMEIP_E_WRONG_INTERFACE_VERSION = 0x08,
    SOMEIP_E_MALFORMED_MESSAGE = 0x09, /*!< Payload cannot be deserialized */
    SOMEIP_E_WRONG_MESSAGE_TYPE = 0x0A,
    SOMEIP_E_E2E_REPEATED = 0x0B,
    SOMEIP_E_E2E_WRONG_SEQUENCE = 0x0C,
    SOMEIP_E_E2E = 0x0D, /*!< E2E error, not further specified */
    SOMEIP_E_E2E_NOT_AVAILABLE = 0x0E,
    SOMEIP_E_E2E_NO_NEW_DATA = 0x0F,

    /* Reserved ranges */
    SOMEIP_E_GENERIC_RESERVED_MIN = 0x10, /*!< 0x10..0x1F: reserved for generic errors */
    SOMEIP_E_GENERIC_RESERVED_MAX = 0x1F,
    SOMEIP_E_SERVICE_SPECIFIC_MIN = 0x20, /*!< 0x20..0x5E: defined by the interface spec */
    SOMEIP_E_SERVICE_SPECIFIC_MAX = 0x5E,
};

/** @brief Payload and segment size limits                          chapter 4.1.2.9, [PRS_SOMEIP_00730] */
enum e_someip_transport_limit {
    SOMEIP_UDP_PAYLOAD_MAX = 1400, /*!< SOME/IP payload per message over UDP */
    SOMEIP_UDP_MESSAGE_MAX = SOMEIP_HEADER_SIZE + SOMEIP_UDP_PAYLOAD_MAX,
    SOMEIP_TP_HEADER_SIZE = 4,
    SOMEIP_TP_OFFSET_ALIGNMENT = 16,
    SOMEIP_TP_SEGMENT_MAX = 1392, /*!< Largest 16-byte aligned segment in 1400 bytes */
};

/** @brief Boolean wire values [PRS_SOMEIP_00615] */
enum e_someip_bool {
    SOMEIP_BOOL_FALSE = 0x00,
    SOMEIP_BOOL_TRUE = 0x01,
};

/** @brief String Byte Order Marks, present in fixed and dynamic strings [PRS_SOMEIP_00087] [00372] */
enum e_someip_bom {
    SOMEIP_BOM_UTF8_SIZE = 3,
    SOMEIP_BOM_UTF8_0 = 0xEF,
    SOMEIP_BOM_UTF8_1 = 0xBB,
    SOMEIP_BOM_UTF8_2 = 0xBF,
    SOMEIP_BOM_UTF16_SIZE = 2,
    SOMEIP_BOM_UTF16BE_0 = 0xFE,
    SOMEIP_BOM_UTF16BE_1 = 0xFF,
    SOMEIP_BOM_UTF16LE_0 = 0xFF,
    SOMEIP_BOM_UTF16LE_1 = 0xFE,
};

/* Identifiers */
typedef uint16_t someip_service_id_t;
typedef uint16_t someip_method_id_t; /*!< Method (bit 15 = 0) or event (bit 15 = 1) */
typedef uint16_t someip_event_id_t;  /*!< A method ID with bit 15 = 1 */
typedef uint16_t someip_eventgroup_id_t;
typedef uint16_t someip_client_id_t;
typedef uint16_t someip_session_id_t;
typedef uint32_t someip_message_id_t; /*!< Service ID << 16 | Method ID */
typedef uint32_t someip_request_id_t; /*!< Client ID << 16 | Session ID */
typedef uint32_t someip_length_t;

/* Single-byte header fields, fixed width because they are wire values */
typedef uint8_t someip_protocol_version_t;
typedef uint8_t someip_interface_version_t; /*!< Major interface version [PRS_SOMEIP_00053] */
typedef uint8_t someip_message_type_t;      /*!< Values: enum e_someip_message_type */
typedef uint8_t someip_return_code_t;       /*!< Values: enum e_someip_return_code */

/** @brief Byte order of payload parameters, configurable per parameter [PRS_SOMEIP_00369] */
typedef enum e_someip_byte_order {
    SOMEIP_BYTE_ORDER_BIG_ENDIAN = 0, /*!< Default, same as the header */
    SOMEIP_BYTE_ORDER_LITTLE_ENDIAN = 1,
} someip_byte_order_t;

/** @brief Size of the length field of dynamic array / string, struct, union             [PRS_SOMEIP_00121]
 *  @note  Enumerator value = size in bytes, 0 = no length field. */
typedef enum e_someip_length_field_size {
    SOMEIP_LENGTH_FIELD_NONE = 0,
    SOMEIP_LENGTH_FIELD_8BIT = 1,
    SOMEIP_LENGTH_FIELD_16BIT = 2,
    SOMEIP_LENGTH_FIELD_32BIT = 4, /*!< Default for dynamic arrays and strings */
} someip_length_field_size_t;

/** @brief String encoding [PRS_SOMEIP_00372] */
typedef enum e_someip_string_encoding {
    SOMEIP_STRING_UTF8 = 0,
    SOMEIP_STRING_UTF16BE = 1,
    SOMEIP_STRING_UTF16LE = 2,
} someip_string_encoding_t;

/** @brief Internal API result
 *  @note  NOT a wire value; the wire Return Code is someip_return_code_t. */
typedef enum e_someip_result {
    SOMEIP_RES_OK = 0,
    SOMEIP_RES_E_PARAM = 1,       /*!< NULL pointer / invalid argument */
    SOMEIP_RES_E_SHORT_INPUT = 2, /*!< Fewer input bytes than required */
    SOMEIP_RES_E_NO_SPACE = 3,    /*!< Output buffer too small */
    SOMEIP_RES_E_MALFORMED = 4,   /*!< Length / structure inconsistent */
    SOMEIP_RES_E_NOT_FOUND = 5,   /*!< Unknown service / method / instance in config */
    SOMEIP_RES_E_TRANSPORT = 6,   /*!< Socket / driver error */
    SOMEIP_RES_E_TIMEOUT = 7,
} someip_result_t;

/** @brief Decoded SOME/IP header, fields in HOST byte order
 *  @warning Not a packed image of the wire (ESP32 is little-endian, the wire is big-endian): the header is
 *           always encoded/decoded field by field (SomeIp_Header.c). Never cast a receive buffer to it.
 *  @note    Members keep wire order; the layout has no padding (16 bytes). */
typedef struct st_someip_header {
    someip_service_id_t service_id;
    someip_method_id_t method_id;
    someip_length_t length; /*!< As on the wire: SOMEIP_HEADER_PART2_SIZE + payload */
    someip_client_id_t client_id;
    someip_session_id_t session_id;
    someip_protocol_version_t protocol_version;
    someip_interface_version_t interface_version;
    someip_message_type_t message_type;
    someip_return_code_t return_code;
} someip_header_t;

/** @brief One SOME/IP message as a zero-copy view: decoded header + payload inside the receive buffer
 *  @warning Valid only while the receive buffer is alive. */
typedef struct st_someip_message {
    someip_header_t header;
    uint8_t const *p_payload;
    uint32_t payload_len; /*!< = header.length - SOMEIP_HEADER_PART2_SIZE */
} someip_message_t;

/** @brief Decoded SOME/IP-TP header */
typedef struct st_someip_tp_header {
    uint32_t offset;       /*!< In bytes, always a multiple of 16 */
    uint8_t more_segments; /*!< 1 = more segments follow, 0 = last segment */
} someip_tp_header_t;

typedef enum e_someip_transport {
    SOMEIP_TRANSPORT_UDP = 0,
    SOMEIP_TRANSPORT_TCP = 1,
} someip_transport_t;

/** @brief Transport endpoint of a SOME/IP peer */
typedef struct st_someip_endpoint {
    someip_transport_t transport;
    uint8_t ipv4[SOMEIP_IPV4_ADDR_SIZE]; /*!< Network order, e.g. {192,168,1,10} */
    uint16_t port;                       /*!< Host order */
} someip_endpoint_t;

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __SOMEIP_TYPES_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
