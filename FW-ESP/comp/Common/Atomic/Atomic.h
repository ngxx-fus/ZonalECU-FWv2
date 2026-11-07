/********************************************************************************************************************************
 * @file Atomic.h
 * @ingroup Atomic
 * @brief Header file for Atomic operations and safe type wrappers.
 ********************************************************************************************************************************/

/* GUARD/BEGIN ***********************************************************************************************
 ************************************************************************************************************/

#ifndef __ATOMIC_ATOMIC_H__
#define __ATOMIC_ATOMIC_H__

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "../Config/Macros.h"
#include "../Error/Error.h"

/* HEADERS ***************************************************************************************************
 ************************************************************************************************************/

ZE_HEADER

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/** @brief Atomically load the value of an atomic variable (GetValue). */
#define ZE_AtomicGet(p_atomic) atomic_load(p_atomic)

/** @brief Atomically store a new value into an atomic variable. */
#define ZE_AtomicSet(p_atomic, val) atomic_store((p_atomic), (val))

/** @brief Atomically clear an atomic variable to zero (Clear). */
#define ZE_AtomicClear(p_atomic) atomic_store((p_atomic), 0U)

/** @brief Atomically retrieve current value and clear variable to zero (GetAndClear). */
#define ZE_AtomicGetAndClear(p_atomic) atomic_exchange((p_atomic), 0U)

/** @brief Atomically perform bitwise AND with mask on an atomic variable (self-AND). */
#define ZE_AtomicAnd(p_atomic, mask) atomic_fetch_and((p_atomic), (mask))

/** @brief Atomically perform bitwise OR with mask on an atomic variable (self-OR). */
#define ZE_AtomicOr(p_atomic, mask) atomic_fetch_or((p_atomic), (mask))

/** @brief Atomically increment an atomic variable by 1. */
#define ZE_AtomicInc(p_atomic) atomic_fetch_add((p_atomic), 1U)

/** @brief Atomically decrement an atomic variable by 1. */
#define ZE_AtomicDec(p_atomic) atomic_fetch_sub((p_atomic), 1U)

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

typedef _Atomic uint32_t ze_atom_uint32_t;
typedef _Atomic uint16_t ze_atom_uint16_t;
typedef _Atomic uint8_t ze_atom_uint8_t;
typedef _Atomic bool ze_atom_bool_t;
typedef ze_atom_bool_t ze_atom_bool;

/* EXPORTED VARIABLES ****************************************************************************************
 ************************************************************************************************************/

/* SHARED VARIABLES ******************************************************************************************
 ************************************************************************************************************/

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/** @brief Macro generator for type-safe atomic load function with error status check. */
#define FN_ZE_AtomicLoad(type, suffix)                                                                     \
    static inline ze_err_t ZE_AtomicLoad##suffix(IN ze_atom_##type *p_var, OUT type *p_val) {              \
        if (IS_NULL(p_var) || IS_NULL(p_val)) {                                                            \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        *p_val = atomic_load(p_var);                                                                       \
        return ZE_OK;                                                                                      \
    }

/** @brief Macro generator for type-safe atomic set / store function. */
#define FN_ZE_AtomicSet(type, suffix)                                                                      \
    static inline ze_err_t ZE_AtomicSet##suffix(INOUT ze_atom_##type *p_var, IN type val) {                \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        atomic_store(p_var, val);                                                                          \
        return ZE_OK;                                                                                      \
    }

/** @brief Macro generator for type-safe atomic clear function. */
#define FN_ZE_AtomicClear(type, suffix)                                                                    \
    static inline ze_err_t ZE_AtomicClear##suffix(INOUT ze_atom_##type *p_var) {                           \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        atomic_store(p_var, (type)0);                                                                      \
        return ZE_OK;                                                                                      \
    }

/** @brief Macro generator for type-safe atomic get-and-clear function. */
#define FN_ZE_AtomicGetAndClear(type, suffix)                                                              \
    static inline ze_err_t ZE_AtomicGetAndClear##suffix(INOUT ze_atom_##type *p_var, OUT type *p_val) {    \
        if (IS_NULL(p_var) || IS_NULL(p_val)) {                                                            \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        *p_val = atomic_exchange(p_var, (type)0);                                                          \
        return ZE_OK;                                                                                      \
    }

/** @brief Macro generator for type-safe atomic bitwise AND function (self-AND). */
#define FN_ZE_AtomicAnd(type, suffix)                                                                      \
    static inline ze_err_t ZE_AtomicAnd##suffix(INOUT ze_atom_##type *p_var, IN type mask,                 \
                                                OUT type *p_old_val) {                                     \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        type prev = atomic_fetch_and(p_var, mask);                                                         \
        if (ISNOT_NULL(p_old_val)) {                                                                       \
            *p_old_val = prev;                                                                             \
        }                                                                                                  \
        return ZE_OK;                                                                                      \
    }

/** @brief Macro generator for type-safe atomic bitwise OR function (self-OR). */
#define FN_ZE_AtomicOr(type, suffix)                                                                       \
    static inline ze_err_t ZE_AtomicOr##suffix(INOUT ze_atom_##type *p_var, IN type mask,                  \
                                               OUT type *p_old_val) {                                      \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        type prev = atomic_fetch_or(p_var, mask);                                                          \
        if (ISNOT_NULL(p_old_val)) {                                                                       \
            *p_old_val = prev;                                                                             \
        }                                                                                                  \
        return ZE_OK;                                                                                      \
    }

/* FUNCTION INSTANTIATIONS ***********************************************************************************
 ************************************************************************************************************/

/* Atomic Load Functions (32, 16, 8, Bool) ******************************************************************/

FN_ZE_AtomicLoad(uint32_t, 32);
FN_ZE_AtomicLoad(uint16_t, 16);
FN_ZE_AtomicLoad(uint8_t, 8);
FN_ZE_AtomicLoad(bool, Bool);

/* Atomic Set / Store Functions (32, 16, 8, Bool) ***********************************************************/

FN_ZE_AtomicSet(uint32_t, 32);
FN_ZE_AtomicSet(uint16_t, 16);
FN_ZE_AtomicSet(uint8_t, 8);
FN_ZE_AtomicSet(bool, Bool);

/* Atomic Clear Functions (32, 16, 8, Bool) *****************************************************************/

FN_ZE_AtomicClear(uint32_t, 32);
FN_ZE_AtomicClear(uint16_t, 16);
FN_ZE_AtomicClear(uint8_t, 8);
FN_ZE_AtomicClear(bool, Bool);

/* Atomic GetAndClear Functions (32, 16, 8, Bool) ***********************************************************/

FN_ZE_AtomicGetAndClear(uint32_t, 32);
FN_ZE_AtomicGetAndClear(uint16_t, 16);
FN_ZE_AtomicGetAndClear(uint8_t, 8);
FN_ZE_AtomicGetAndClear(bool, Bool);

/* Atomic Bitwise AND Functions (32, 16, 8) *****************************************************************/

FN_ZE_AtomicAnd(uint32_t, 32);
FN_ZE_AtomicAnd(uint16_t, 16);
FN_ZE_AtomicAnd(uint8_t, 8);

/* Atomic Bitwise OR Functions (32, 16, 8) ******************************************************************/

FN_ZE_AtomicOr(uint32_t, 32);
FN_ZE_AtomicOr(uint16_t, 16);
FN_ZE_AtomicOr(uint8_t, 8);

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __ATOMIC_ATOMIC_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
