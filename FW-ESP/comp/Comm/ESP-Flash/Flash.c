/********************************************************************************************************************************
 * @file Flash.c
 * @ingroup DIRECTORY
 * @brief Source file for [[Flash]]
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "esp_attr.h"
#include "spi_flash_mmap.h"
#include "esp_rom_spiflash.h"
 
#include "Bitwise.h"
#include "Error.h"
#include "Flash.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

/* Flash address bitfield breakdown for ESP32-S3
 *  1 BLOCK = 16 SECTORS
 *              1 SECTOR = 16 PAGES
 *                          1 PAGE = 64 WORDS
 *                                    1 WORD = 4 BYTES
 *  MSB (31-bit)                          LSB (0-bit)
 *  ^                                     ^ 
 *  3322 2222 2222 1111 1111 1100 0000 0000
 *  1098 7654 3210 9876 5432 1098 7654 3210
 *  XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX
 *  ^^^^ ^^^^ ^^^^ ^^^^---------------------------> Block index      ADDR[31:16],   nbits
 *                      ^^^^----------------------> Sector index     ADDR[15:12],   4bits
 *                           ^^^^-----------------> Page index       ADDR[11:8],    4bits
 *                                ^^^^ ^^---------> Word index       ADDR[7:2],     6bits
 *                                       ^^-------> Byte index       ADDR[1:0],     2bits
 */

#define OFFSET_BYTE(addr)    BW_SHR(BW_AND(addr, 0x00000003), 0x00)
#define OFFSET_WORD(addr)    BW_SHR(BW_AND(addr, 0x000000FC), 0x02)
#define OFFSET_PAGE(addr)    BW_SHR(BW_AND(addr, 0x00000F00), 0x08)
#define OFFSET_SECT(addr)    BW_SHR(BW_AND(addr, 0x0000F000), 0x0C)
#define OFFSET_BLOCK(addr)   BW_SHR(BW_AND(addr, 0xFFFF0000), 0x10)

#define ZE_FLASH_OPENED      CHART2UINT32('O', 'P', 'E', 'N')
#define ZE_FLASH_NOT_OPENED  CHART2UINT32('N', 'O', 'N', 'E')
#define ZE_FLASH_PAGE_SIZE   BYTE(256U)     /* Min of write size */
#define ZE_FLASH_SECTOR_SIZE KBYTE(4)       /* Min of erase size */
#define ZE_FLASH_BLOCK_SIZE  KBYTE(64)      /* Max of erase size */
#define ZE_FLASH_BLOCK_NUM   (1U)           /* Number of flash block */
#define ZE_FLASH_BLOCK_ADDR                 /* Place holder, TODO: Implement the actual address! */

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef union ze_flash_data32_un {
    uint8_t     byte    [4];
    uint16_t    half    [2];
    uint32_t    word;
} ze_flash_udata32_t;

typedef union ze_flash_data_page_un {
    ze_flash_udata32_t  data32[ZE_FLASH_PAGE_SIZE / sizeof(ze_flash_udata32_t)];
} ze_flash_page_t;

typedef union ze_flash_sector_un {
    ze_flash_page_t     page[ZE_FLASH_SECTOR_SIZE / sizeof(ze_flash_page_t)];
} ze_flash_sector_t;

typedef union ze_flash_block_un {
    ze_flash_sector_t   sector[ZE_FLASH_BLOCK_SIZE / sizeof(ze_flash_sector_t)];
} ze_flash_block_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

static ze_flash_info_t prv_ze_flash_info = {
    .block_size  = ZE_FLASH_PAGE_SIZE,
    .page_size   = ZE_FLASH_SECTOR_SIZE,
    .sector_size = ZE_FLASH_SECTOR_SIZE,
    .block_num   = ZE_FLASH_BLOCK_NUM
};

#if ZE_PSRAM_EN
    ze_flash_sector_t PLACE_IN_SECTION("psram") prv_cache_sector[2];
#else
    ze_flash_sector_t prv_cache_sector[2];
#endif

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

ze_flash_instance_t g_ze_flash_ctrl = {
    .open   = ZE_FLASH_NOT_OPENED,
    .p_info = &prv_ze_flash_info,
    .p_cfg  = NULL,
    .p_ext  = NULL,
};



/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

ze_err_t prv_flash_read(inout ze_flash_instance_t rw_ptr p_ctrl, in void rw_ptr p_data, in uint32_t size){
    /* BEGIN */
    uint32_t ro addr32_beg              = (uint32_t) p_data;
    uint32_t ro index_byte_beg         = OFFSET_BYTE(addr32_beg);
    uint32_t ro index_word_beg         = OFFSET_WORD(addr32_beg);
    uint32_t ro index_sect_beg         = OFFSET_SECT(addr32_beg);
    uint32_t ro index_block_beg        = OFFSET_BLOCK(addr32_beg);
    
    /* END */
    uint32_t ro addr32_end              = ((uint32_t) p_data) + size - 1U;
    uint32_t ro index_byte_end         = OFFSET_BYTE(addr32_end);
    uint32_t ro index_word_end         = OFFSET_WORD(addr32_end);
    uint32_t ro index_page_end         = OFFSET_PAGE(addr32_end);
    uint32_t ro index_sect_end         = OFFSET_SECT(addr32_end);
    uint32_t ro index_block_end        = OFFSET_BLOCK(addr32_end);

    /* PROCESS */

    if(index_block_beg != index_block_end){

        /* Only supports within block read action */
        return ZE_ERR_INVALID_ARG;
    }

    esp_rom_spiflash_read(ZE_FLASH_BLOCK_ADDR, (uint32_t *)p_buffer, size);

    return ZE_SUCCESS;
}

ze_err_t prv_ze_flash_write_unaligned_data(inout ze_flash_instance_t rw_ptr p_ctrl, in void rw_ptr p_data, in uint32_t size) {

    return ZE_SUCCESS;
}

ze_err_t prv_ze_flash_write_aligned_data(inout ze_flash_instance_t rw_ptr p_ctrl, in void ro_ptr p_data, in uint32_t size) {
    return ZE_SUCCESS;
}

ze_err_t prv_ze_flash_sync(inout ze_flash_instance_t rw_ptr p_ctrl, in void ro_ptr p_data, in uint32_t size) {
    
    /* Determine */
    return ZE_SUCCESS;
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

ze_err_t ZE_Flash_Open(inout ze_flash_instance_t rw_ptr p_ctrl, in ze_flash_cfg_t ro_ptr p_cfg) {
    if (ZE_FLASH_OPENED == p_ctrl->open) {
        return ZE_ERR_ALREADY_OPENED;
    }

    /* Place the code inherited from ESP to open flash*/

    p_ctrl->open = ZE_FLASH_OPENED;
    return ZE_SUCCESS;
}
ze_err_t ZE_Flash_Read(inout ze_flash_instance_t rw_ptr p_ctrl, in void ro_ptr p_data, in uint32_t size) {

    return ZE_SUCCESS;
}
ze_err_t ZE_Flash_Write(inout ze_flash_instance_t rw_ptr p_ctrl, in void rw_ptr p_data, in uint32_t size) {

    return ZE_SUCCESS;
}
ze_err_t ZE_Flash_Close(inout ze_flash_instance_t rw_ptr p_ctrl) {

    p_ctrl->open = ZE_FLASH_NOT_OPENED;
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/