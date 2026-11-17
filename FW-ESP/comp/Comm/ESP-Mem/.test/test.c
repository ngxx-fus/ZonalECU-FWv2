/********************************************************************************************************************************
 * @file test.c
 * @brief Standalone x64 Fast-Test Suite for Memory Copy Algorithm ([[Mem]])
 ********************************************************************************************************************************/

/* INCLUDES *********************************************************************************************************************
 ********************************************************************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

/* TYPE DEFINITIONS & MOCK MACROS (FOR STANDALONE X64 EXECUTION) ****************************************************************
 ********************************************************************************************************************************/
#define ro const
#define rw
#define ro_ptr *const
#define rw_ptr *

typedef int32_t ze_err_t;
#define ZE_SUCCESS              (0)
#define ZE_ERR_INVALID_ARG      (-1)

typedef union ze_word_t {
    uint32_t word;
    uint8_t  byte[4];
} ze_word_t;
typedef ze_word_t* ze_wordptr_t;

#define BYTE_INDEX(addr)        ((uintptr_t)(addr) & 3U)
#define BIT_INDEX(addr)         (((uintptr_t)(addr) & 3U) << 3U)
#define WORD_BASE(addr)         ((uintptr_t)(addr) & ~3UL)
#define WORD_WIDTH              (0x20U)
#define WORD_MAX                (0xFFFFFFFFU)
#define SPEED_UP_THRES          12U

#define SHL(val, shift)         ((uint32_t)(val) << (shift))
#define SHR(val, shift)         ((uint32_t)(val) >> (shift))
#define AND(a, b)               ((uint32_t)(a) & (uint32_t)(b))
#define OR(a, b)                ((uint32_t)(a) | (uint32_t)(b))
#define INV(a)                  (~(uint32_t)(a))

/* CORE ALGORITHM UNDER TEST ****************************************************************************************************
 ********************************************************************************************************************************/
static inline ze_err_t prv_copy_to_aligned_dst(rw void ro_ptr p_dst, ro void ro_ptr p_src, ro uint32_t n_bytes) {
    if (0 != BYTE_INDEX(p_dst)) {
        return ZE_ERR_INVALID_ARG;
    }

    ro ze_word_t rw_ptr p_src_word          =   (ze_wordptr_t) WORD_BASE(p_src);
    rw ze_word_t rw_ptr p_dst_word          =   (ze_wordptr_t) WORD_BASE(p_dst);
    ro uint32_t         unaligned_bit_idx   =   BIT_INDEX(p_src);
    ro uint32_t         unaligned_bit_num   =   AND(0x20U - unaligned_bit_idx, 0x1F);
    ro uint32_t         borrowed_bit_num    =   unaligned_bit_idx;
    rw uint32_t         temp_word_a         =   0U;
    rw uint32_t         temp_word_b         =   0U;
    rw uint32_t         n_bits              =   SHL(n_bytes, 3);

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
        temp_word_a      = AND(temp_word_a, INV(SHL(WORD_MAX, n_bits)));
        temp_word_b      = p_dst_word->word;
        temp_word_b      = AND(temp_word_b, SHL(WORD_MAX, n_bits));
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

    ro ze_word_t rw_ptr p_src_word              =   (ze_wordptr_t) WORD_BASE(p_src);
    rw ze_word_t rw_ptr p_dst_word              =   (ze_wordptr_t) WORD_BASE(p_dst);
    ro uint32_t         unaligned_dst_bit_idx   =   BIT_INDEX(p_dst);
    ro uint32_t         unaligned_dst_bit_num   =   WORD_WIDTH - unaligned_dst_bit_idx;
    ro uint32_t         unaligned_dst_byte_num  =   SHR(unaligned_dst_bit_num, 3U);
    ro uint32_t         unaligned_src_bit_idx   =   BIT_INDEX(p_src);
    ro uint32_t         unaligned_src_bit_num   =   WORD_WIDTH - unaligned_src_bit_idx;
    rw uint32_t         temp_word_a             = 0U;
    rw uint32_t         temp_word_b             = 0U;
    rw uint32_t         reminder_bit_num        = 0U;

    if (0U == unaligned_dst_bit_idx) {
        return prv_copy_to_aligned_dst(p_dst, p_src, n_bytes);
    }

    temp_word_a         = (p_src_word++)->word;
    temp_word_b         = p_dst_word->word;

    if (unaligned_dst_bit_num <= unaligned_src_bit_num) {
        reminder_bit_num    = unaligned_src_bit_num - unaligned_dst_bit_num;

        temp_word_a         = SHL(temp_word_a,  reminder_bit_num);
        temp_word_a         = AND(temp_word_a,      SHL(WORD_MAX,     unaligned_dst_bit_idx));
        temp_word_b         = AND(temp_word_b,  INV(SHL(WORD_MAX,     unaligned_dst_bit_idx)));

        p_dst_word->word    = OR(temp_word_a, temp_word_b);

        return prv_copy_to_aligned_dst(
            (rw uint8_t rw_ptr)p_dst + unaligned_dst_byte_num, 
            (ro uint8_t rw_ptr)p_src + unaligned_dst_byte_num, 
            n_bytes - unaligned_dst_byte_num
        );
    }

    reminder_bit_num    = unaligned_dst_bit_num - unaligned_src_bit_num;

    temp_word_a         = SHR(temp_word_a, reminder_bit_num);
    temp_word_a         = AND(temp_word_a,     SHL(WORD_MAX, unaligned_dst_bit_idx));
    temp_word_a         = OR(temp_word_a,      SHL(p_src_word->word, unaligned_src_bit_num + unaligned_dst_bit_idx));

    temp_word_b         = AND(temp_word_b,  INV(SHL(WORD_MAX,     unaligned_dst_bit_idx)));
    p_dst_word->word    = OR(temp_word_a, temp_word_b);

    return prv_copy_to_aligned_dst(
        (rw uint8_t rw_ptr)p_dst + unaligned_dst_byte_num, 
        (ro uint8_t rw_ptr)p_src + unaligned_dst_byte_num, 
        n_bytes - unaligned_dst_byte_num
    );
}

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
    return prv_copy_to_unaligned_dst(p_dst, p_src, size);
}

