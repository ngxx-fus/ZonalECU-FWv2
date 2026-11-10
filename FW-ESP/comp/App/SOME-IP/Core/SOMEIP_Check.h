

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __SOMEIP_CHECK_H__
#define __SOMEIP_CHECK_H__

/** @brief SOME/IP message validation and error message: public API (chapter 4.2.6, Figure 4.21)
 *  @details - Check a received header in the order of Figure 4.21 [PRS_SOMEIP_00195] [PRS_SOMEIP_00576]
 *           - Decide whether an ERROR message may be sent back [00188] [00189] [00537] [00539]
 *           - Build the header of the ERROR message [PRS_SOMEIP_00190] */

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdint.h>
#include "Macros.h"
#include "SOMEIP_Types.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

HEADER_CPP

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Configuration of one method / event of a service
 *  @note  expected_type: SOMEIP_MT_REQUEST = request/response, SOMEIP_MT_REQUEST_NO_RETURN = fire & forget,
 *         SOMEIP_MT_NOTIFICATION = event / field notifier. */
typedef struct st_someip_method_cfg {
    someip_method_id_t method_id; /*!< Method (bit 15 = 0) or event (bit 15 = 1) */
    someip_message_type_t expected_type;
} someip_method_cfg_t;

/** @brief Configuration of one service */
typedef struct st_someip_service_cfg {
    someip_method_cfg_t const *p_methods;
    uint16_t method_len; /*!< Number of entries in p_methods */
    someip_service_id_t service_id;
    someip_interface_version_t interface_version; /*!< Major interface version [PRS_SOMEIP_00053] */
} someip_service_cfg_t;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Check a received header against the configuration (steps 1..6 of Figure 4.21)
 *  @details The checks are done in this exact order, the first failing check gives the Return Code:
 *           @verbatim
 *           #  check                                  failed -> *p_rc
 *           -  -------------------------------------  --------------------------------
 *           1  Protocol Version == 0x01               SOMEIP_E_WRONG_PROTOCOL_VERSION
 *           2  Service ID known                       SOMEIP_E_UNKNOWN_SERVICE
 *           3  Interface Version == configured        SOMEIP_E_WRONG_INTERFACE_VERSION
 *           4  Method ID known                        SOMEIP_E_UNKNOWN_METHOD
 *           5  Message Type supported (Table 4.4)     SOMEIP_E_WRONG_MESSAGE_TYPE
 *           6  Message Type as configured             SOMEIP_E_WRONG_MESSAGE_TYPE
 *           @endverbatim
 *           All checks pass: *p_rc = SOMEIP_E_OK [PRS_SOMEIP_00195] [PRS_SOMEIP_00576].
 *  @warning SOMEIP_RES_OK does NOT mean the message is valid: always look at *p_rc.
 *  @param p_rx         Received header
 *  @param p_services   Configuration table, may be NULL if service_len is 0
 *  @param service_len  Number of entries in p_services
 *  @param p_rc         SOMEIP_E_OK or the Return Code of the first failing check
 *  @param pp_method    Configured method if step 4 passed, otherwise NULL; needed by
 *                      SOMEIP_CheckShouldReplyError() [PRS_SOMEIP_00537]
 *  @retval SOMEIP_RES_OK      *p_rc and *pp_method are valid
 *  @retval SOMEIP_RES_E_PARAM p_rx, p_rc or pp_method is NULL, or p_services is NULL with service_len > 0 */
someip_result_t SOMEIP_CheckHeader(IN someip_header_t *p_rx, IN someip_service_cfg_t *p_services, IN uint16_t service_len,
                                   OUT someip_return_code_t *p_rc, OUT someip_method_cfg_t const **pp_method);

/** @brief Decide whether an ERROR message may be sent back for a message that failed a check
 *  @details Only a REQUEST (0x00, or 0x20 with TP-Flag) is answered with an ERROR message:
 *           @verbatim
 *           received                                       reply ERROR?   spec
 *           ---------------------------------------------  -------------  ------------------
 *           NOTIFICATION                                   no             [PRS_SOMEIP_00188]
 *           REQUEST_NO_RETURN                              no             [PRS_SOMEIP_00189]
 *           RESPONSE / ERROR                               no             [PRS_SOMEIP_00539]
 *           REQUEST, but method configured as event or     no             [PRS_SOMEIP_00537]
 *             fire & forget (expected_type != REQUEST)
 *           REQUEST, method unknown (p_method == NULL)     yes            Figure 4.21
 *           REQUEST, method configured as REQUEST          yes            Figure 4.21
 *           @endverbatim
 *  @param p_rx     Received header
 *  @param p_method Configured method from SOMEIP_CheckHeader(), NULL if not found
 *  @retval 1 Send an ERROR message
 *  @retval 0 Drop silently, or p_rx is NULL */
uint8_t SOMEIP_CheckShouldReplyError(IN someip_header_t *p_rx, IN someip_method_cfg_t *p_method);

/** @brief Build the header of the ERROR message that answers a received request
 *  @details Copied from p_rx [PRS_SOMEIP_00190]: Message ID, Request ID [00704], Interface Version.
 *           Set:
 *           - Protocol Version = p_rx->protocol_version, except for SOMEIP_E_WRONG_PROTOCOL_VERSION where it
 *             is SOMEIP_PROTOCOL_VERSION (0x01) (Figure 4.21)
 *           - Message Type = SOMEIP_MT_ERROR (0x81), TP-Flag never set
 *           - Return Code = rc
 *           - Length = SOMEIP_HEADER_PART2_SIZE (8): the payload is NOT copied [PRS_SOMEIP_00190]
 *  @note    p_rx and p_err may point to the same header.
 *  @param p_rx  Header of the received request
 *  @param rc    Return Code to send, must not be SOMEIP_E_OK
 *  @param p_err ERROR header, ready for SOMEIP_HeaderEncode()
 *  @retval SOMEIP_RES_OK      *p_err is valid
 *  @retval SOMEIP_RES_E_PARAM p_rx or p_err is NULL, or rc is SOMEIP_E_OK */
someip_result_t SOMEIP_BuildErrorHeader(IN someip_header_t *p_rx, IN someip_return_code_t rc, OUT someip_header_t *p_err);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

FOOTER_CPP

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __SOMEIP_CHECK_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/
