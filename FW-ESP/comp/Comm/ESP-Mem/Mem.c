/********************************************************************************************************************************
 * @file Mem.c
 * @ingroup DIRECTORY
 * @brief Source file for [[Mem]]
 ********************************************************************************************************************************/

/* INCLUDES **********************************************************************************************************************
 ********************************************************************************************************************************/

#include "Mem.h"

#include "Common/Base/All.h"
#include "Common/Error/All.h"
#include "Common/Bitwise/All.h"
#include <stdint.h>
#include <string.h>

/* MACROS ************************************************************************************************************************
 ********************************************************************************************************************************/

#define BYTE_INDEX(addr)        AND(UINTPTR(addr), 3U)
#define BIT_INDEX(addr)         SHL(AND(UINTPTR(addr), 3U), 3U)
#define WORD_BASE(addr)         AND(UINTPTR(addr), ~((uintptr_t)3U))
#define x8                      << 3
#define L                       0U
#define H                       1U
#define WORD_WIDTH              (0x20U)
#define WORD_MAX                (UINT32_MAX)
#define SPEED_UP_THRES          12U

#ifndef ZE_MEM_USE_PLATFORM_ASM
    #if defined(ESP_PLATFORM) && (defined(__XTENSA__) || defined(CONFIG_IDF_TARGET_ESP32))
        #define ZE_MEM_USE_PLATFORM_ASM 1
    #else
        #define ZE_MEM_USE_PLATFORM_ASM 0
    #endif
#endif

/* PRIVATE FUNCTIONS *************************************************************************************************************
 ********************************************************************************************************************************/

static inline ze_err_t prv_assign_to_aligned_dst(rw void ro_ptr p_dst, ro uint8_t byte, ro uint32_t n_bytes) {
    if (0 != BYTE_INDEX(p_dst)) {
        /* p_dst must be aligned 4 */
        return ZE_ERR_INVALID_ARG;
    }

    rw ze_word_t rw_ptr p_dst_word = (ze_wordptr_t)WORD_BASE(p_dst);
    ro uint32_t fill_word = (uint32_t)byte * 0x01010101U;
    rw uint32_t n_bits = SHL(n_bytes, 3U);

    while (WORD_WIDTH <= n_bits) {
        (p_dst_word++)->word = fill_word;
        n_bits -= WORD_WIDTH;
    }

    if ((0 < n_bits) && (n_bits < WORD_WIDTH)) {
        ro uint32_t mask = INV(SHL(WORD_MAX, n_bits));
        ro uint32_t word_a = AND(fill_word, mask);
        ro uint32_t word_b = AND(p_dst_word->word, INV(mask));
        p_dst_word->word = OR(word_a, word_b);
    }

    return ZE_SUCCESS;
}

static inline ze_err_t prv_assign_to_unaligned_dst(rw void ro_ptr p_dst, ro uint8_t byte, ro uint32_t n_bytes) {
    if (n_bytes < SPEED_UP_THRES) {
        rw uint8_t rw_ptr d = (rw uint8_t rw_ptr)p_dst;
        rw uint32_t bytes_left = n_bytes;
        while (bytes_left--) {
            *d++ = byte;
        }
        return ZE_SUCCESS;
    }

    rw ze_word_t rw_ptr p_dst_word = (ze_wordptr_t)WORD_BASE(p_dst);
    ro uint32_t unaligned_dst_bit_idx = BIT_INDEX(p_dst);
    ro uint32_t unaligned_dst_bit_num = WORD_WIDTH - unaligned_dst_bit_idx;
    ro uint32_t unaligned_dst_byte_num = SHR(unaligned_dst_bit_num, 3U);
    ro uint32_t fill_word = (uint32_t)byte * 0x01010101U;

    if (0U == unaligned_dst_bit_idx) {
        return prv_assign_to_aligned_dst(p_dst, byte, n_bytes);
    }

    /* Fill head of unaligned destination word */
    ro uint32_t mask = SHL(WORD_MAX, unaligned_dst_bit_idx);
    ro uint32_t word_a = AND(fill_word, mask);
    ro uint32_t word_b = AND(p_dst_word->word, INV(mask));
    p_dst_word->word = OR(word_a, word_b);

    return prv_assign_to_aligned_dst((rw uint8_t rw_ptr)p_dst + unaligned_dst_byte_num, byte, n_bytes - unaligned_dst_byte_num);
}

