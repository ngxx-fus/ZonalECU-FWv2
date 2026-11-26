/********************************************************************************************************************************
 * @file SOMP-IP.c
 * @ingroup ZonalECU_FWv2
 * @brief Source file for [[SOME-IP Service]]. Implements SOME/IP loopback demo thread.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "Common/All.h"
#include "Comm/ESP-Logger/SysLog.h"
#include "App/SOME-IP/SOMEIP.h"
#include "App/SOME-IP/Core/SOMEIP_Header.h"
#include "App/SOME-IP/Core/SOMEIP_Types.h"
#include "App/SOME-IP/Event/SOMEIP_Event.h"
#include "App/SOME-IP/Message/SOMEIP_Message.h"
#include "App/SOME-IP/Serialization/SOMEIP_Serialization.h"
#include "App/SOME-IP/Server/SOMEIP_Server.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define MAIN_LED_GPIO          (GPIO_NUM_2)
#define MAIN_LOOP_MS           (500U)
#define MAIN_STEP_LOOPS        (6U)      /* One demo step every 6 loops = 3 s */
#define MAIN_FRAME_SIZE        (64U)
#define MAIN_HEX_CHARS         (3U)      /* "xx " per printed byte */
#define MAIN_HEX_LINE          (193U)    /* MAIN_FRAME_SIZE bytes of MAIN_HEX_CHARS + terminator */

#define DEMO_SERVICE_ID        (0x1001U)
#define DEMO_INTERFACE_VERSION (1U)
#define DEMO_METHOD_SET        (0x0001U)
#define DEMO_METHOD_GET        (0x0002U)
#define DEMO_METHOD_UNKNOWN    (0x0009U) /* Not configured, to show an ERROR message */
#define DEMO_EVENT_BRIGHTNESS  (0x8001U)
#define DEMO_EVENT_DISTANCE    (0x8002U)
#define DEMO_DISTANCE_CYCLE_MS (2000U)
#define DEMO_CLIENT_ID         (0x0042U) /* Client ID of the simulated CCU */
#define DEMO_BRIGHTNESS_MAX    (100U)
#define DEMO_E_OUT_OF_RANGE    (0x20U)   /* Service specific Return Code (0x20..0x5E) */

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  State of the Light service, passed as p_ctx to every callback */
typedef struct st_demo_app {
    uint16_t distance_mm;
    uint8_t  brightness;
    bool     is_brightness_changed; /* Set by SetBrightness, the main loop then sends the notification */
} demo_app_t;

/* PRIVATE FUNCTION PROTOTYPES ***************************************************************************************************
 ********************************************************************************************************************************/

static void            prv_print_hex(IN uint8_t *p_msg, IN uint32_t msg_len);
static someip_result_t prv_demo_send(IN someip_endpoint_t *p_peer, IN uint8_t *p_msg, IN uint32_t msg_len, INOUT void *p_ctx);
static someip_return_code_t prv_on_set_brightness(IN someip_message_t *p_request, INOUT someip_ser_cursor_t *p_response,
                                                  INOUT void *p_ctx);
static someip_return_code_t prv_on_get_brightness(IN someip_message_t *p_request, INOUT someip_ser_cursor_t *p_response,
                                                  INOUT void *p_ctx);
static someip_result_t prv_ser_brightness(INOUT someip_ser_cursor_t *p_payload, INOUT void *p_ctx);
static someip_result_t prv_ser_distance  (INOUT someip_ser_cursor_t *p_payload, INOUT void *p_ctx);
static void            prv_client_call   (IN someip_method_id_t method_id, IN uint8_t *p_args, IN uint32_t args_len);
static void            prv_run_step      (IN uint32_t step);
static someip_result_t prv_someip_init   (void);

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Methods and events of the Light service, used by SOMEIP_CheckHeader() */
static const someip_method_cfg_t s_light_methods[] = {
    {.method_id = DEMO_METHOD_SET, .expected_type = SOMEIP_MT_REQUEST},
    {.method_id = DEMO_METHOD_GET, .expected_type = SOMEIP_MT_REQUEST},
};

static const someip_service_cfg_t s_services[] = {
    {
        .p_methods         = s_light_methods,
        .method_len        = (uint16_t)(sizeof(s_light_methods) / sizeof(s_light_methods[0])),
        .service_id        = DEMO_SERVICE_ID,
        .interface_version = DEMO_INTERFACE_VERSION,
    },
};