/* TEST HARNESS DEFINITIONS *****************************************************************************************************
 ********************************************************************************************************************************/
#define CANARY_PRE              (0xAAU)
#define CANARY_POST             (0x55U)
#define CANARY_INIT             (0xEEU)
#define GUARD_SIZE              (16U)
#define BUFFER_SIZE             (2048U)

static uint8_t g_src_pool[BUFFER_SIZE];
static uint8_t g_dst_pool[BUFFER_SIZE];
static uint8_t g_ref_pool[BUFFER_SIZE];

static uint32_t g_tests_passed = 0;
static uint32_t g_tests_failed = 0;

static bool prv_verify_copy(uint32_t dst_offset, uint32_t src_offset, uint32_t size) {
    uint8_t *dst_start = g_dst_pool + GUARD_SIZE + dst_offset;
    uint8_t *src_start = g_src_pool + GUARD_SIZE + src_offset;
    uint8_t *ref_start = g_ref_pool + GUARD_SIZE + dst_offset;

    memset(g_dst_pool, CANARY_INIT, BUFFER_SIZE);
    memset(g_ref_pool, CANARY_INIT, BUFFER_SIZE);
    memset(g_dst_pool, CANARY_PRE, GUARD_SIZE + dst_offset);
    memset(g_ref_pool, CANARY_PRE, GUARD_SIZE + dst_offset);
    memset(dst_start + size, CANARY_POST, GUARD_SIZE);
    memset(ref_start + size, CANARY_POST, GUARD_SIZE);

    memcpy(ref_start, src_start, size);
    ze_err_t err = ZE_MemCopy(dst_start, src_start, size);
    if (err != ZE_SUCCESS) {
        printf("    [FAIL] ZE_MemCopy error: %d (dst_off=%u, src_off=%u, size=%u)\n",
               err, dst_offset, src_offset, size);
        return false;
    }

    /* 1. Verify pre-guard canary */
    for (uint32_t i = 0; i < GUARD_SIZE + dst_offset; i++) {
        if (g_dst_pool[i] != CANARY_PRE) {
            printf("    [FAIL] Pre-guard canary corrupted at index %u (got 0x%02X, expected 0x%02X)\n",
                   i, g_dst_pool[i], CANARY_PRE);
            return false;
        }
    }

    /* 2. Verify payload matches reference */
    if (memcmp(dst_start, ref_start, size) != 0) {
        printf("    [FAIL] Payload mismatch for size=%u (dst_off=%u, src_off=%u)\n",
               size, dst_offset, src_offset);
        for (uint32_t i = 0; i < size; i++) {
            if (dst_start[i] != ref_start[i]) {
                printf("        diff at byte %u: got 0x%02X, expected 0x%02X\n",
                       i, dst_start[i], ref_start[i]);
                if (i >= 8) {
                    printf("        ... (remaining diffs truncated)\n");
                    break;
                }
            }
        }
        return false;
    }

    /* 3. Verify post-guard canary */
    for (uint32_t i = 0; i < GUARD_SIZE; i++) {
        if (dst_start[size + i] != CANARY_POST) {
            printf("    [FAIL] Post-guard canary corrupted at offset +%u (got 0x%02X, expected 0x%02X)\n",
                   i, dst_start[size + i], CANARY_POST);
            return false;
        }
    }

    return true;
}

