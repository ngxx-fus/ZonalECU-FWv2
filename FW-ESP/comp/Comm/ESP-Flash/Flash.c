/********************************************************************************************************************************
 * @file Flash.c
 * @ingroup FW_ESP
 * @brief Source file for [[Flash]] driver
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "esp_attr.h"
#include "esp_rom_spiflash.h"
#include "spi_flash_mmap.h"

#include "Common/Base/All.h"
#include "Common/Bitwise/All.h"
#include "Common/Error/Error.h"
#include "Flash.h"
#include "Mem.h"

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define OFFSET_BYTE(addr)   SHR(AND(addr, 0x00000003U), 0x00U)
#define OFFSET_WORD(addr)   SHR(AND(addr, 0x000000FCU), 0x02U)
#define OFFSET_PAGE(addr)   SHR(AND(addr, 0x00000F00U), 0x08U)
#define OFFSET_SECT(addr)   SHR(AND(addr, 0x0000F000U), 0x0CU)
#define OFFSET_BLOCK(addr)  SHR(AND(addr, 0xFFFF0000U), 0x10U)

#define ZE_FLASH_CACHE_SIZE (128U)
#define ZE_FLASH_CACHE_MASK (~((uint32_t)(ZE_FLASH_CACHE_SIZE - 1U)))

/* DEFINITIONS *******************************************************************************************************************
 ********************************************************************************************************************************/

typedef struct ze_flash_cache_st {
    uint32_t tag; /* 128-byte aligned physical base address */
    uint8_t  valid; /* Cache valid flag */
    uint8_t  dirty; /* Cache dirty flag */
    uint8_t  reserved [2];
    uint32_t data     [ZE_FLASH_CACHE_SIZE / sizeof(uint32_t)]; /* 128-byte cache line (32 words) */
} ze_flash_cache_t;

/* EXPORTED VARIABLES ************************************************************************************************************
 ********************************************************************************************************************************/

/* Dedicated 64KB block placeholder in Flash */
__attribute__((section(".rodata.ze_flash"), used, aligned(4))) const ze_fblock_t g_ze_flash_block = {0};

/* PRIVATE VARIABLES *************************************************************************************************************
 ********************************************************************************************************************************/

static bool s_flash_opened = false;

static ze_flash_cache_t s_cache = {
    .tag   = UINT32_MAX,
    .valid = 0U,
    .dirty = 0U,
};

static uint32_t s_phys_base_addr = UINT32_MAX;

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Convert virtual cache address to physical SPI flash offset
 *  @param   offset Relative offset within the 64KB flash block
 *  @return  uint32_t Physical SPI Flash address or UINT32_MAX on failure */
static inline uint32_t prv_get_phys_addr(uint32_t offset) {
    if (UINT32_MAX == s_phys_base_addr) {
        size_t phys = spi_flash_cache2phys(&g_ze_flash_block);
        if (SPI_FLASH_CACHE2PHYS_FAIL == phys) {
            return UINT32_MAX;
        }
        s_phys_base_addr = (uint32_t)phys;
    }
    return s_phys_base_addr + offset;
}

/** @brief   Resolve offset within 64KB block from variable pointer or numerical offset
 *  @param   p_flash_addr Flash address or numerical offset
 *  @return  uint32_t Resolved byte offset, or UINT32_MAX if invalid */
static inline uint32_t prv_get_offset(const void *p_flash_addr) {
    ro uintptr_t addr = (uintptr_t)p_flash_addr;
    ro uintptr_t base = (uintptr_t)&g_ze_flash_block;

    if ((addr >= base) && (addr < (base + ZE_FLASH_BLCK_SIZE))) {
        return (uint32_t)(addr - base);
    }
    if (addr < ZE_FLASH_BLCK_SIZE) {
        return (uint32_t)addr;
    }
    return UINT32_MAX;
}

/** @brief   Validate 64KB flash block boundary check
 *  @param   offset Starting byte offset
 *  @param   size   Size in bytes
 *  @return  bool True if within 64KB, false if out of bounds */
static inline bool prv_is_valid_range(uint32_t offset, uint32_t size) {
    if ((0U == size) || (size > ZE_FLASH_BLCK_SIZE)) {
        return false;
    }
    if ((offset + size) > ZE_FLASH_BLCK_SIZE) {
        return false;
    }
    return true;
}

/** @brief   Write a 4-byte word to SPI Flash at word-aligned physical address
 *  @param   phys_addr 4-byte aligned physical address
 *  @param   val       32-bit word value to write
 *  @return  ze_err_t Status code */