/** @brief  Application function of every method */
static const someip_handler_cfg_t s_handlers[] = {
    {.handler = prv_on_set_brightness, .service_id = DEMO_SERVICE_ID, .method_id = DEMO_METHOD_SET, .use_error_message = 0U},
    {.handler = prv_on_get_brightness, .service_id = DEMO_SERVICE_ID, .method_id = DEMO_METHOD_GET, .use_error_message = 0U},
};

/** @brief  Events of the Light service */
static const someip_event_cfg_t s_events[] = {
    {
        .serialize         = prv_ser_brightness,
        .cycle_ms          = 0U,
        .service_id        = DEMO_SERVICE_ID,
        .event_id          = DEMO_EVENT_BRIGHTNESS,
        .interface_version = DEMO_INTERFACE_VERSION,
        .is_field          = 1U,
        .session_active    = 1U,
    },
    {
        .serialize         = prv_ser_distance,
        .cycle_ms          = DEMO_DISTANCE_CYCLE_MS,
        .service_id        = DEMO_SERVICE_ID,
        .event_id          = DEMO_EVENT_DISTANCE,
        .interface_version = DEMO_INTERFACE_VERSION,
        .is_field          = 0U,
        .session_active    = 1U,
    },
};

#define DEMO_EVENT_LEN (sizeof(s_events) / sizeof(s_events[0]))

static someip_event_state_t s_event_states[DEMO_EVENT_LEN];
static someip_subscriber_t s_subscribers[4];

/** @brief  Simulated CCU: static subscriber until SOME/IP-SD exists */
static const someip_endpoint_t s_ccu = {
    .transport = SOMEIP_TRANSPORT_UDP,
    .ipv4      = {192U, 168U, 1U, 10U},
    .port      = 30501U,
};

static demo_app_t s_app = {
    .distance_mm           = 500U,
    .brightness            = 0U,
    .is_brightness_changed = false,
};

static uint8_t s_server_tx[MAIN_FRAME_SIZE];
static uint8_t s_event_tx[MAIN_FRAME_SIZE];
static uint8_t s_client_frame[MAIN_FRAME_SIZE];

static someip_server_t s_server;
static someip_publisher_t s_publisher;
static someip_session_id_t s_client_session = 0U;

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Print a message as hex bytes on one line */
static void prv_print_hex(IN uint8_t *p_msg, IN uint32_t msg_len) {
    char line[MAIN_HEX_LINE];
    uint32_t idx;
    uint32_t pos = 0U;

    line[0] = '\0';

    for (idx = 0U; (idx < msg_len) && (idx < MAIN_FRAME_SIZE); idx++) {
        (void)snprintf(&line[pos], sizeof(line) - pos, "%02X ", p_msg[idx]);
        pos += MAIN_HEX_CHARS;
    }

   ZE_LOGI("%s", line);
}

/** @brief  Transport of the demo: print the message instead of sending it */
static someip_result_t prv_demo_send(IN someip_endpoint_t *p_peer, IN uint8_t *p_msg, IN uint32_t msg_len, INOUT void *p_ctx) {
    someip_header_t hdr;
    someip_result_t res;

    (void)p_ctx;
    res = SOMEIP_HeaderDecode(p_msg, msg_len, &hdr);

    if (SOMEIP_RES_OK != res) {
       ZE_LOGE("TX: header decode failed (%d)", res);
        return SOMEIP_RES_E_TRANSPORT;
    }

    ZE_LOGI("TX -> %u.%u.%u.%u:%u  srv=0x%04X id=0x%04X client=0x%04X session=0x%04X type=0x%02X rc=0x%02X", p_peer->ipv4[0],
            p_peer->ipv4[1], p_peer->ipv4[2], p_peer->ipv4[3], p_peer->port, hdr.service_id, hdr.method_id, hdr.client_id,
            hdr.session_id, hdr.message_type, hdr.return_code);
   prv_print_hex(p_msg, msg_len);

    return SOMEIP_RES_OK;
}

