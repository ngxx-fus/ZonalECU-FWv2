/********************************************************************************************************************************
 * @file Service.h
 * @ingroup ZonalECU_FWv2
 * @brief Header file for [[Service]]. Application service manager wrapping ESP-FreeRTOS task control blocks.
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __FW_ESP_COMP_APP_ZE_SRVS_SERVICE_H__
#define __FW_ESP_COMP_APP_ZE_SRVS_SERVICE_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "App/ESP-FreeRTOS/All.h"
#include "Common/Base/All.h"
#include "Common/Error/All.h"
#include "Comm/ESP-Mem/Mem.h"
#include <stdint.h>

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define ZE_SRV_NAME_MAX_LEN ZE_TASK_NAME_MAX_LEN /*!< Standardized service name capacity */
#define ZE_SRV_COUNT_MAX    (6U)                 /*!< Maximum registered services in the system */

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* Wrap directly from ESP-FreeRTOS standardized task types ***********************************************************************/

typedef ze_task_fn_t           ze_srv_task_fn_t;       /*!< Service task entry function pointer */
typedef ze_task_param_t        ze_srv_param_t;         /*!< Service task parameter pointer */
typedef ze_task_handle_t       ze_srv_handle_t;        /*!< Service runtime task handle */
typedef ze_task_stack_size_t   ze_srv_stack_size_t;    /*!< Service allocated stack size */
typedef ze_task_id_t           ze_srv_id_t;            /*!< Service identifier */
typedef ze_task_name_t         ze_srv_name_t;          /*!< Service name buffer */

typedef ze_task_stat_t         ze_srv_stat_t;          /*!< Service lifecycle status */
#define ZE_SRV_DISABLED        ZE_TASK_DISABLED        /*!< Service is disabled */
#define ZE_SRV_ENABLED         ZE_TASK_ENABLED         /*!< Service is enabled */

typedef ze_task_prio_t         ze_srv_prio_t;          /*!< Service priority */
#define ZE_SRV_PRIO_BACKGROUND ZE_TASK_PRIO_BACKGROUND /*!< Background priority */
#define ZE_SRV_PRIO_LOW        ZE_TASK_PRIO_LOW        /*!< Low priority */
#define ZE_SRV_PRIO_NORMAL     ZE_TASK_PRIO_NORMAL     /*!< Normal priority */
#define ZE_SRV_PRIO_HIGH       ZE_TASK_PRIO_HIGH       /*!< High priority */
#define ZE_SRV_PRIO_URGENCY    ZE_TASK_PRIO_URGENCY    /*!< Critical priority */
#define ZE_SRV_PRIO_REALTIME   ZE_TASK_PRIO_REALTIME   /*!< Realtime priority */

typedef ze_task_core_t         ze_srv_core_t;          /*!< Service core affinity */
#define ZE_SRV_CORE_0          ZE_TASK_CORE_0          /*!< Core 0 affinity */
#define ZE_SRV_CORE_1          ZE_TASK_CORE_1          /*!< Core 1 affinity */
#define ZE_SRV_CORE_ANY        ZE_TASK_CORE_ANY        /*!< Any core affinity */

/** @brief  Service Control Block directly wrapping ze_task_t */
typedef ze_task_t              ze_srv_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

extern ze_srv_t rw_ptr gp_ze_srv_list;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Initialize and spawn all enabled application services
 *  @return  ZE_SUCCESS on success, or specific error code */
ze_err_t ZE_Srv_Init(void);

/** @brief   Get service configuration descriptor by service ID
 *  @param   src_id    Service numeric/enum ID
 *  @param   p_service Pointer to caller's buffer to receive service info
 *  @return  ZE_SUCCESS on success, or ZE_ERR_NOT_FOUND */
ze_err_t ZE_Srv_InfoGet(IN ze_srv_id_t src_id, OUT ze_srv_t rw_ptr p_service);

/** @brief   Update read-write fields of a service descriptor by service ID
 *  @param   src_id    Service numeric/enum ID
 *  @param   p_service Pointer to descriptor holding new rw field values
 *  @return  ZE_SUCCESS on success, or ZE_ERR_NOT_FOUND */
ze_err_t ZE_Srv_InfoSet(IN ze_srv_id_t src_id, IN ze_srv_t ro_ptr p_service);

/** @brief   Get service configuration descriptor by service Name
 *  @param   p_srv_name Name of the service to look up
 *  @param   p_service  Pointer to caller's buffer to receive service info
 *  @return  ZE_SUCCESS on success, or ZE_ERR_NOT_FOUND */
ze_err_t ZE_Srv_InfoGetByName(IN char ro_ptr p_srv_name, OUT ze_srv_t rw_ptr p_service);

/** @brief   Start and spawn a specific service task by ID using ESP-FreeRTOS
 *  @param   src_id Service numeric/enum ID
 *  @return  ZE_SUCCESS on success, or specific error code */
ze_err_t ZE_Srv_Start(IN ze_srv_id_t src_id);

/** @brief   Stop and terminate a running service task by ID using ESP-FreeRTOS
 *  @param   src_id Service numeric/enum ID
 *  @return  ZE_SUCCESS on success, or specific error code */
ze_err_t ZE_Srv_Stop(IN ze_srv_id_t src_id);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __FW_ESP_COMP_APP_ZE_SRVS_SERVICE_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/