static inline ze_err_t prv_copy_to_aligned_dst(rw void ro_ptr p_dst, ro void ro_ptr p_src, ro uint32_t n_bytes) {
    if (0 != BYTE_INDEX(p_dst)) {
        /* p_dst must be aligned 4 */
        return ZE_ERR_INVALID_ARG;
    }

    ro ze_word_t rw_ptr p_src_word = (ze_wordptr_t)WORD_BASE(p_src);
    rw ze_word_t rw_ptr p_dst_word = (ze_wordptr_t)WORD_BASE(p_dst);
    ro uint32_t unaligned_bit_idx = BIT_INDEX(p_src);
    ro uint32_t unaligned_bit_num = AND(0x20U - unaligned_bit_idx, 0x1F);
    ro uint32_t borrowed_bit_num = unaligned_bit_idx;
    rw uint32_t temp_word_a = 0U;
    rw uint32_t temp_word_b = 0U;
    rw uint32_t n_bits = SHL(n_bytes, 3);

    if (0U == unaligned_bit_num) {
        while (WORD_WIDTH <= n_bits) {
            (p_dst_word++)->word = (p_src_word++)->word;
            n_bits -= WORD_WIDTH;
        }
    } else {
        temp_word_a = (p_src_word++)->word;
        while (WORD_WIDTH <= n_bits) {
            temp_word_b = (p_src_word++)->word;
            temp_word_a = SHR(temp_word_a, borrowed_bit_num);
            temp_word_a = OR(temp_word_a, SHL(temp_word_b, unaligned_bit_num));
            (p_dst_word++)->word = temp_word_a;
            temp_word_a = temp_word_b;
            n_bits -= WORD_WIDTH;
        }
    }

    if ((0 < n_bits) && (n_bits < WORD_WIDTH)) {
        if (0U == unaligned_bit_num) {
            temp_word_a = p_src_word->word;
        } else {
            temp_word_a = SHR(temp_word_a, borrowed_bit_num);
            if (n_bits > unaligned_bit_num) {
                temp_word_b = p_src_word->word;
                temp_word_a = OR(temp_word_a, SHL(temp_word_b, unaligned_bit_num));
            }
        }
        temp_word_a = AND(temp_word_a, INV(SHL(WORD_MAX, n_bits)));
        temp_word_b = p_dst_word->word;
        temp_word_b = AND(temp_word_b, SHL(WORD_MAX, n_bits));
        p_dst_word->word = OR(temp_word_a, temp_word_b);
    }

    return ZE_SUCCESS;
}