/** @brief  Method SetBrightness: in uint8 0..100, out uint8 applied value */
static someip_return_code_t prv_on_set_brightness(IN someip_message_t *p_request, INOUT someip_ser_cursor_t *p_response,
                                                  INOUT void *p_ctx) {
    demo_app_t *p_app = (demo_app_t *)p_ctx;
    someip_deser_cursor_t reader;
    uint8_t level = 0U;

    (void)SOMEIP_DeserInit(&reader, p_request->p_payload, p_request->payload_len, (uint32_t)SOMEIP_OFS_PAYLOAD,
                           SOMEIP_BYTE_ORDER_BIG_ENDIAN);

    if (SOMEIP_RES_OK != SOMEIP_DeserReadU8(&reader, &level)) {
        return SOMEIP_E_MALFORMED_MESSAGE;
    }

    if (DEMO_BRIGHTNESS_MAX < level) {
       ZE_LOGW("SetBrightness(%u) rejected, out of range", level);
        (void)SOMEIP_SerWriteU8(p_response, p_app->brightness);
        return DEMO_E_OUT_OF_RANGE;
    }

    if (level != p_app->brightness) {
        p_app->brightness = level;
        p_app->is_brightness_changed = true;
    }

   ZE_LOGI("SetBrightness(%u) applied", level);
    (void)SOMEIP_SerWriteU8(p_response, p_app->brightness);

    return SOMEIP_E_OK;
}

/** @brief  Method GetBrightness (getter of the field): out uint8 */
static someip_return_code_t prv_on_get_brightness(IN someip_message_t *p_request, INOUT someip_ser_cursor_t *p_response,
                                                  INOUT void *p_ctx) {
    (void)p_request;
   ZE_LOGI("GetBrightness() -> %u", ((demo_app_t *)p_ctx)->brightness);

    return (SOMEIP_RES_OK == prv_ser_brightness(p_response, p_ctx)) ? SOMEIP_E_OK : SOMEIP_E_NOT_OK;
}

/** @brief  Payload of the Brightness field: uint8 */
static someip_result_t prv_ser_brightness(INOUT someip_ser_cursor_t *p_payload, INOUT void *p_ctx) {
    return SOMEIP_SerWriteU8(p_payload, ((demo_app_t *)p_ctx)->brightness);
}

/** @brief  Payload of the Distance event: uint16 in mm */
static someip_result_t prv_ser_distance(INOUT someip_ser_cursor_t *p_payload, INOUT void *p_ctx) {
    return SOMEIP_SerWriteU16(p_payload, ((demo_app_t *)p_ctx)->distance_mm);
}

/** @brief  Simulated CCU: build one REQUEST and give it to the server as a received datagram */
static void prv_client_call(IN someip_method_id_t method_id, IN uint8_t *p_args, IN uint32_t args_len) {
    someip_ser_cursor_t writer;
    someip_header_t hdr;
    someip_result_t res;

    s_client_session = (0xFFFFU == s_client_session) ? 1U : (someip_session_id_t)(s_client_session + 1U);

    (void)SOMEIP_SerInit(&writer, &s_client_frame[SOMEIP_HEADER_SIZE], MAIN_FRAME_SIZE - SOMEIP_HEADER_SIZE,
                         (uint32_t)SOMEIP_OFS_PAYLOAD, SOMEIP_BYTE_ORDER_BIG_ENDIAN);
    (void)SOMEIP_SerWriteBytes(&writer, p_args, args_len);

    res = SOMEIP_MessageBuildRequest(&hdr, DEMO_SERVICE_ID, method_id, DEMO_CLIENT_ID, s_client_session, DEMO_INTERFACE_VERSION,
                                    SOMEIP_SerGetLength(&writer));

    if (SOMEIP_RES_OK != res) {
       ZE_LOGE("Build request failed (%d)", res);
        return;
    }

    (void)SOMEIP_HeaderEncode(&hdr, s_client_frame, MAIN_FRAME_SIZE);
   ZE_LOGI("RX <- CCU  srv=0x%04X id=0x%04X session=0x%04X (%lu payload bytes)", hdr.service_id, hdr.method_id, hdr.session_id,
            (unsigned long)SOMEIP_SerGetLength(&writer));

    res = SOMEIP_ServerProcess(&s_server, &s_ccu, s_client_frame, (uint32_t)SOMEIP_HEADER_SIZE + SOMEIP_SerGetLength(&writer));

    if (SOMEIP_RES_OK != res) {
       ZE_LOGE("ServerProcess failed (%d)", res);
    }
}

