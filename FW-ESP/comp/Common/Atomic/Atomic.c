/********************************************************************************************************************************
 * @file Atomic.c
 * @ingroup Atomic
 * @brief Source file for Atomic
 ********************************************************************************************************************************/

/* INCLUDES **************************************************************************************************
 ************************************************************************************************************/

#include "./Atomic.h"

/* MACROS ****************************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATION MACROS ***********************************************************************************************/

#define FN_ZE_AtomGET(type, suffix)                                                                        \
    static inline ze_err_t ZE_AtomGET##suffix(IN ze_atom_##type *p_var, OUT type *p_val) {                 \
        if (IS_NULL(p_var) || IS_NULL(p_val)) {                                                            \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        *p_val = atomic_load(p_var);                                                                       \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomSET(type, suffix)                                                                        \
    static inline ze_err_t AtomSET##suffix(INOUT ze_atom_##type *p_var, IN type val) {                     \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        atomic_store(p_var, val);                                                                          \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomCLR(type, suffix)                                                                       \
    static inline ze_err_t ZE_AtomCLR##suffix(INOUT ze_atom_##type *p_var) {                               \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        atomic_store(p_var, (type)0);                                                                      \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomGETnCLR(type, suffix)                                                                    \
    static inline ze_err_t ZE_AtomGETnCLR##suffix(INOUT ze_atom_##type *p_var, OUT type *p_val) {          \
        if (IS_NULL(p_var) || IS_NULL(p_val)) {                                                            \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        *p_val = atomic_exchange(p_var, (type)0);                                                          \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomAND(type, suffix)                                                                        \
    static inline ze_err_t ZE_AtomAND##suffix(INOUT ze_atom_##type *p_var, IN type mask,                   \
                                              OUT type *p_old_val) {                                       \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        type prev = atomic_fetch_and(p_var, mask);                                                         \
        if (ISNOT_NULL(p_old_val)) {                                                                       \
            *p_old_val = prev;                                                                             \
        }                                                                                                  \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomOR(type, suffix)                                                                         \
    static inline ze_err_t ZE_AtomOR##suffix(INOUT ze_atom_##type *p_var, IN type mask,                    \
                                             OUT type *p_old_val) {                                        \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        type prev = atomic_fetch_xor(p_var, mask);                                                         \
        if (ISNOT_NULL(p_old_val)) {                                                                       \
            *p_old_val = prev;                                                                             \
        }                                                                                                  \
        return ZE_OK;                                                                                      \
    }

#define FN_ZE_AtomNOT(type, suffix)                                                                        \
    static inline ze_err_t ZE_AtomNOT##suffix(INOUT ze_atom_##type *p_var, IN type mask,                   \
                                              OUT type *p_old_val) {                                       \
        if (IS_NULL(p_var)) {                                                                              \
            return ZE_ERR_INVALID_ARG;                                                                     \
        }                                                                                                  \
        type prev = atomic_fetch_xor(p_var, mask);                                                         \
        if (ISNOT_NULL(p_old_val)) {                                                                       \
            *p_old_val = prev;                                                                             \
        }                                                                                                  \
        return ZE_OK;                                                                                      \
    }

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

/* EXPORTED VARIABLES ****************************************************************************************
 ************************************************************************************************************/

/* PRIVATE VARIABLES *****************************************************************************************
 ************************************************************************************************************/

/* PRIVATE FUNCTIONS *****************************************************************************************
 ************************************************************************************************************/

/* PUBLIC VARIABLES ******************************************************************************************
 ************************************************************************************************************/

/* PUBLIC FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

/* FUNCTION IMPLEMENTATIONS **********************************************************************************
 ************************************************************************************************************/

FN_ZE_AtomGET(uint32_t, 32);
FN_ZE_AtomGET(uint16_t, 16);
FN_ZE_AtomGET(uint8_t, 8);
FN_ZE_AtomSET(uint32_t, 32);
FN_ZE_AtomSET(uint16_t, 16);
FN_ZE_AtomSET(uint8_t, 8);
FN_ZE_AtomCLR(uint32_t, 32);
FN_ZE_AtomCLR(uint16_t, 16);
FN_ZE_AtomCLR(uint8_t, 8);
FN_ZE_AtomGETnCLR(uint32_t, 32);
FN_ZE_AtomGETnCLR(uint16_t, 16);
FN_ZE_AtomGETnCLR(uint8_t, 8);
FN_ZE_AtomAND(uint32_t, 32);
FN_ZE_AtomAND(uint16_t, 16);
FN_ZE_AtomAND(uint8_t, 8);
FN_ZE_AtomOR(uint32_t, 32);
FN_ZE_AtomOR(uint16_t, 16);
FN_ZE_AtomOR(uint8_t, 8);
FN_ZE_AtomNOT(uint32_t, 32);
FN_ZE_AtomNOT(uint16_t, 16);
FN_ZE_AtomNOT(uint8_t, 8);

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
