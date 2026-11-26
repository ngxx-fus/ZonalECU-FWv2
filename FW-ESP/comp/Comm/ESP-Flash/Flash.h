/********************************************************************************************************************************
 * @file Flash.h
 * @ingroup FW_ESP
 * @brief Header file for [[Flash]]
 ********************************************************************************************************************************/

/* GUARD/BEGIN *******************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef __FW_ESP_COMP_COMM_ESP_FLASH_FLASH_H__
#define __FW_ESP_COMP_COMM_ESP_FLASH_FLASH_H__

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdint.h>

#include "Common/Base/All.h"
#include "Common/Error/Error.h"

/* HEADERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_HEADER

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#ifndef ZE_FLASH_PAGE_SIZE
    #define ZE_FLASH_PAGE_SIZE    BYTE(256U)
#endif
#ifndef ZE_FLASH_SECT_SIZE
    #define ZE_FLASH_SECT_SIZE    KBYTE(4)
#endif
#ifndef ZE_FLASH_BLCK_SIZE
    #define ZE_FLASH_BLCK_SIZE    KBYTE(64)
#endif
#ifndef ZE_FLASH_BLCK_NUM
    #define ZE_FLASH_BLCK_NUM     (1U)
#endif

#ifndef ZE_PLACE_IN_FLASH
    #define ZE_PLACE_IN_FLASH     __attribute__((section(".rodata.ze_flash"), used, aligned(4)))
#endif
#ifndef ze_place_in_flash
    #define ze_place_in_flash     ZE_PLACE_IN_FLASH /*ZE_EXCEPTION(exception-for-type-macro)*/
#endif

#define ZE_FLASH_BASE_ADDRESS ((uintptr_t)&g_ze_flash_block)

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef union ze_fdata_page_un {
    rw ze_word_t word [ZE_FLASH_PAGE_SIZE / sizeof(ze_word_t)];
} ze_fpage_t;

typedef union ze_fsect_un {
    rw ze_fpage_t word [ZE_FLASH_SECT_SIZE / sizeof(ze_fpage_t)];
} ze_fsect_t;

typedef union ze_fblock_un {
    rw ze_fsect_t sect [ZE_FLASH_BLCK_SIZE / sizeof(ze_fsect_t)];
} ze_fblock_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

extern const ze_fblock_t g_ze_flash_block;

/* SHARED FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Open and initialize dedicated 64KB flash block
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_Flash_Open(void);

/** @brief   Read data from 64KB flash partition into RAM buffer
 *  @param   p_dest      RAM buffer to store read data
 *  @param   p_flash_src Flash variable address (e.g. &ze_var) or byte offset within 64KB
 *  @param   size        Number of bytes to read
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_Flash_Read(out void rw_ptr p_dest, in void ro_ptr p_flash_src, in uint32_t size);

/** @brief   Write data from RAM buffer to 64KB flash partition
 *  @param   p_flash_dest Flash variable address (e.g. &ze_var) or byte offset within 64KB
 *  @param   p_src        RAM source data buffer
 *  @param   size         Number of bytes to write
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_Flash_Write(in void ro_ptr p_flash_dest, in void ro_ptr p_src, in uint32_t size);

/** @brief   Close dedicated 64KB flash block
 *  @return  ZE_SUCCESS on success, or error code */
ze_err_t ZE_Flash_Close(void);

/* FOOTERS ***********************************************************************************************************************
 ********************************************************************************************************************************/

ZE_FOOTER

/* GUARD/END *********************************************************************************************************************
 ********************************************************************************************************************************/

#endif /* __FW_ESP_COMP_COMM_ESP_FLASH_FLASH_H__ */

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/