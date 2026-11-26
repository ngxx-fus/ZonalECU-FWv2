/********************************************************************************************************************************
 * @file ESP-FreeRTOS.h
 * @ingroup ZonalECU_FWv2
 * @brief Header file for [[ESP-FreeRTOS]]. Standardized FreeRTOS primitives and wrapper APIs.
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __ZONALECU_FWV2_FW_ESP_COMP_APP_ESP_FREERTOS_ESP_FREERTOS_H__
#define __ZONALECU_FWV2_FW_ESP_COMP_APP_ESP_FREERTOS_ESP_FREERTOS_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "Common/Base/All.h"
#include "Common/Error/All.h"
#include "Comm/ESP-Mem/All.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define ZE_TASK_NAME_MAX_LEN (64U)        /*!< Maximum size of the name/label of the task */
#define ZE_RTOS_WAIT_FOREVER (UINT32_MAX) /*!< Infinite wait timeout definition in microseconds */

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief  Service task entry function pointer */
typedef TaskFunction_t ze_task_fn_t;

/** @brief  Service task parameter pointer */
typedef void *         ze_task_param_t;

/** @brief  Service runtime task handle */
typedef TaskHandle_t   ze_task_handle_t;

/** @brief  The service stack size */
typedef uint32_t       ze_task_stack_size_t;

/** @brief  The service identification type */
typedef uint32_t       ze_task_id_t;

/** @brief  Standardized Service Name buffer */
typedef char ze_task_name_t[ZE_TASK_NAME_MAX_LEN];

/** @brief  Standardized Service Status */
typedef enum ze_task_stat_e {
    ZE_TASK_DISABLED = 0U, /*!< Task is disabled by software */
    ZE_TASK_ENABLED  = 1U, /*!< Task is enabled and active */
} ze_task_stat_t;

/** @brief  Standardized Service Priorities scaled above IDLE priority */
typedef enum ze_task_prio_e {
    ZE_TASK_PRIO_BACKGROUND = 1U,                                           /*!< Lowest: (IDLE + 1) Logging, Monitor */
    ZE_TASK_PRIO_LOW        = 1U + ((configMAX_PRIORITIES - 2U) * 1U / 5U), /*!< Low: Stat LEDs, Non-critical */
    ZE_TASK_PRIO_NORMAL     = 1U + ((configMAX_PRIORITIES - 2U) * 2U / 5U), /*!< Standard: UI, General logic */
    ZE_TASK_PRIO_HIGH       = 1U + ((configMAX_PRIORITIES - 2U) * 3U / 5U), /*!< High: Sensor sampling, Motor */
    ZE_TASK_PRIO_URGENCY    = 1U + ((configMAX_PRIORITIES - 2U) * 4U / 5U), /*!< Critical: Safety checks, Comms */
    ZE_TASK_PRIO_REALTIME   = 1U + ((configMAX_PRIORITIES - 2U) * 5U / 5U), /*!< Maximum: Immediate hardware response */
} ze_task_prio_t;

/** @brief  Standardized Service Core Affinity */
typedef enum ze_task_core_e {
    ZE_TASK_CORE_0   = 1U, /*!< Task will be pinned to CORE0 */
    ZE_TASK_CORE_1   = 2U, /*!< Task will be pinned to CORE1 */
    ZE_TASK_CORE_ANY = 3U, /*!< Task will not be pinned to any specific CORE */
} ze_task_core_t;

/** @brief  Standardized Service Control Block */
typedef struct ze_task_st {
    ro ze_task_fn_t         p_srv;      /*!< Service task entry function */
    rw ze_task_param_t      p_param;    /*!< Service task parameter */
    rw ze_task_handle_t     p_handle;   /*!< Runtime FreeRTOS task handle */
    ro ze_task_stack_size_t stack_size; /*!< Allocated stack size */
    ro ze_task_prio_t       priority;   /*!< Service task priority */
    ro ze_task_core_t       core_id;    /*!< Core pinning affinity */
    rw ze_task_stat_t       stat;       /*!< Service lifecycle status */
    rw ze_task_name_t       name;       /*!< Human-readable service name */
} ze_task_t;

/** @brief  RTOS Mutex handle */
typedef SemaphoreHandle_t ze_rtos_mutex_t;

/** @brief  RTOS Semaphore handle */
typedef SemaphoreHandle_t ze_rtos_semaphore_t;

/** @brief  RTOS Queue handle */
typedef QueueHandle_t     ze_rtos_queue_t;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/* Task Management APIs **********************************************************************************************************/

/** @brief   Start and spawn a FreeRTOS task defined by ze_task_t
 *  @param   p_task Pointer to task control block
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_TaskStart(INOUT ze_task_t rw_ptr p_task);

/** @brief   Stop and delete a running FreeRTOS task
 *  @param   p_task Pointer to task control block
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_TaskStop(INOUT ze_task_t rw_ptr p_task);

/** @brief   Update task descriptor using local ZE_MemCopy
 *  @param   p_task_src Source task control block
 *  @param   p_task_dst Destination task control block
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_TaskUpdate(IN ze_task_t ro_ptr p_task_src, OUT ze_task_t rw_ptr p_task_dst);

/* Mutex APIs ********************************************************************************************************************/

