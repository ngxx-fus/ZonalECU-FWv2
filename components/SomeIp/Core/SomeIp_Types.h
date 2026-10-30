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
#include "Macros.h"

/* HEADERS **************************************************************************************************
 ************************************************************************************************************/
ZE_HEADER

/* MACROS ***************************************************************************************************
 ************************************************************************************************************/

/* ==========================================================================================================
 * HEADER LAYOUT                                                       [PRS_SOMEIP_00030] [PRS_SOMEIP_00368]
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

/* ==========================================================================================================
 * MESSAGE ID / REQUEST ID                                                                  [PRS_SOMEIP_00034]
 * ======================================================================================================== */
#define SOMEIP_MESSAGE_ID(serviceId, methodId)  ((SomeIp_MessageId_t)(((uint32_t)(serviceId) << 16) | \
                                                                       (uint32_t)(methodId)))
#define SOMEIP_REQUEST_ID(clientId, sessionId)  ((SomeIp_RequestId_t)(((uint32_t)(clientId) << 16) | \
                                                                       (uint32_t)(sessionId)))

/* Method ID bit 15: 0 = method (0x0000-0x7FFF), 1 = event/notification (0x8000-0xFFFF) */
#define SOMEIP_EVENT_ID_FLAG                    (0x8000u)
#define SOMEIP_METHOD_ID_MAX                    (0x7FFFu)

/* Session ID                                  [PRS_SOMEIP_00932] [PRS_SOMEIP_00933] [PRS_SOMEIP_00521] */
#define SOMEIP_SESSION_ID_INACTIVE              (0x0000u)   /* session handling not active             */
#define SOMEIP_SESSION_ID_MIN                   (0x0001u)   /* first value, and value after wrap       */
#define SOMEIP_SESSION_ID_MAX                   (0xFFFFu)

/* ==========================================================================================================
 * PROTOCOL VERSION                                                                         [PRS_SOMEIP_00051]
 * ======================================================================================================== */
#define SOMEIP_PROTOCOL_VERSION                 (0x01u)

/* ==========================================================================================================
 * MESSAGE TYPE                                                 Table 4.4, [PRS_SOMEIP_00055] [PRS_SOMEIP_00367]
 * ======================================================================================================== */
#define SOMEIP_MT_REQUEST                       (0x00u)     /* A request expecting a response (even void)       */
#define SOMEIP_MT_REQUEST_NO_RETURN             (0x01u)     /* A fire & forget request                          */
#define SOMEIP_MT_NOTIFICATION                  (0x02u)     /* event / field notification                       */
#define SOMEIP_MT_RESPONSE                      (0x80u)     /* The response message                             */
#define SOMEIP_MT_ERROR                         (0x81u)     /* The response containing an error                 */
#define SOMEIP_MT_TP_REQUEST                    (0x20u)     /* A TP request expecting a response (even void)    */
#define SOMEIP_MT_TP_REQUEST_NO_RETURN          (0x21u)     /* A TP fire&forget request                         */
#define SOMEIP_MT_TP_NOTIFICATION               (0x22u)     /* A TP request of a notification/event call back expecting no response */
#define SOMEIP_MT_TP_RESPONSE                   (0xA0u)     /* The TP response message                          */
#define SOMEIP_MT_TP_ERROR                      (0xA1u)     /* The TP response containing an error              */

#define SOMEIP_MT_FLAG_RESPONSE                 (0x80u)
#define SOMEIP_MT_FLAG_TP                       (0x20u)     /* "TP-Flag"                                  */

/* ==========================================================================================================
 * RETURN CODE                                                                  Table 4.11, [PRS_SOMEIP_00191]
 *
 *    REQUEST / REQUEST_NO_RETURN / NOTIFICATION : always E_OK                      Table 4.5
 *    RESPONSE : any code (normally E_OK)        ERROR : any code except E_OK
 * ======================================================================================================== */
#define SOMEIP_E_OK                             (0x00u)     /* no error occurred                          */
#define SOMEIP_E_NOT_OK                         (0x01u)     /* unspecified error                          */
#define SOMEIP_E_UNKNOWN_SERVICE                (0x02u)
#define SOMEIP_E_UNKNOWN_METHOD                 (0x03u)
#define SOMEIP_E_NOT_READY                      (0x04u)     /* service/method known, application not up   */
#define SOMEIP_E_NOT_REACHABLE                  (0x05u)     /* internal only - never sent                 */
#define SOMEIP_E_TIMEOUT                        (0x06u)     /* internal only - never sent                 */
#define SOMEIP_E_WRONG_PROTOCOL_VERSION         (0x07u)
#define SOMEIP_E_WRONG_INTERFACE_VERSION        (0x08u)
#define SOMEIP_E_MALFORMED_MESSAGE              (0x09u)     /* payload cannot be deserialized             */
#define SOMEIP_E_WRONG_MESSAGE_TYPE             (0x0Au)
#define SOMEIP_E_E2E_REPEATED                   (0x0Bu)
#define SOMEIP_E_E2E_WRONG_SEQUENCE             (0x0Cu)
#define SOMEIP_E_E2E                            (0x0Du)     /* not further specified E2E error            */
#define SOMEIP_E_E2E_NOT_AVAILABLE              (0x0Eu)
#define SOMEIP_E_E2E_NO_NEW_DATA                (0x0Fu)

