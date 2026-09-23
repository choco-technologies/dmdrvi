#define DMOD_ENABLE_REGISTRATION ON
#define ENABLE_DIF_REGISTRATIONS

#include "dmod_test.h"
#include "dmdrvi.h"

#include <limits.h>

_Static_assert(sizeof(dmdrvi_offset_t) == sizeof(int64_t),
               "dmdrvi offsets must be exactly 64-bit");
_Static_assert(sizeof(dmdrvi_size_t) == sizeof(uint64_t),
               "dmdrvi sizes must be exactly 64-bit");
_Static_assert(sizeof(dmdrvi_ssize_t) == sizeof(int64_t),
               "dmdrvi I/O results must be exactly 64-bit");
_Static_assert(_Generic((dmod_dmdrvi_read_t)0,
                       dmdrvi_ssize_t (*)(dmdrvi_context_t, void*, void*, size_t,
                                          dmdrvi_offset_t): 1,
                       default: 0),
               "dmdrvi_read must use the 2.0 64-bit contract");
_Static_assert(_Generic((dmod_dmdrvi_write_t)0,
                       dmdrvi_ssize_t (*)(dmdrvi_context_t, void*, const void*, size_t,
                                          dmdrvi_offset_t): 1,
                       default: 0),
               "dmdrvi_write must use the 2.0 64-bit contract");

static int contains(const char* text, const char* needle)
{
    for( ; *text != '\0'; ++text )
    {
        const char* left = text;
        const char* right = needle;
        while( *right != '\0' && *left == *right )
        {
            ++left;
            ++right;
        }
        if( *right == '\0' )
        {
            return 1;
        }
    }
    return 0;
}

void dmod_test_setup(void)
{
}

void dmod_test_teardown(void)
{
}

DMOD_TEST_STEP(dmdrvi_v2_signatures_are_registered)
{
    DMOD_TEST_EXPECT_TRUE(contains(dmod_dmdrvi_read_sig, ":2.0/"));
    DMOD_TEST_EXPECT_TRUE(contains(dmod_dmdrvi_write_sig, ":2.0/"));
    DMOD_TEST_EXPECT_TRUE(contains(dmod_dmdrvi_stat_sig, ":2.0/"));
}

DMOD_TEST_STEP(dmdrvi_sizes_round_trip_above_four_gib)
{
    const dmdrvi_size_t expected = (UINT64_C(1) << 32) + UINT64_C(123);
    dmdrvi_stat_t stat = { .size = expected, .mode = 0 };
    dmdrvi_block_info_t info = {
        .logical_block_size = 512,
        .erase_block_size = 4096,
        .block_count = expected,
        .flags = DMDRVI_BLOCK_FLAG_ERASE_SUPPORTED,
    };

    DMOD_TEST_EXPECT_TRUE(stat.size == expected);
    DMOD_TEST_EXPECT_TRUE(info.block_count == expected);
}

DMOD_TEST_STEP(dmdrvi_errors_are_distinct_from_eof)
{
    const dmdrvi_ssize_t eof = 0;
    const dmdrvi_ssize_t error = -1;

    DMOD_TEST_EXPECT_TRUE(error < eof);
}