static inline ze_err_t prv_write_word(uint32_t phys_addr, uint32_t val) {
    esp_rom_spiflash_result_t res = esp_rom_spiflash_write(phys_addr, &val, (int32_t)sizeof(uint32_t));
    if (ESP_ROM_SPIFLASH_RESULT_OK != res) {
        return ZE_ERR_INVALID_STATE;
    }
    return ZE_SUCCESS;
}

/** @brief   Write unaligned/aligned data buffer to SPI Flash with word-level merging
 *  @param   phys_addr Physical flash address
 *  @param   p_src     Source data pointer
 *  @param   n_bytes   Number of bytes to write
 *  @return  ze_err_t Status code */
static ze_err_t prv_write_flash_data(uint32_t phys_addr, ro void ro_ptr p_src, uint32_t n_bytes) {
    rw const uint8_t *p_curr_src = (const uint8_t *)p_src;
    rw uint32_t curr_phys = phys_addr;
    rw uint32_t remaining = n_bytes;

    /* Handle unaligned head (if phys_addr is not 4-byte aligned) */
    ro uint32_t head_offset = AND(curr_phys, 3U);
    if (0U != head_offset) {
        ro uint32_t aligned_phys = AND(curr_phys, ~3U);
        ro uint32_t head_bytes = 4U - head_offset;
        ro uint32_t bytes_to_copy = (remaining < head_bytes) ? remaining : head_bytes;

        uint32_t word_val = UINT32_MAX;
       esp_rom_spiflash_read(aligned_phys, &word_val, (int32_t)sizeof(uint32_t));

        for (uint32_t i = 0U; i < bytes_to_copy; i++) {
            ro uint32_t bit_pos = SHL(head_offset + i, 3U);
            ro uint32_t byte_mask = SHL(0xFFU, bit_pos);
            ro uint32_t new_byte = SHL((uint32_t)p_curr_src[i], bit_pos);
            word_val = OR(AND(word_val, INV(byte_mask)), new_byte);
        }

        ze_err_t err = prv_write_word(aligned_phys, word_val);
        if (ZE_SUCCESS != err) {
            return err;
        }

        curr_phys += bytes_to_copy;
        p_curr_src += bytes_to_copy;
        remaining -= bytes_to_copy;
    }

    /* Handle 4-byte aligned body */
    while (remaining >= 4U) {
        uint32_t word_val = 0U;
        (void)ZE_MemCopy(&word_val, p_curr_src, sizeof(uint32_t));

        ze_err_t err = prv_write_word(curr_phys, word_val);
        if (ZE_SUCCESS != err) {
            return err;
        }

        curr_phys += 4U;
        p_curr_src += 4U;
        remaining -= 4U;
    }

    /* Handle tail trailing bytes (< 4 bytes) */
    if (remaining > 0U) {
        uint32_t word_val = UINT32_MAX;
       esp_rom_spiflash_read(curr_phys, &word_val, (int32_t)sizeof(uint32_t));

        for (uint32_t i = 0U; i < remaining; i++) {
            ro uint32_t bit_pos = SHL(i, 3U);
            ro uint32_t byte_mask = SHL(0xFFU, bit_pos);
            ro uint32_t new_byte = SHL((uint32_t)p_curr_src[i], bit_pos);
            word_val = OR(AND(word_val, INV(byte_mask)), new_byte);
        }

        ze_err_t err = prv_write_word(curr_phys, word_val);
        if (ZE_SUCCESS != err) {
            return err;
        }
    }

    return ZE_SUCCESS;
}

/* PUBLIC VARIABLES **************************************************************************************************************
 ********************************************************************************************************************************/

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

/** @brief   Open dedicated flash block and bind base address
 *  @return  ze_err_t Status code */
ze_err_t ZE_Flash_Open(void) {
    if (s_flash_opened) {
        return ZE_ERR_ALREADY_OPENED;
    }

    /* Invalidate cache on open */
    s_cache.valid = 0U;
    s_cache.tag = UINT32_MAX;

    s_flash_opened = true;
    return ZE_SUCCESS;
}

/** @brief   Read binary payload from dedicated 64KB flash partition with 128B cache
 *  @param   p_dest      Output RAM buffer
 *  @param   p_flash_src Flash variable address (e.g. &ze_var) or byte offset within 64KB
 *  @param   size        Read size in bytes
 *  @return  ze_err_t Status code */
