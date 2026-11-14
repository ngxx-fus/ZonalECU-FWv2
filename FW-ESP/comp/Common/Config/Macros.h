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

/* Macros for permission (Group 1: Uppercase) **************************************/
#ifndef RO
    #define RO                 const   /* Read only data permission */
#endif
#ifndef WO
    #define WO                         /* Write only data permission */
#endif
#ifndef RW
    #define RW                         /* Read/Write data permission */
#endif
#ifndef IN
    #define IN                 const   /* Input parameter */
#endif
#ifndef OUT
    #define OUT                        /* Output parameter, shall be a pointer */
#endif
#ifndef INOUT
    #define INOUT                      /* Input/Output parameter, shall be a pointer */
#endif

#ifndef PTR_RO
    #define PTR_RO             * const /* Read only pointer address */
#endif
#ifndef PTR_WO
    #define PTR_WO             *       /* Write only pointer address */
#endif
#ifndef PTR_RW
    #define PTR_RW             *       /* Read/Write pointer address */
#endif

/* Macros for permission (Group 2: Lowercase aliases) ******************************/
#ifndef ro
    #define ro                 const   /* Read only data permission */
#endif
#ifndef wo
    #define wo                         /* Write only data permission */
#endif
#ifndef rw
    #define rw                         /* Read/Write data permission */
#endif
#ifndef in
    #define in                 const   /* Input parameter */
#endif
#ifndef out
    #define out                        /* Output parameter, shall be a pointer */
#endif
#ifndef inout
    #define inout                      /* Input/Output parameter, shall be a pointer */
#endif

#ifndef ptr_ro
    #define ptr_ro             * const /* Read only pointer address */
#endif
#ifndef ptr_wo
    #define ptr_wo             *       /* Write only pointer address */
#endif
#ifndef ptr_rw
    #define ptr_rw             *       /* Read/Write pointer address */
#endif

/* Macros for permission (Group 3: Leading '__' Uppercase) *************************/
#ifndef __RO
    #define __RO               const   /* Read only data permission */
#endif
#ifndef __WO
    #define __WO                       /* Write only data permission */
#endif
#ifndef __RW
    #define __RW                       /* Read/Write data permission */
#endif
#ifndef __IN
    #define __IN               const   /* Input parameter */
#endif
#ifndef __OUT
    #define __OUT                      /* Output parameter, shall be a pointer */
#endif
#ifndef __INOUT
    #define __INOUT                    /* Input/Output parameter, shall be a pointer */
#endif

#ifndef __PTR_RO
    #define __PTR_RO           * const /* Read only pointer address */
#endif
#ifndef __PTR_WO
    #define __PTR_WO           *       /* Write only pointer address */
#endif
#ifndef __PTR_RW
    #define __PTR_RW           *       /* Read/Write pointer address */
#endif

/* Macros for permission (Group 4: Leading '__' Lowercase) *************************/
#ifndef __ro
    #define __ro               const   /* Read only data permission */
#endif
#ifndef __wo
    #define __wo                       /* Write only data permission */
#endif
#ifndef __rw
    #define __rw                       /* Read/Write data permission */
#endif
#ifndef __in
    #define __in               const   /* Input parameter */
#endif
#ifndef __out
    #define __out                      /* Output parameter, shall be a pointer */
#endif
#ifndef __inout
    #define __inout                    /* Input/Output parameter, shall be a pointer */
#endif

#ifndef __ptr_ro
    #define __ptr_ro           * const /* Read only pointer address */
#endif
#ifndef __ptr_wo
    #define __ptr_wo           *       /* Write only pointer address */
#endif
#ifndef __ptr_rw
    #define __ptr_rw           *       /* Read/Write pointer address */
#endif


/* Readable check macros **********************************************************/

#define IS_NULL(X) ((X) == (NULL))
#define IS_ZERO(X) ((X) == 0)

#define IS_NOT_NULL(X) ((X) != (NULL))
#define IS_NOT_ZERO(X) ((X) != 0)

#define IS_GT(X, Y) ((X) > (Y))
#define IS_LT(X, Y) ((X) < (Y))
#define IS_EQ(X, Y) ((X) == (Y))

#define CHART2UINT32(C3, C2, C1, C0)    (((uint32_t)(C3) << 24) | \
                                        ((uint32_t)(C2) << 16)  | \
                                        ((uint32_t)(C1) << 8)   | \
                                        ((uint32_t)(C0) << 0))

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __COMMON_MACROS_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/