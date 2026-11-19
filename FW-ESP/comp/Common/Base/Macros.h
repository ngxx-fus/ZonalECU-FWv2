/**
 * @file Macros.h
 * @ingroup Common
 * @brief C/C++ cross-compatibility and parameter permission macros.
 */

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __COMMON_MACROS_H__
#define __COMMON_MACROS_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "__prv_common__.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

/* C / C++ Compatibility Macros */
#ifdef __cplusplus
    #ifndef HEADER_CPP
        #define HEADER_CPP     extern "C" {
    #endif
    #ifndef FOOTER_CPP
        #define FOOTER_CPP     }
    #endif

    #ifndef EXTERN_C_BEGIN
        #define EXTERN_C_BEGIN extern "C" {
    #endif
    #ifndef EXTERN_C_END
        #define EXTERN_C_END   }
    #endif
#else
    #ifndef HEADER_CPP
        #define HEADER_CPP
    #endif
    #ifndef FOOTER_CPP
        #define FOOTER_CPP
    #endif

    #ifndef EXTERN_C_BEGIN
        #define EXTERN_C_BEGIN
    #endif
    #ifndef EXTERN_C_END
        #define EXTERN_C_END
    #endif
#endif

#if !defined(ZE_HEADER) && defined(HEADER_CPP)
    #define ZE_HEADER      HEADER_CPP
#endif

#if !defined(ZE_FOOTER) && defined(FOOTER_CPP)
    #define ZE_FOOTER      FOOTER_CPP
#endif

#ifndef ZE_UNUSED
    #define ZE_UNUSED(x)   ((void)(x))
#endif

/* Macros for permission (Group 1: Uppercase) ***********************************************************************************/

#ifndef RO
    #define RO             const   /* Read only data permission */
#endif
#ifndef WO
    #define WO                     /* Write only data permission */
#endif
#ifndef RW
    #define RW                     /* Read/Write data permission */
#endif
#ifndef IN
    #define IN             const   /* Input parameter */
#endif
#ifndef OUT
    #define OUT                    /* Output parameter, shall be a pointer */
#endif
#ifndef INOUT
    #define INOUT                  /* Input/Output parameter, shall be a pointer */
#endif

#ifndef RO_PTR
    #define RO_PTR         * const /* Read only pointer address */
#endif
#ifndef WO_PTR
    #define WO_PTR         *       /* Write only pointer address */
#endif
#ifndef RW_PTR
    #define RW_PTR         *       /* Read/Write pointer address */
#endif

#ifndef SHARED
    #define SHARED         extern
#endif

/* Macros for permission (Group 2: Lowercase aliases) ***************************************************************************/
#ifndef ro
    #define ro             const   /* Read only data permission */
#endif
#ifndef wo
    #define wo                     /* Write only data permission */
#endif
#ifndef rw
    #define rw                     /* Read/Write data permission */
#endif
#ifndef in
    #define in             const   /* Input parameter */
#endif
#ifndef out
    #define out                    /* Output parameter, shall be a pointer */
#endif
#ifndef inout
    #define inout                  /* Input/Output parameter, shall be a pointer */
#endif

/** Example: 
 *      ro TYPE rw_ptr VAR
 *  - "rw_ptr VAR"    : Able to point to another memory region.
 *  - "ro TYPE"       : Write operation is PROHIBITED for the pointed region. */

#ifndef ro_ptr
    #define ro_ptr         * const /* Read only pointer address */
#endif
#ifndef wo_ptr
    #define wo_ptr         *       /* Write only pointer address */
#endif
#ifndef rw_ptr
    #define rw_ptr         *       /* Read/Write pointer address */
#endif

#ifndef shared
    #define shared         extern
#endif

/* Macros for permission (Group 3: Leading '__' Uppercase) **********************************************************************/
#ifndef __RO
    #define __RO           const   /* Read only data permission */
#endif
#ifndef __WO
    #define __WO                   /* Write only data permission */
#endif
#ifndef __RW
    #define __RW                   /* Read/Write data permission */
#endif
#ifndef __IN
    #define __IN           const   /* Input parameter */
#endif
#ifndef __OUT
    #define __OUT                  /* Output parameter, shall be a pointer */
#endif
#ifndef __INOUT
    #define __INOUT                /* Input/Output parameter, shall be a pointer */
#endif

#ifndef __RO_PTR
    #define __RO_PTR       * const /* Read only pointer address */
#endif
#ifndef __WO_PTR
    #define __WO_PTR       *       /* Write only pointer address */
#endif
#ifndef __RW_PTR
    #define __RW_PTR       *       /* Read/Write pointer address */
#endif

#ifndef __SHARED
    #define __SHARED       extern
#endif

/* Macros for permission (Group 4: Leading '__' Lowercase) **********************************************************************/
#ifndef __ro
    #define __ro           const   /* Read only data permission */
#endif
#ifndef __wo
    #define __wo                   /* Write only data permission */
#endif
#ifndef __rw
    #define __rw                   /* Read/Write data permission */
#endif
#ifndef __in
    #define __in           const   /* Input parameter */
#endif
#ifndef __out
    #define __out                  /* Output parameter, shall be a pointer */
#endif
#ifndef __inout
    #define __inout                /* Input/Output parameter, shall be a pointer */
#endif

#ifndef __ro_ptr
    #define __ro_ptr       * const /* Read only pointer address */
#endif
#ifndef __wo_ptr
    #define __wo_ptr       *       /* Write only pointer address */
#endif
#ifndef __rw_ptr
    #define __rw_ptr       *       /* Read/Write pointer address */
#endif

#ifndef __shared
    #define __shared       extern
#endif

/* Generic pointer macro (rw) **************************************************************************************************/

#ifndef __ptr
    #define __ptr          *
#endif

#ifndef ptr
    #define ptr            *
#endif

#ifndef PTR
    #define PTR            *
#endif

#ifndef __PTR
    #define __PTR          *
#endif

/* Readable check macros *******************************************************************************************************/

#define IS_NULL(X)     ((X) == (NULL))
#define IS_ZERO(X)     ((X) == 0)

#define IS_NOT_NULL(X) ((X) != (NULL))
#define IS_NOT_ZERO(X) ((X) != 0)

#define IS_GT(X, Y)    ((X) > (Y))
#define IS_LT(X, Y)    ((X) < (Y))
#define IS_EQ(X, Y)    ((X) == (Y))

#define CHART2UINT32(C3, C2, C1, C0)                                                                                             \
    (((uint32_t)(C3) << 24) | ((uint32_t)(C2) << 16) | ((uint32_t)(C1) << 8) | ((uint32_t)(C0) << 0))

/* Standard units ***************************************************************************************************************/

#ifndef BIT
    #define BIT(x)         (x)
#endif
#ifndef BYTE
    #define BYTE(x)        (x)
#endif
#ifndef KILO
    #define KILO(x)        ((x) << 10U)
#endif
#ifndef MEGA
    #define MEGA(x)        ((x) << 20U)
#endif
#ifndef KBYTE
    #define KBYTE(x)       KILO(x)
#endif
#ifndef MBYTE
    #define MBYTE(x)       MEGA(x)
#endif
#ifndef KBIT
    #define KBIT(x)        KILO(x)
#endif
#ifndef MBIT
    #define MBIT(x)        MEGA(x)
#endif

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __COMMON_MACROS_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/