static inline ze_err_t prv_copy_to_unaligned_dst(rw void ro_ptr p_dst, ro void ro_ptr p_src, ro uint32_t n_bytes) {
    if (n_bytes < SPEED_UP_THRES) {
        rw uint8_t rw_ptr d = (rw uint8_t rw_ptr)p_dst;
        ro uint8_t rw_ptr s = (ro uint8_t rw_ptr)p_src;
        rw uint32_t bytes_left = n_bytes;
        while (bytes_left--) {
            *d++ = *s++;
        }
        return ZE_SUCCESS;
    }

    ro ze_word_t rw_ptr p_src_word = (ze_wordptr_t)WORD_BASE(p_src);
    rw ze_word_t rw_ptr p_dst_word = (ze_wordptr_t)WORD_BASE(p_dst);
    ro uint32_t unaligned_dst_bit_idx = BIT_INDEX(p_dst);
    ro uint32_t unaligned_dst_bit_num = WORD_WIDTH - unaligned_dst_bit_idx;
    ro uint32_t unaligned_dst_byte_num = SHR(unaligned_dst_bit_num, 3U);
    ro uint32_t unaligned_src_bit_idx = BIT_INDEX(p_src);
    ro uint32_t unaligned_src_bit_num = WORD_WIDTH - unaligned_src_bit_idx;
    rw uint32_t temp_word_a = 0U;
    rw uint32_t temp_word_b = 0U;
    rw uint32_t reminder_bit_num = 0U;

    if (0U == unaligned_dst_bit_idx) {
        return prv_copy_to_aligned_dst(p_dst, p_src, n_bytes);
    }

    temp_word_a = (p_src_word++)->word;
    temp_word_b = p_dst_word->word;

    if (unaligned_dst_bit_num <= unaligned_src_bit_num) {
        reminder_bit_num = unaligned_src_bit_num - unaligned_dst_bit_num;

        temp_word_a = SHL(temp_word_a, reminder_bit_num);
        temp_word_a = AND(temp_word_a, SHL(WORD_MAX, unaligned_dst_bit_idx));
        temp_word_b = AND(temp_word_b, INV(SHL(WORD_MAX, unaligned_dst_bit_idx)));

        p_dst_word->word = OR(temp_word_a, temp_word_b);

        return prv_copy_to_aligned_dst((rw uint8_t rw_ptr)p_dst + unaligned_dst_byte_num,
                                       (ro uint8_t rw_ptr)p_src + unaligned_dst_byte_num, n_bytes - unaligned_dst_byte_num);
    }

    reminder_bit_num = unaligned_dst_bit_num - unaligned_src_bit_num;

    temp_word_a = SHR(temp_word_a, reminder_bit_num);
    temp_word_a = AND(temp_word_a, SHL(WORD_MAX, unaligned_dst_bit_idx));
    temp_word_a = OR(temp_word_a, SHL(p_src_word->word, unaligned_src_bit_num + unaligned_dst_bit_idx));

    temp_word_b = AND(temp_word_b, INV(SHL(WORD_MAX, unaligned_dst_bit_idx)));
    p_dst_word->word = OR(temp_word_a, temp_word_b);

    return prv_copy_to_aligned_dst((rw uint8_t rw_ptr)p_dst + unaligned_dst_byte_num,
                                   (ro uint8_t rw_ptr)p_src + unaligned_dst_byte_num, n_bytes - unaligned_dst_byte_num);
}

/* PUBLIC FUNCTIONS **************************************************************************************************************
 ********************************************************************************************************************************/

ze_err_t ZE_MemCopy(rw void ro_ptr p_dst, ro void ro_ptr p_src, uint32_t size) {
    if (0U == size) {
        return ZE_SUCCESS;
    }
    if ((NULL == p_dst) || (NULL == p_src)) {
        return ZE_ERR_INVALID_ARG;
    }
    if (p_dst == p_src) {
        return ZE_SUCCESS;
    }

    #if (ZE_MEM_USE_PLATFORM_ASM == 1)
        (void)memcpy(p_dst, p_src, size);
        return ZE_SUCCESS;
    #else
        return prv_copy_to_unaligned_dst(p_dst, p_src, size);
    #endif
}

ze_err_t ZE_MemSet(rw void ro_ptr p_dst, ro uint8_t preset, uint32_t size) {
    if (0U == size) {
        return ZE_SUCCESS;
    }
    if (NULL == p_dst) {
        return ZE_ERR_INVALID_ARG;
    }

    #if (ZE_MEM_USE_PLATFORM_ASM == 1)
        (void)memset(p_dst, preset, size);
        return ZE_SUCCESS;
    #else
        return prv_assign_to_unaligned_dst(p_dst, preset, size);
    #endif
}

/* EOF ***************************************************************************************************************************
 ********************************************************************************************************************************/