/** @brief   Create a standard mutex
 *  @param   p_mutex Pointer to receive created mutex handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_MutexCreate(OUT ze_rtos_mutex_t rw_ptr p_mutex);

/** @brief   Take/Lock a mutex with timeout in microseconds
 *  @param   p_mutex    Mutex handle
 *  @param   timeout_us Timeout in microseconds (or ZE_RTOS_WAIT_FOREVER)
 *  @return  ZE_SUCCESS on success, ZE_ERR_TIMEOUT on timeout, or error code */
ze_err_t ZE_RTOS_MutexTake(IN ze_rtos_mutex_t p_mutex, IN uint32_t timeout_us);

/** @brief   Give/Unlock a mutex
 *  @param   p_mutex Mutex handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_MutexGive(IN ze_rtos_mutex_t p_mutex);

/* Semaphore APIs ****************************************************************************************************************/

/** @brief   Create a binary semaphore
 *  @param   p_sem Pointer to receive created semaphore handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_SemCreateBinary(OUT ze_rtos_semaphore_t rw_ptr p_sem);

/** @brief   Create a counting semaphore
 *  @param   max_count  Maximum count value
 *  @param   init_count Initial count value
 *  @param   p_sem      Pointer to receive created semaphore handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_SemCreateCounting(IN uint32_t max_count, IN uint32_t init_count, OUT ze_rtos_semaphore_t rw_ptr p_sem);

/** @brief   Take a semaphore with timeout in microseconds
 *  @param   p_sem      Semaphore handle
 *  @param   timeout_us Timeout in microseconds (or ZE_RTOS_WAIT_FOREVER)
 *  @return  ZE_SUCCESS on success, ZE_ERR_TIMEOUT on timeout, or error code */
ze_err_t ZE_RTOS_SemTake(IN ze_rtos_semaphore_t p_sem, IN uint32_t timeout_us);

/** @brief   Give a semaphore
 *  @param   p_sem Semaphore handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_SemGive(IN ze_rtos_semaphore_t p_sem);

/* Task Execution & Sleep APIs ***************************************************************************************************/

/** @brief  Yield current task execution to other ready tasks */
void ZE_RTOS_TaskYield(void);

/** @brief   Put current task to sleep for a specified duration in milliseconds
 *  @param   sleep_ms Sleep duration in milliseconds */
void ZE_RTOS_TaskSleep(IN uint32_t sleep_ms);

/** @brief   Wake up a target task from task context
 *  @param   p_handle Target task handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_TaskWakeUp(IN ze_task_handle_t p_handle);

/** @brief   Wake up a target task from ISR context
 *  @param   p_handle Target task handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_ISR_TaskWakeUp(IN ze_task_handle_t p_handle);

/* Queue APIs ********************************************************************************************************************/

/** @brief   Create a FreeRTOS message queue
 *  @param   queue_len Item capacity of the queue
 *  @param   item_size Size in bytes of each item
 *  @param   p_queue   Pointer to receive created queue handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_QueueCreate(IN uint32_t queue_len, IN uint32_t item_size, OUT ze_rtos_queue_t rw_ptr p_queue);

/** @brief   Clear/Reset all items in a queue
 *  @param   p_queue Queue handle
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_RTOS_QueueClear(IN ze_rtos_queue_t p_queue);

/** @brief   Push an item into queue with timeout
 *  @param   p_queue    Queue handle
 *  @param   p_data     Pointer to data buffer to copy from
 *  @param   n_bytes    Data size in bytes (must match queue item size)
 *  @param   timeout_us Timeout in microseconds (or ZE_RTOS_WAIT_FOREVER)
 *  @return  ZE_SUCCESS on success, ZE_ERR_TIMEOUT on timeout, or error code */
ze_err_t ZE_RTOS_QueuePush(IN ze_rtos_queue_t p_queue, IN ze_generic_data_t ro_ptr p_data, IN ze_size_t n_bytes,
                           IN uint32_t timeout_us);

/** @brief   Pop an item from queue with timeout
 *  @param   p_queue    Queue handle
 *  @param   p_data     Pointer to buffer to copy into
 *  @param   n_bytes    Expected buffer size in bytes
 *  @param   timeout_us Timeout in microseconds (or ZE_RTOS_WAIT_FOREVER)
 *  @return  ZE_SUCCESS on success, ZE_ERR_TIMEOUT on timeout, or error code */
ze_err_t ZE_RTOS_QueuePop(IN ze_rtos_queue_t p_queue, OUT ze_generic_data_t rw_ptr p_data, IN ze_size_t n_bytes,
                          IN uint32_t timeout_us);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __ZONALECU_FWV2_FW_ESP_COMP_APP_ESP_FREERTOS_ESP_FREERTOS_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/