/* 16 TEST CASES SPECIFICATION & IMPLEMENTATION *********************************************************************************
 ********************************************************************************************************************************/

/** TESTCASE 01: Aligned dest; Aligned src; Copy size < 12
 *  DESC: Both buffers 4-byte aligned; Small payload size below speedup threshold (0, 1, 2, 3, 4, 7, 10, 11 bytes).
 *  EXPECTED: Fast byte loop copy or direct word write. Canaries fully preserved. Returns ZE_SUCCESS.
 */
static bool test_tc01_aligned_dst_aligned_src_small(void) {
    const uint32_t sizes[] = {0, 1, 2, 3, 4, 7, 8, 10, 11};
    for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        if (!prv_verify_copy(0, 0, sizes[s])) return false;
    }
    return true;
}

/** TESTCASE 02: Aligned dest; Aligned src; Copy size >= 12
 *  DESC: Both buffers 4-byte aligned; Medium payload size (12, 13, 16, 24, 32, 47, 63 bytes).
 *  EXPECTED: Full 32-bit word bulk copy with tail remainder. No underflow or overrun. Returns ZE_SUCCESS.
 */
static bool test_tc02_aligned_dst_aligned_src_medium(void) {
    const uint32_t sizes[] = {12, 13, 16, 17, 24, 32, 47, 63};
    for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        if (!prv_verify_copy(0, 0, sizes[s])) return false;
    }
    return true;
}

/** TESTCASE 03: Aligned dest; Aligned src; Copy size >= 64
 *  DESC: Both buffers 4-byte aligned; Large payload size (64, 65, 80, 100, 128, 199, 255 bytes).
 *  EXPECTED: Consecutive 32-bit word loop execution. Boundary and tail masks verified. Returns ZE_SUCCESS.
 */
static bool test_tc03_aligned_dst_aligned_src_large(void) {
    const uint32_t sizes[] = {64, 65, 80, 100, 128, 199, 255};
    for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        if (!prv_verify_copy(0, 0, sizes[s])) return false;
    }
    return true;
}

/** TESTCASE 04: Aligned dest; Aligned src; Copy size >= 256
 *  DESC: Both buffers 4-byte aligned; Bulk payload size (256, 257, 300, 512, 768, 1024 bytes).
 *  EXPECTED: Sustained high-throughput 32-bit word copying. Maximum payload integrity. Returns ZE_SUCCESS.
 */
static bool test_tc04_aligned_dst_aligned_src_bulk(void) {
    const uint32_t sizes[] = {256, 257, 300, 512, 768, 1024};
    for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
        if (!prv_verify_copy(0, 0, sizes[s])) return false;
    }
    return true;
}

/** TESTCASE 05: Aligned dest; Unaligned src; Copy size < 12
 *  DESC: Destination aligned (offset 0), Source unaligned (offsets 1, 2, 3); Size < 12 bytes.
 *  EXPECTED: Fast byte loop or single word barrel shift. Pre/post canaries intact. Returns ZE_SUCCESS.
 */