#define SOMEIP_E_GENERIC_RESERVED_MIN           (0x10u)     /* 0x10..0x1F reserved for generic errors     */
#define SOMEIP_E_GENERIC_RESERVED_MAX           (0x1Fu)
#define SOMEIP_E_SERVICE_SPECIFIC_MIN           (0x20u)     /* 0x20..0x5E defined by the interface spec   */
#define SOMEIP_E_SERVICE_SPECIFIC_MAX           (0x5Eu)

/* ==========================================================================================================
 * PAYLOAD / TRANSPORT LIMITS                                                            chapter 4.1.2.9
 * ======================================================================================================== */
#define SOMEIP_UDP_PAYLOAD_MAX                  (1400u)     /* SOME/IP payload per message over UDP       */
#define SOMEIP_UDP_MESSAGE_MAX                  (SOMEIP_HEADER_SIZE + SOMEIP_UDP_PAYLOAD_MAX)

/* ==========================================================================================================
 * SOME/IP-TP                                [PRS_SOMEIP_00723] [PRS_SOMEIP_00724] [PRS_SOMEIP_00727]
 *                                              [PRS_SOMEIP_00730] [PRS_SOMEIP_00931]
 *
 *    TP header (4 bytes, big-endian), placed right after the SOME/IP header:
 *    | Offset [31:4] (unit = 16 bytes) | Reserved [3:1] | More Segments [0] |
 * ======================================================================================================== */
#define SOMEIP_TP_HEADER_SIZE                   (4u)
#define SOMEIP_TP_OFFSET_MASK                   (0xFFFFFFF0u)
#define SOMEIP_TP_RESERVED_MASK                 (0x0000000Eu) /* sender: 0, receiver: ignore [00726]     */
#define SOMEIP_TP_MORE_SEGMENTS_MASK            (0x00000001u)
#define SOMEIP_TP_OFFSET_ALIGNMENT              (16u)

/* Maximum segment payload length specified by AUTOSAR */
#define SOMEIP_TP_SEGMENT_MAX                   (1392u)

/* ==========================================================================================================
 * SERIALIZATION                                                                         chapter 4.1.4
 * ======================================================================================================== */

/* Boolean: 8 bit, only bit 0 is evaluated                                                  [PRS_SOMEIP_00615] */
#define SOMEIP_BOOL_FALSE                       (0x00u)
#define SOMEIP_BOOL_TRUE                        (0x01u)
#define SOMEIP_BOOL_VALUE_MASK                  (0x01u)

/* String Byte Order Marks (always present, fixed and dynamic strings)  [PRS_SOMEIP_00087] [PRS_SOMEIP_00372] */
#define SOMEIP_BOM_UTF8_SIZE                    (3u)
#define SOMEIP_BOM_UTF8_0                       (0xEFu)
#define SOMEIP_BOM_UTF8_1                       (0xBBu)
#define SOMEIP_BOM_UTF8_2                       (0xBFu)
#define SOMEIP_BOM_UTF16_SIZE                   (2u)
#define SOMEIP_BOM_UTF16BE_0                    (0xFEu)
#define SOMEIP_BOM_UTF16BE_1                    (0xFFu)
#define SOMEIP_BOM_UTF16LE_0                    (0xFFu)
#define SOMEIP_BOM_UTF16LE_1                    (0xFEu)

/* Union: type field value 0 is reserved for NULL (empty union)                             [PRS_SOMEIP_00907] */
#define SOMEIP_UNION_TYPE_NULL                  (0u)

/* TYPEDEFS *************************************************************************************************
 ************************************************************************************************************/

/* ---- Identifiers ---------------------------------------------------------------------------------------- */
typedef uint16_t SomeIp_ServiceId_t;
typedef uint16_t SomeIp_MethodId_t;         /* method (bit15 = 0) or event (bit15 = 1)                    */
typedef uint16_t SomeIp_EventId_t;          /* a MethodId with bit15 = 1                                  */
typedef uint16_t SomeIp_EventgroupId_t;
typedef uint16_t SomeIp_ClientId_t;
typedef uint16_t SomeIp_SessionId_t;
typedef uint32_t SomeIp_MessageId_t;        /* ServiceId << 16 | MethodId                                 */
typedef uint32_t SomeIp_RequestId_t;        /* ClientId  << 16 | SessionId                                */
typedef uint32_t SomeIp_Length_t;