/** @brief  One step of the demo scenario */
static void prv_run_step(IN uint32_t step) {
    uint8_t level;

    switch (step % 5U) {
        case 0U:
           ZE_LOGI        ("===== Step 1: GetBrightness -> RESPONSE 0x80, rc E_OK =====");
           prv_client_call(DEMO_METHOD_GET, NULL, 0U);
            break;

        case 1U:
           ZE_LOGI("===== Step 2: SetBrightness(75) -> RESPONSE + NOTIFICATION 0x8001 =====");
            level = 75U;
           prv_client_call(DEMO_METHOD_SET, &level, 1U);
            break;

        case 2U:
           ZE_LOGI("===== Step 3: SetBrightness(150) -> RESPONSE, rc 0x20 (out of range) =====");
            level = 150U;
           prv_client_call(DEMO_METHOD_SET, &level, 1U);
            break;

        case 3U:
           ZE_LOGI        ("===== Step 4: unknown method 0x0009 -> ERROR 0x81, rc E_UNKNOWN_METHOD =====");
           prv_client_call(DEMO_METHOD_UNKNOWN, NULL, 0U);
            break;

        default:
           ZE_LOGI("===== Step 5: SetBrightness(0) -> RESPONSE + NOTIFICATION 0x8001 =====");
            level = 0U;
           prv_client_call(DEMO_METHOD_SET, &level, 1U);
            break;
    }
}

/** @brief  Initialize the server and the publisher, then subscribe the CCU to both events */
static someip_result_t prv_someip_init(void) {
    someip_server_cfg_t server_cfg = {
        .p_services  = s_services,
        .p_handlers  = s_handlers,
        .p_tx_buf    = s_server_tx,
        .send        = prv_demo_send,
        .p_ctx       = &s_app,
        .tx_buf_size = MAIN_FRAME_SIZE,
        .service_len = (uint16_t)(sizeof(s_services) / sizeof(s_services[0])),
        .handler_len = (uint16_t)(sizeof(s_handlers) / sizeof(s_handlers[0])),
    };
    someip_publisher_cfg_t pub_cfg = {
        .p_events    = s_events,
        .p_states    = s_event_states,
        .p_subs      = s_subscribers,
        .p_tx_buf    = s_event_tx,
        .send        = prv_demo_send,
        .p_ctx       = &s_app,
        .tx_buf_size = MAIN_FRAME_SIZE,
        .event_len   = (uint16_t)DEMO_EVENT_LEN,
        .sub_len     = (uint16_t)(sizeof(s_subscribers) / sizeof(s_subscribers[0])),
    };
    someip_result_t res;

    res = SOMEIP_ServerInit(&s_server, &server_cfg);
    if (SOMEIP_RES_OK != res) {
        return res;
    }

    res = SOMEIP_PublisherInit(&s_publisher, &pub_cfg);
    if (SOMEIP_RES_OK != res) {
        return res;
    }

   ZE_LOGI("===== Subscribe CCU to field 0x8001 -> initial NOTIFICATION =====");
    res = SOMEIP_EventSubscribe(&s_publisher, DEMO_SERVICE_ID, DEMO_EVENT_BRIGHTNESS, &s_ccu);
    if (SOMEIP_RES_OK != res) {
        return res;
    }

   ZE_LOGI("===== Subscribe CCU to event 0x8002 (cyclic 2000 ms) =====");
    return SOMEIP_EventSubscribe(&s_publisher, DEMO_SERVICE_ID, DEMO_EVENT_DISTANCE, &s_ccu);
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   SOME/IP Demo Service FreeRTOS thread function
 *  @param   pvParameters Unused task parameters */
void ZE_Thread_SOMEIP(void *p_pv_parameters /* RENAMED: pvParameters -> p_pv_parameters | FILE: SOMP-IP.c */) {
    (void)p_pv_parameters /* RENAMED: pvParameters -> p_pv_parameters | FILE: SOMP-IP.c */;
    someip_result_t res;
    uint32_t loop_cnt = 0U;
    uint32_t now_ms;

   ZE_LOGI("SOME/IP loopback service started");

    res = prv_someip_init();
    if (SOMEIP_RES_OK != res) {
       ZE_LOGE    ("SOME/IP init failed (%d)", res);
       vTaskDelete(NULL);
        return;
    }

    while (true) {
        now_ms = (uint32_t)(esp_timer_get_time() / 1000);

        /* Distance changes a little every loop */
        s_app.distance_mm = (uint16_t)(500U + (10U * (loop_cnt % 20U)));

        /* Time for the next step of the scenario */
        if (0U == (loop_cnt % MAIN_STEP_LOOPS)) {
           prv_run_step(loop_cnt / MAIN_STEP_LOOPS);
        }

        /* Update on change: SetBrightness changed the field value */
        if (s_app.is_brightness_changed) {
            s_app.is_brightness_changed = false;
            (void)SOMEIP_EventNotify(&s_publisher, DEMO_SERVICE_ID, DEMO_EVENT_BRIGHTNESS);
        }

        /* Cyclic update of the Distance event */
        (void)SOMEIP_EventMainFunction(&s_publisher, now_ms);

        loop_cnt++;
       vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_MS));
    }
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/