/********************************************************************************************************************************
 * @file ESP-FreeRTOS.c
 * @ingroup ZonalECU_FWv2
 * @brief Source file for [[ESP-FreeRTOS]]. Standardized FreeRTOS wrappers and synchronization primitives.
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "ESP-FreeRTOS.h"
#include "Comm/ESP-Logger/SysLog.h"
#include "Comm/ESP-Mem/Mem.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define ESP_FREERTOS_US_PER_SEC (1000000ULL)

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Convert microsecond timeout to FreeRTOS tick count
 *  @param   timeout_us Timeout in microseconds
 *  @return  Tick count or portMAX_DELAY */
static inline TickType_t prv_timeout_us_to_ticks(IN uint32_t timeout_us) {
    if (ZE_RTOS_WAIT_FOREVER == timeout_us) {
        return portMAX_DELAY;
    }
    if (0U == timeout_us) {
        return 0U;
    }

    /* Direct us to ticks conversion with ceiling rounding using 64-bit arithmetic to prevent overflow */
    uint64_t total_ticks = (((uint64_t)timeout_us * (uint64_t)configTICK_RATE_HZ) + (ESP_FREERTOS_US_PER_SEC - 1ULL)) /
                           ESP_FREERTOS_US_PER_SEC;

    if (total_ticks > (uint64_t)portMAX_DELAY) {
        return portMAX_DELAY;
    }

    return (TickType_t)((0ULL == total_ticks) ? 1U : total_ticks);
}

/** @brief   Map core affinity enum to FreeRTOS core ID
 *  @param   core_id Service core enum
 *  @return  0, 1, or tskNO_AFFINITY */