/* ---- Single-byte header fields (values: see MACROS) ----------------------------------------------------- */
typedef uint8_t  SomeIp_ProtocolVersion_t;
typedef uint8_t  SomeIp_InterfaceVersion_t; /* major version of the service interface   [PRS_SOMEIP_00053] */
typedef uint8_t  SomeIp_MessageType_t;      /* SOMEIP_MT_*                                                */
typedef uint8_t  SomeIp_ReturnCode_t;       /* SOMEIP_E_*                                                 */

/* Byte order of payload parameters is configurable per parameter              [PRS_SOMEIP_00369] */
typedef enum
{
    SOMEIP_BYTE_ORDER_BIG_ENDIAN = 0,       /* default, same as the header                                */
    SOMEIP_BYTE_ORDER_LITTLE_ENDIAN,
} SomeIp_ByteOrder_t;

/* Size of a length field (dynamic array/string, struct, union). 0 = no length field  [PRS_SOMEIP_00121] */
typedef enum
{
    SOMEIP_LENGTH_FIELD_NONE  = 0,
    SOMEIP_LENGTH_FIELD_8BIT  = 1,
    SOMEIP_LENGTH_FIELD_16BIT = 2,
    SOMEIP_LENGTH_FIELD_32BIT = 4,          /* default for dynamic arrays and strings                     */
} SomeIp_LengthFieldSize_t;                 /* enumerator value = size in bytes                           */

/* String encoding                                                                    [PRS_SOMEIP_00372] */
typedef enum
{
    SOMEIP_STRING_UTF8 = 0,
    SOMEIP_STRING_UTF16BE,
    SOMEIP_STRING_UTF16LE,
} SomeIp_StringEncoding_t;

/* ---- Internal API result (NOT a wire value - see SomeIp_ReturnCode_t) ----------------------------------- */
typedef enum
{
    SOMEIP_RES_OK = 0,
    SOMEIP_RES_E_PARAM,                     /* NULL pointer / invalid argument                            */
    SOMEIP_RES_E_SHORT_INPUT,               /* fewer input bytes than required                            */
    SOMEIP_RES_E_NO_SPACE,                  /* output buffer too small                                    */
    SOMEIP_RES_E_MALFORMED,                 /* Length / structure inconsistent                            */
    SOMEIP_RES_E_NOT_FOUND,                 /* unknown service / method / instance in configuration       */
    SOMEIP_RES_E_TRANSPORT,                 /* socket / driver error                                      */
    SOMEIP_RES_E_TIMEOUT,
} SomeIp_Result_t;

/* ---- Header & message ----------------------------------------------------------------------------------- */

/*
 * Decoded SOME/IP header, fields in HOST byte order.
 * Not a packed image of the wire: ESP32 is little-endian, the wire is big-endian, so the header is always
 * encoded/decoded field by field (SomeIp_Header.c). Never cast a receive buffer to this struct.
 */
typedef struct
{
    SomeIp_ServiceId_t        serviceId;
    SomeIp_MethodId_t         methodId;
    SomeIp_Length_t           length;            /* as on the wire: SOMEIP_HEADER_PART2_SIZE + payload    */
    SomeIp_ClientId_t         clientId;
    SomeIp_SessionId_t        sessionId;
    SomeIp_ProtocolVersion_t  protocolVersion;
    SomeIp_InterfaceVersion_t interfaceVersion;
    SomeIp_MessageType_t      messageType;
    SomeIp_ReturnCode_t       returnCode;
} SomeIp_Header_t;

/*
 * One SOME/IP message as a zero-copy view: decoded header + payload pointer into the receive buffer.
 * Valid only while that buffer is alive.
 */
typedef struct
{
    SomeIp_Header_t header;
    const uint8_t  *payload;
    uint32_t        payloadLength;               /* = header.length - SOMEIP_HEADER_PART2_SIZE            */
} SomeIp_Message_t;

/* Decoded SOME/IP-TP header */
typedef struct
{
    uint32_t offset;                             /* in bytes, always a multiple of 16                     */
    uint8_t  moreSegments;                       /* 1 = more segments follow, 0 = last segment            */
} SomeIp_TpHeader_t;

/* ---- Transport endpoint --------------------------------------------------------------------------------- */
typedef enum
{
    SOMEIP_TRANSPORT_UDP = 0,
    SOMEIP_TRANSPORT_TCP,
} SomeIp_Transport_t;

typedef struct
{
    uint8_t            ipv4[4];                  /* network order: {192,168,1,10}                         */
    uint16_t           port;                     /* host order                                            */
    SomeIp_Transport_t transport;
} SomeIp_Endpoint_t;

/* FOOTERS **************************************************************************************************
 ************************************************************************************************************/
ZE_FOOTER

/* GUARD/END ************************************************************************************************
 ************************************************************************************************************/
#endif /* __SOMEIP_TYPES_H__ */

/* EOF ******************************************************************************************************
 ************************************************************************************************************/
