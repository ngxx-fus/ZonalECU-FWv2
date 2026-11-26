/********************************************************************************************************************************
 * @file Service.c
 * @ingroup ZonalECU_FWv2
 * @brief Source file for [[Service]]. Service table implementation wrapping ESP-FreeRTOS task control.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "Service.h"
#include <stdio.h>
#include <string.h>
#include "Comm/ESP-Logger/SysLog.h"
#include "Comm/ESP-Mem/Mem.h"
#include "SOME-IP/SOMP-IP.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Registered application service table */
static ze_srv_t s_srv_table[ZE_SRV_COUNT_MAX] = {
    [0] ={.p_srv = NULL,.p_param = NULL,.p_handle = NULL,.stack_size = 4096U,.priority = ZE_SRV_PRIO_HIGH,.core_id = ZE_SRV_CORE_1,.stat = ZE_SRV_DISABLED,.name = "Motor",},
    [1] ={.p_srv = NULL,.p_param = NULL,.p_handle = NULL,.stack_size = 4096U,.priority = ZE_SRV_PRIO_HIGH,.core_id = ZE_SRV_CORE_0,.stat = ZE_SRV_DISABLED,.name = "Ethernet",},
    [2] ={.p_srv = NULL,.p_param = NULL,.p_handle = NULL,.stack_size = 2048U,.priority = ZE_SRV_PRIO_NORMAL,.core_id = ZE_SRV_CORE_1,.stat = ZE_SRV_DISABLED,.name = "Ultrasonic",},
    [3] ={.p_srv = NULL,.p_param = NULL,.p_handle = NULL,.stack_size = 2048U,.priority = ZE_SRV_PRIO_LOW,.core_id = ZE_SRV_CORE_1,.stat = ZE_SRV_DISABLED,.name = "HeartBeat",},
    [4] ={.p_srv = NULL,.p_param = NULL,.p_handle = NULL,.stack_size = 2048U,.priority = ZE_SRV_PRIO_NORMAL,.core_id = ZE_SRV_CORE_1,.stat = ZE_SRV_DISABLED,.name = "Location",},
    [5] ={.p_srv = ZE_Thread_SOMEIP,.p_param = NULL,.p_handle = NULL,.stack_size = 4096U,.priority = ZE_SRV_PRIO_BACKGROUND,.core_id = ZE_SRV_CORE_1,.stat = ZE_SRV_ENABLED,.name = "SOME-IP",},
};

/** @brief Global pointer to service list descriptor table */
ze_srv_t rw_ptr gp_ze_srv_list = s_srv_table;

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief Initialize and spawn all enabled application services */
ze_err_t ZE_Srv_Init(void) {
    ze_err_t err;

    for (uint32_t idx = 0U; idx < ZE_SRV_COUNT_MAX; idx++) {
        if (ZE_SRV_ENABLED == s_srv_table[idx].stat) {
            err = ZE_Srv_Start(idx);
            if (ZE_SUCCESS != err) {
               ZE_LOGE("Failed to auto-start service %s (err %d)", s_srv_table[idx].name, err);
                return err;
            }
        }
    }

    return ZE_SUCCESS;
}

/** @brief Get service configuration descriptor by service ID */
ze_err_t ZE_Srv_InfoGet(IN ze_srv_id_t src_id, OUT ze_srv_t rw_ptr p_service) {
    if (NULL == p_service) {
        return ZE_ERR_INVALID_ARG;
    }

    if (src_id >= ZE_SRV_COUNT_MAX) {
        return ZE_ERR_NOT_FOUND;
    }

    return ZE_MemCopy(p_service, &s_srv_table[src_id], sizeof(ze_srv_t));
}

/** @brief Update read-write fields of a service descriptor by service ID */
ze_err_t ZE_Srv_InfoSet(IN ze_srv_id_t src_id, IN ze_srv_t ro_ptr p_service) {
    if (NULL == p_service) {
        return ZE_ERR_INVALID_ARG;
    }

    if (src_id >= ZE_SRV_COUNT_MAX) {
        return ZE_ERR_NOT_FOUND;
    }

    ze_srv_t *p_target = &s_srv_table[src_id];

    /* Update only fields marked as read-write (rw) */
    p_target->p_param = p_service->p_param;
    p_target->stat = p_service->stat;

    (void)snprintf(p_target->name, sizeof(p_target->name), "%s", p_service->name);

    return ZE_SUCCESS;
}

/** @brief Get service configuration descriptor by service Name */
ze_err_t ZE_Srv_InfoGetByName(IN char ro_ptr p_srv_name, OUT ze_srv_t rw_ptr p_service) {
    if ((NULL == p_srv_name) || (NULL == p_service)) {
        return ZE_ERR_INVALID_ARG;
    }

    for (uint32_t idx = 0U; idx < ZE_SRV_COUNT_MAX; idx++) {
        if (0 == strncmp(s_srv_table[idx].name, p_srv_name, ZE_SRV_NAME_MAX_LEN)) {
            return ZE_MemCopy(p_service, &s_srv_table[idx], sizeof(ze_srv_t));
        }
    }

    return ZE_ERR_NOT_FOUND;
}

/** @brief Start and spawn a specific service task by ID using ESP-FreeRTOS */
ze_err_t ZE_Srv_Start(IN ze_srv_id_t src_id) {
    if (src_id >= ZE_SRV_COUNT_MAX) {
        return ZE_ERR_NOT_FOUND;
    }

    return ZE_RTOS_TaskStart(&s_srv_table[src_id]);
}

/** @brief Stop and terminate a running service task by ID using ESP-FreeRTOS */
ze_err_t ZE_Srv_Stop(IN ze_srv_id_t src_id) {
    if (src_id >= ZE_SRV_COUNT_MAX) {
        return ZE_ERR_NOT_FOUND;
    }

    return ZE_RTOS_TaskStop(&s_srv_table[src_id]);
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/