static inline BaseType_t prv_convert_core_id(IN ze_task_core_t core_id) {
    switch (core_id) {
        case ZE_TASK_CORE_0:
            return 0;
        case ZE_TASK_CORE_1:
            return 1;
        case ZE_TASK_CORE_ANY:
        default:
            return tskNO_AFFINITY;
    }
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/* Task Management APIs ********************************************************************************************************/
ze_err_t ZE_RTOS_TaskStart(INOUT ze_task_t rw_ptr p_task) {
    if (NULL == p_task) {
        return ZE_ERR_INVALID_ARG;
    }

    if (NULL != p_task->p_handle) {
       ZE_LOGW("Task %s is already running", p_task->name);
        return ZE_ERR_ALREADY_INIT;
    }

    if (NULL == p_task->p_srv) {
       ZE_LOGW("Task %s has no entry function assigned", p_task->name);
        return ZE_ERR_INVALID_STATE;
    }

    BaseType_t core = prv_convert_core_id(p_task->core_id);
    BaseType_t ret = xTaskCreatePinnedToCore(p_task->p_srv, p_task->name, p_task->stack_size, p_task->p_param, p_task->priority,
                                             &p_task->p_handle, core);

    if (pdPASS != ret) {
       ZE_LOGE("Failed to spawn task %s", p_task->name);
        return ZE_ERR_UNEXPECTED;
    }

    p_task->stat = ZE_TASK_ENABLED;
   ZE_LOGI("Task %s started on core %d (priority %u)", p_task->name, (int)core, (unsigned int)p_task->priority);

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_TaskStop(INOUT ze_task_t rw_ptr p_task) {
    if (NULL == p_task) {
        return ZE_ERR_INVALID_ARG;
    }

    if (NULL != p_task->p_handle) {
       vTaskDelete(p_task->p_handle);
        p_task->p_handle = NULL;
    }

    p_task->stat = ZE_TASK_DISABLED;
   ZE_LOGI("Task %s stopped", p_task->name);

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_TaskUpdate(IN ze_task_t ro_ptr p_task_src, OUT ze_task_t rw_ptr p_task_dst) {
    if ((NULL == p_task_src) || (NULL == p_task_dst)) {
        return ZE_ERR_INVALID_ARG;
    }

    /* Use local ZE_MemCopy from Comm/ESP-Mem/Mem.h */
    return ZE_MemCopy(p_task_dst, p_task_src, sizeof(ze_task_t));
}

/* Mutex APIs ******************************************************************************************************************/
ze_err_t ZE_RTOS_MutexCreate(OUT ze_rtos_mutex_t rw_ptr p_mutex) {
    if (NULL == p_mutex) {
        return ZE_ERR_INVALID_ARG;
    }

    *p_mutex = xSemaphoreCreateMutex();
    if (NULL == *p_mutex) {
       ZE_LOGE("Failed to create FreeRTOS mutex");
        return ZE_ERR_NO_MEM;
    }

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_MutexTake(IN ze_rtos_mutex_t p_mutex, IN uint32_t timeout_us) {
    if (NULL == p_mutex) {
        return ZE_ERR_INVALID_ARG;
    }

    TickType_t ticks = prv_timeout_us_to_ticks(timeout_us);
    BaseType_t ret = xSemaphoreTake(p_mutex, ticks);

    return (pdTRUE == ret) ? ZE_SUCCESS : ZE_ERR_TIMEOUT;
}

ze_err_t ZE_RTOS_MutexGive(IN ze_rtos_mutex_t p_mutex) {
    if (NULL == p_mutex) {
        return ZE_ERR_INVALID_ARG;
    }

    BaseType_t ret = xSemaphoreGive(p_mutex);
    return (pdTRUE == ret) ? ZE_SUCCESS : ZE_ERR_UNEXPECTED;
}

/* Semaphore APIs **************************************************************************************************************/
ze_err_t ZE_RTOS_SemCreateBinary(OUT ze_rtos_semaphore_t rw_ptr p_sem) {
    if (NULL == p_sem) {
        return ZE_ERR_INVALID_ARG;
    }

    *p_sem = xSemaphoreCreateBinary();
    if (NULL == *p_sem) {
       ZE_LOGE("Failed to create FreeRTOS binary semaphore");
        return ZE_ERR_NO_MEM;
    }

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_SemCreateCounting(IN uint32_t max_count, IN uint32_t init_count, OUT ze_rtos_semaphore_t rw_ptr p_sem) {
    if ((NULL == p_sem) || (0U == max_count) || (init_count > max_count)) {
        return ZE_ERR_INVALID_ARG;
    }

    *p_sem = xSemaphoreCreateCounting(max_count, init_count);
    if (NULL == *p_sem) {
       ZE_LOGE("Failed to create FreeRTOS counting semaphore");
        return ZE_ERR_NO_MEM;
    }

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_SemTake(IN ze_rtos_semaphore_t p_sem, IN uint32_t timeout_us) {
    if (NULL == p_sem) {
        return ZE_ERR_INVALID_ARG;
    }

    TickType_t ticks = prv_timeout_us_to_ticks(timeout_us);
    BaseType_t ret = xSemaphoreTake(p_sem, ticks);

    return (pdTRUE == ret) ? ZE_SUCCESS : ZE_ERR_TIMEOUT;
}

ze_err_t ZE_RTOS_SemGive(IN ze_rtos_semaphore_t p_sem) {
    if (NULL == p_sem) {
        return ZE_ERR_INVALID_ARG;
    }

    BaseType_t ret = xSemaphoreGive(p_sem);
    return (pdTRUE == ret) ? ZE_SUCCESS : ZE_ERR_UNEXPECTED;
}

/* Task Execution & Sleep APIs *************************************************************************************************/
void ZE_RTOS_TaskYield(void) {
   taskYIELD();
}

void ZE_RTOS_TaskSleep(IN uint32_t sleep_ms) {
   vTaskDelay(pdMS_TO_TICKS(sleep_ms));
}

ze_err_t ZE_RTOS_TaskWakeUp(IN ze_task_handle_t p_handle) {
    if (NULL == p_handle) {
        return ZE_ERR_INVALID_ARG;
    }

   vTaskResume(p_handle);
    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_ISR_TaskWakeUp(IN ze_task_handle_t p_handle) {
    if (NULL == p_handle) {
        return ZE_ERR_INVALID_ARG;
    }

    BaseType_t yield = xTaskResumeFromISR(p_handle);
   portYIELD_FROM_ISR(yield);

    return ZE_SUCCESS;
}

/* Queue APIs ******************************************************************************************************************/
ze_err_t ZE_RTOS_QueueCreate(IN uint32_t queue_len, IN uint32_t item_size, OUT ze_rtos_queue_t rw_ptr p_queue) {
    if ((NULL == p_queue) || (0U == queue_len) || (0U == item_size)) {
        return ZE_ERR_INVALID_ARG;
    }

    *p_queue = xQueueCreate(queue_len, item_size);
    if (NULL == *p_queue) {
       ZE_LOGE("Failed to create FreeRTOS queue");
        return ZE_ERR_NO_MEM;
    }

    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_QueueClear(IN ze_rtos_queue_t p_queue) {
    if (NULL == p_queue) {
        return ZE_ERR_INVALID_ARG;
    }

    (void)xQueueReset(p_queue);
    return ZE_SUCCESS;
}

ze_err_t ZE_RTOS_QueuePush(IN ze_rtos_queue_t p_queue, IN ze_generic_data_t ro_ptr p_data, IN ze_size_t n_bytes,
                           IN uint32_t timeout_us) {
    (void)n_bytes;
    if ((NULL == p_queue) || (NULL == p_data)) {
        return ZE_ERR_INVALID_ARG;
    }

    TickType_t ticks = prv_timeout_us_to_ticks(timeout_us);
    BaseType_t ret = xQueueSend(p_queue, p_data, ticks);

    return (pdPASS == ret) ? ZE_SUCCESS : ZE_ERR_TIMEOUT;
}

ze_err_t ZE_RTOS_QueuePop(IN ze_rtos_queue_t p_queue, OUT ze_generic_data_t rw_ptr p_data, IN ze_size_t n_bytes,
                          IN uint32_t timeout_us) {
    (void)n_bytes;
    if ((NULL == p_queue) || (NULL == p_data)) {
        return ZE_ERR_INVALID_ARG;
    }

    TickType_t ticks = prv_timeout_us_to_ticks(timeout_us);
    BaseType_t ret = xQueueReceive(p_queue, p_data, ticks);

    return (pdPASS == ret) ? ZE_SUCCESS : ZE_ERR_TIMEOUT;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/