ze_err_t ZE_Flash_Read(out void rw_ptr p_dest, in void ro_ptr p_flash_src, in uint32_t size) {
    if (IS_NULL(p_dest) || IS_NULL(p_flash_src)) {
        return ZE_ERR_INVALID_ARG;
    }
    if (!s_flash_opened) {
        return ZE_ERR_NOT_OPENED;
    }

    ro uint32_t offset = prv_get_offset(p_flash_src);
    if (UINT32_MAX == offset) {
        return ZE_ERR_INVALID_ARG;
    }
    if (!prv_is_valid_range(offset, size)) {
        return ZE_ERR_INVALID_SIZE;
    }

    ro uint32_t phys_addr = prv_get_phys_addr(offset);
    if (UINT32_MAX == phys_addr) {
        return ZE_ERR_NOT_FOUND;
    }

    /* Check 128-byte cache hit */
    ro uint32_t cache_tag = AND(phys_addr, ZE_FLASH_CACHE_MASK);
    ro uint32_t cache_offset = AND(phys_addr, (ZE_FLASH_CACHE_SIZE - 1U));

    if ((1U == s_cache.valid) && (s_cache.tag == cache_tag) && ((cache_offset + size) <= ZE_FLASH_CACHE_SIZE)) {
        (void)ZE_MemCopy(p_dest, (const uint8_t *)s_cache.data + cache_offset, size);
        return ZE_SUCCESS;
    }

    /* Small read: reload 128-byte cache line */
    if ((cache_offset + size) <= ZE_FLASH_CACHE_SIZE) {
        esp_rom_spiflash_result_t res = esp_rom_spiflash_read(cache_tag, s_cache.data, (int32_t)ZE_FLASH_CACHE_SIZE);
        if (ESP_ROM_SPIFLASH_RESULT_OK == res) {
            s_cache.tag = cache_tag;
            s_cache.valid = 1U;
            (void)ZE_MemCopy(p_dest, (const uint8_t *)s_cache.data + cache_offset, size);
            return ZE_SUCCESS;
        }
    }

    /* Larger read: copy directly from mapped flash memory via ZE_MemCopy */
    ro uint8_t *p_flash_mem = (const uint8_t *)&g_ze_flash_block + offset;
    return ZE_MemCopy(p_dest, p_flash_mem, size);
}

/** @brief   Erase required sectors and write binary payload to dedicated flash partition
 *  @param   p_flash_dest Flash variable address (e.g. &ze_var) or byte offset within 64KB
 *  @param   p_src        Read-only input RAM buffer
 *  @param   size         Write size in bytes
 *  @return  ze_err_t Status code */
ze_err_t ZE_Flash_Write(in void ro_ptr p_flash_dest, in void ro_ptr p_src, in uint32_t size) {
    if (IS_NULL(p_flash_dest) || IS_NULL(p_src)) {
        return ZE_ERR_INVALID_ARG;
    }
    if (!s_flash_opened) {
        return ZE_ERR_NOT_OPENED;
    }

    ro uint32_t offset = prv_get_offset(p_flash_dest);
    if (UINT32_MAX == offset) {
        return ZE_ERR_INVALID_ARG;
    }
    if (!prv_is_valid_range(offset, size)) {
        return ZE_ERR_INVALID_SIZE;
    }

    ro uint32_t phys_addr = prv_get_phys_addr(offset);
    if (UINT32_MAX == phys_addr) {
        return ZE_ERR_NOT_FOUND;
    }

    /* Erase required 4KB sectors */
    ro uint32_t sector_start = phys_addr / ZE_FLASH_SECT_SIZE;
    ro uint32_t sector_end = (phys_addr + size - 1U) / ZE_FLASH_SECT_SIZE;

    for (uint32_t sec = sector_start; sec <= sector_end; sec++) {
        esp_rom_spiflash_result_t erase_res = esp_rom_spiflash_erase_sector(sec);
        if (ESP_ROM_SPIFLASH_RESULT_OK != erase_res) {
            return ZE_ERR_INVALID_STATE;
        }
    }

    /* Invalidate cache if overlapping */
    if ((1U == s_cache.valid) && (s_cache.tag >= (sector_start * ZE_FLASH_SECT_SIZE)) &&
        (s_cache.tag < ((sector_end + 1U) * ZE_FLASH_SECT_SIZE))) {
        s_cache.valid = 0U;
    }

    /* Write data with word alignment/unaligned handling */
    return prv_write_flash_data(phys_addr, p_src, size);
}

/** @brief   Close dedicated flash block
 *  @return  ze_err_t Status code */
ze_err_t ZE_Flash_Close(void) {
    if (!s_flash_opened) {
        return ZE_ERR_NOT_OPENED;
    }

    /* Invalidate cache on close */
    s_cache.valid = 0U;
    s_cache.tag = UINT32_MAX;

    s_flash_opened = false;
    return ZE_SUCCESS;
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/