/********************************************************************************************************************************
 * @file Atomic.h
 * @ingroup Atomic
 * @brief Header file for Atomic
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

#define ZE_AtomGET(p_atomic)       atomic_load(p_atomic)
#define ZE_AtomSET(p_atomic, val)  atomic_store((p_atomic), (val))
#define ZE_AtomCLR(p_atomic)       atomic_store((p_atomic), 0U)
#define ZE_AtomGETnCLR(p_atomic)   atomic_exchange((p_atomic), 0U)
#define ZE_AtomAND(p_atomic, mask) atomic_fetch_and((p_atomic), (mask))
#define ZE_AtomOR(p_atomic, mask)  atomic_fetch_or((p_atomic), (mask))
#define ZE_AtomNOT(p_atomic, mask) atomic_fetch_xor((p_atomic), (mask))
#define ZE_AtomicInc(p_atomic)     atomic_fetch_add((p_atomic), 1U)
#define ZE_AtomicDec(p_atomic)     atomic_fetch_sub((p_atomic), 1U)

/* FUNCTION PROTOTYPE MACROS *********************************************************************************/

#define FN_ZE_AtomGET_Prototype(type, suffix)                                                              \
    ze_err_t ZE_AtomGET##suffix(IN ze_atom_##type *p_var, OUT type *p_val);
#define FN_ZE_AtomSET_Prototype(type, suffix)                                                              \
    ze_err_t AtomSET##suffix(INOUT ze_atom_##type *p_var, IN type val);
#define FN_ZE_AtomCLR_Prototype(type, suffix)                                                              \
    ze_err_t ZE_AtomCLR##suffix(INOUT ze_atom_##type *p_var)
#define FN_ZE_AtomGETnCLR_Prototype(type, suffix)                                                          \
    ze_err_t ZE_AtomGETnCLR##suffix(INOUT ze_atom_##type *p_var, OUT type *p_val);
#define FN_ZE_AtomAND_Prototype(type, suffix)                                                              \
    ze_err_t ZE_AtomAND##suffix(INOUT ze_atom_##type *p_var, IN type mask,                   \
                                              OUT type *p_old_val);
#define FN_ZE_AtomOR_Prototype(type, suffix)                                                               \
    ze_err_t ZE_AtomOR##suffix(INOUT ze_atom_##type *p_var, IN type mask,                    \
                                             OUT type *p_old_val);
#define FN_ZE_AtomNOT_Prototype(type, suffix)                                                              \
    ze_err_t ZE_AtomNOT##suffix(INOUT ze_atom_##type *p_var, IN type mask,                   \
                                              OUT type *p_old_val);

/* DEFINITIONS ***********************************************************************************************
 ************************************************************************************************************/

typedef _Atomic uint32_t ze_atom_uint32_t;
typedef _Atomic uint16_t ze_atom_uint16_t;
typedef _Atomic uint8_t ze_atom_uint8_t;

/* EXPORTED VARIABLES ****************************************************************************************
 ************************************************************************************************************/

/* SHARED VARIABLES ******************************************************************************************
 ************************************************************************************************************/

/* SHARED FUNCTIONS ******************************************************************************************
 ************************************************************************************************************/

FN_ZE_AtomGET_Prototype(uint32_t, 32);
FN_ZE_AtomGET_Prototype(uint16_t, 16);
FN_ZE_AtomGET_Prototype(uint8_t, 8);
FN_ZE_AtomSET_Prototype(uint32_t, 32);
FN_ZE_AtomSET_Prototype(uint16_t, 16);
FN_ZE_AtomSET_Prototype(uint8_t, 8);
FN_ZE_AtomCLR_Prototype(uint32_t, 32);
FN_ZE_AtomCLR_Prototype(uint16_t, 16);
FN_ZE_AtomCLR_Prototype(uint8_t, 8);
FN_ZE_AtomGETnCLR_Prototype(uint32_t, 32);
FN_ZE_AtomGETnCLR_Prototype(uint16_t, 16);
FN_ZE_AtomGETnCLR_Prototype(uint8_t, 8);
FN_ZE_AtomAND_Prototype(uint32_t, 32);
FN_ZE_AtomAND_Prototype(uint16_t, 16);
FN_ZE_AtomAND_Prototype(uint8_t, 8);
FN_ZE_AtomOR_Prototype(uint32_t, 32);
FN_ZE_AtomOR_Prototype(uint16_t, 16);
FN_ZE_AtomOR_Prototype(uint8_t, 8);
FN_ZE_AtomNOT_Prototype(uint32_t, 32);
FN_ZE_AtomNOT_Prototype(uint16_t, 16);
FN_ZE_AtomNOT_Prototype(uint8_t, 8);

/* Free the prv macro */

#undef FN_ZE_AtomGET_Prototype
#undef FN_ZE_AtomSET_Prototype
#undef FN_ZE_AtomCLR_Prototype
#undef FN_ZE_AtomGETnCLR_Prototype
#undef FN_ZE_AtomAND_Prototype
#undef FN_ZE_AtomOR_Prototype
#undef FN_ZE_AtomNOT_Prototype

/* FOOTERS ***************************************************************************************************
 ************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *************************************************************************************************
 ************************************************************************************************************/

#endif /* __ATOMIC_ATOMIC_H__ */

/* EOF *******************************************************************************************************
 ************************************************************************************************************/