static bool test_tc05_aligned_dst_unaligned_src_small(void) {
    const uint32_t sizes[] = {0, 1, 2, 3, 5, 8, 9, 10, 11};
    for (uint32_t so = 1; so <= 3; so++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(0, so, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 06: Aligned dest; Unaligned src; Copy size >= 12
 *  DESC: Destination aligned (offset 0), Source unaligned (offsets 1, 2, 3); Size >= 12 bytes.
 *  EXPECTED: Barrel shifter aligns source stream with (SHR | SHL). Tail masked correctly. Returns ZE_SUCCESS.
 */
static bool test_tc06_aligned_dst_unaligned_src_medium(void) {
    const uint32_t sizes[] = {12, 13, 15, 16, 23, 35, 48, 63};
    for (uint32_t so = 1; so <= 3; so++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(0, so, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 07: Aligned dest; Unaligned src; Copy size >= 64
 *  DESC: Destination aligned (offset 0), Source unaligned (offsets 1, 2, 3); Size >= 64 bytes.
 *  EXPECTED: Multi-word barrel shifter stream copy without register drift. Returns ZE_SUCCESS.
 */
static bool test_tc07_aligned_dst_unaligned_src_large(void) {
    const uint32_t sizes[] = {64, 65, 80, 100, 128, 175, 255};
    for (uint32_t so = 1; so <= 3; so++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(0, so, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 08: Aligned dest; Unaligned src; Copy size >= 256
 *  DESC: Destination aligned (offset 0), Source unaligned (offsets 1, 2, 3); Size >= 256 bytes.
 *  EXPECTED: Large buffer shift performance without memory corruption or leak. Returns ZE_SUCCESS.
 */
static bool test_tc08_aligned_dst_unaligned_src_bulk(void) {
    const uint32_t sizes[] = {256, 259, 350, 512, 777, 1024};
    for (uint32_t so = 1; so <= 3; so++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(0, so, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 09: Unaligned dest; Aligned src; Copy size < 12
 *  DESC: Destination unaligned (offsets 1, 2, 3), Source aligned (offset 0); Size < 12 bytes.
 *  EXPECTED: Head byte protection. Destination initial bytes preserved. Returns ZE_SUCCESS.
 */
static bool test_tc09_unaligned_dst_aligned_src_small(void) {
    const uint32_t sizes[] = {0, 1, 2, 3, 4, 7, 9, 10, 11};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(doff, 0, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 10: Unaligned dest; Aligned src; Copy size >= 12
 *  DESC: Destination unaligned (offsets 1, 2, 3), Source aligned (offset 0); Size >= 12 bytes.
 *  EXPECTED: Head aligned first to 4-byte boundary, then bulk copy via word loop. Returns ZE_SUCCESS.
 */
static bool test_tc10_unaligned_dst_aligned_src_medium(void) {
    const uint32_t sizes[] = {12, 13, 16, 17, 25, 33, 49, 63};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(doff, 0, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 11: Unaligned dest; Aligned src; Copy size >= 64
 *  DESC: Destination unaligned (offsets 1, 2, 3), Source aligned (offset 0); Size >= 64 bytes.
 *  EXPECTED: Accurate head alignment followed by multi-word body and tail masking. Returns ZE_SUCCESS.
 */
static bool test_tc11_unaligned_dst_aligned_src_large(void) {
    const uint32_t sizes[] = {64, 65, 88, 120, 128, 200, 255};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(doff, 0, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 12: Unaligned dest; Aligned src; Copy size >= 256
 *  DESC: Destination unaligned (offsets 1, 2, 3), Source aligned (offset 0); Size >= 256 bytes.
 *  EXPECTED: Long payload integrity with initial destination offset. Returns ZE_SUCCESS.
 */
static bool test_tc12_unaligned_dst_aligned_src_bulk(void) {
    const uint32_t sizes[] = {256, 258, 333, 512, 800, 1024};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
            if (!prv_verify_copy(doff, 0, sizes[s])) return false;
        }
    }
    return true;
}

/** TESTCASE 13: Unaligned dest; Unaligned src; Copy size < 12
 *  DESC: Both buffers unaligned (all 9 offset pairs: 1..3 x 1..3); Size < 12 bytes.
 *  EXPECTED: Fast byte loop handles arbitrary pointer offsets safely. Returns ZE_SUCCESS.
 */
static bool test_tc13_unaligned_dst_unaligned_src_small(void) {
    const uint32_t sizes[] = {0, 1, 2, 3, 5, 8, 9, 10, 11};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (uint32_t so = 1; so <= 3; so++) {
            for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
                if (!prv_verify_copy(doff, so, sizes[s])) return false;
            }
        }
    }
    return true;
}

/** TESTCASE 14: Unaligned dest; Unaligned src; Copy size >= 12
 *  DESC: Both buffers unaligned (all 9 offset pairs: 1..3 x 1..3); Size >= 12 bytes.
 *  EXPECTED: Head byte alignment delegates to aligned dst with source barrel shifter. Returns ZE_SUCCESS.
 */
static bool test_tc14_unaligned_dst_unaligned_src_medium(void) {
    const uint32_t sizes[] = {12, 13, 16, 17, 26, 37, 50, 63};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (uint32_t so = 1; so <= 3; so++) {
            for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
                if (!prv_verify_copy(doff, so, sizes[s])) return false;
            }
        }
    }
    return true;
}

/** TESTCASE 15: Unaligned dest; Unaligned src; Copy size >= 64
 *  DESC: Both buffers unaligned (all 9 offset pairs: 1..3 x 1..3); Size >= 64 bytes.
 *  EXPECTED: Full multi-stage copy (Head Align -> Word Barrel Shift -> Tail Mask). Returns ZE_SUCCESS.
 */
static bool test_tc15_unaligned_dst_unaligned_src_large(void) {
    const uint32_t sizes[] = {64, 65, 85, 110, 128, 192, 255};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (uint32_t so = 1; so <= 3; so++) {
            for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
                if (!prv_verify_copy(doff, so, sizes[s])) return false;
            }
        }
    }
    return true;
}

/** TESTCASE 16: Unaligned dest; Unaligned src; Copy size >= 256
 *  DESC: Both buffers unaligned (all 9 offset pairs: 1..3 x 1..3); Bulk size >= 256 bytes.
 *  EXPECTED: Complete data consistency across 1KB buffers for all unalignment permutations. Returns ZE_SUCCESS.
 */
static bool test_tc16_unaligned_dst_unaligned_src_bulk(void) {
    const uint32_t sizes[] = {256, 257, 345, 512, 700, 1024};
    for (uint32_t doff = 1; doff <= 3; doff++) {
        for (uint32_t so = 1; so <= 3; so++) {
            for (size_t s = 0; s < sizeof(sizes)/sizeof(sizes[0]); s++) {
                if (!prv_verify_copy(doff, so, sizes[s])) return false;
            }
        }
    }
    return true;
}

/* MAIN TEST RUNNER *************************************************************************************************************
 ********************************************************************************************************************************/
int main(void) {
    /* Initialize source pool with pseudo-random pattern */
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        g_src_pool[i] = (uint8_t)((i * 7U + 0x3Bu) & 0xFFU);
    }

    printf("================================================================================\n");
    printf("                  ZONAL ECU FW-ESP MEMORY COPY VERIFICATION TEST                \n");
    printf("================================================================================\n\n");

    typedef struct {
        const char *name;
        bool (*func)(void);
    } test_entry_t;

    const test_entry_t test_matrix[16] = {
        {"TC01: Dst ALIGNED,   Src ALIGNED,   Size < 12 ", test_tc01_aligned_dst_aligned_src_small},
        {"TC02: Dst ALIGNED,   Src ALIGNED,   Size >= 12", test_tc02_aligned_dst_aligned_src_medium},
        {"TC03: Dst ALIGNED,   Src ALIGNED,   Size >= 64", test_tc03_aligned_dst_aligned_src_large},
        {"TC04: Dst ALIGNED,   Src ALIGNED,   Size >= 256", test_tc04_aligned_dst_aligned_src_bulk},
        {"TC05: Dst ALIGNED,   Src UNALIGNED, Size < 12 ", test_tc05_aligned_dst_unaligned_src_small},
        {"TC06: Dst ALIGNED,   Src UNALIGNED, Size >= 12", test_tc06_aligned_dst_unaligned_src_medium},
        {"TC07: Dst ALIGNED,   Src UNALIGNED, Size >= 64", test_tc07_aligned_dst_unaligned_src_large},
        {"TC08: Dst ALIGNED,   Src UNALIGNED, Size >= 256", test_tc08_aligned_dst_unaligned_src_bulk},
        {"TC09: Dst UNALIGNED, Src ALIGNED,   Size < 12 ", test_tc09_unaligned_dst_aligned_src_small},
        {"TC10: Dst UNALIGNED, Src ALIGNED,   Size >= 12", test_tc10_unaligned_dst_aligned_src_medium},
        {"TC11: Dst UNALIGNED, Src ALIGNED,   Size >= 64", test_tc11_unaligned_dst_aligned_src_large},
        {"TC12: Dst UNALIGNED, Src ALIGNED,   Size >= 256", test_tc12_unaligned_dst_aligned_src_bulk},
        {"TC13: Dst UNALIGNED, Src UNALIGNED, Size < 12 ", test_tc13_unaligned_dst_unaligned_src_small},
        {"TC14: Dst UNALIGNED, Src UNALIGNED, Size >= 12", test_tc14_unaligned_dst_unaligned_src_medium},
        {"TC15: Dst UNALIGNED, Src UNALIGNED, Size >= 64", test_tc15_unaligned_dst_unaligned_src_large},
        {"TC16: Dst UNALIGNED, Src UNALIGNED, Size >= 256", test_tc16_unaligned_dst_unaligned_src_bulk},
    };

    for (int i = 0; i < 16; i++) {
        printf("[TEST %02d/16] %s ... ", i + 1, test_matrix[i].name);
        fflush(stdout);
        if (test_matrix[i].func()) {
            printf("PASSED\n");
            g_tests_passed++;
        } else {
            printf("FAILED\n");
            g_tests_failed++;
        }
    }

    printf("\n================================================================================\n");
    printf("TEST SUMMARY: %u / 16 PASSED, %u FAILED\n", g_tests_passed, g_tests_failed);
    printf("================================================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
