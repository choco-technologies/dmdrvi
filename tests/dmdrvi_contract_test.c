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

_Static_assert(DMDRVI_IOCTL_MONITOR_GET_POLICY >= 0x200 &&
               DMDRVI_IOCTL_MONITOR_REFRESH < DMDRVI_IOCTL_CUSTOM_BASE,
               "monitor commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_GFX_GET_INFO >= 0x300 &&
               DMDRVI_IOCTL_GFX_GET_BACKLIGHT < DMDRVI_IOCTL_CUSTOM_BASE,
               "graphics commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_INPUT_GET_INFO >= 0x400 &&
               DMDRVI_IOCTL_INPUT_WAIT_EVENT < DMDRVI_IOCTL_CUSTOM_BASE,
               "input commands must stay in their own standard range");
_Static_assert(sizeof(dmdrvi_input_contact_t) == 8,
               "input contacts must have no padding");
_Static_assert(sizeof(dmdrvi_input_state_t) == 12 + 8 * DMDRVI_INPUT_MAX_CONTACTS,
               "input states must have no padding, so they compare byte by byte");
_Static_assert(sizeof(((dmdrvi_monitor_policy_t*)0)->event_handler) ==
               DMDRVI_MONITOR_HANDLER_NAME_MAX,
               "monitor policy must carry the handler name inline");

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

DMOD_TEST_STEP(dmdrvi_monitor_commands_are_distinct)
{
    const int commands[] = {
        DMDRVI_IOCTL_NET_SET_MAC_ADDR, DMDRVI_IOCTL_NET_GET_MAC_ADDR,
        DMDRVI_IOCTL_NET_GET_LINK_STATUS, DMDRVI_IOCTL_NET_START, DMDRVI_IOCTL_NET_STOP,
        DMDRVI_IOCTL_BLOCK_GET_INFO, DMDRVI_IOCTL_BLOCK_ERASE, DMDRVI_IOCTL_BLOCK_DISCARD,
        DMDRVI_IOCTL_MONITOR_GET_POLICY, DMDRVI_IOCTL_MONITOR_EVENT, DMDRVI_IOCTL_MONITOR_REFRESH,
        DMDRVI_IOCTL_GFX_GET_INFO, DMDRVI_IOCTL_GFX_GET_BACKLIGHT,
        DMDRVI_IOCTL_INPUT_GET_INFO, DMDRVI_IOCTL_INPUT_GET_STATE, DMDRVI_IOCTL_INPUT_WAIT_EVENT,
    };
    const int count = (int)(sizeof(commands) / sizeof(commands[0]));
    int duplicates = 0;

    for( int i = 0; i < count; ++i )
    {
        for( int j = i + 1; j < count; ++j )
        {
            duplicates += (commands[i] == commands[j]) ? 1 : 0;
        }
    }
    DMOD_TEST_EXPECT_EQ(duplicates, 0);
}

DMOD_TEST_STEP(dmdrvi_monitor_policy_holds_longest_handler_name)
{
    dmdrvi_monitor_policy_t policy = { .settle_ms = 50, .poll_interval_ms = 0 };
    const char* name = "a_handler_name_of_31_characters";   /* 31 + terminator */
    int length = 0;

    while( name[length] != '\0' )
    {
        policy.event_handler[length] = name[length];
        ++length;
    }
    policy.event_handler[length] = '\0';

    DMOD_TEST_EXPECT_EQ(length, (int)DMDRVI_MONITOR_HANDLER_NAME_MAX - 1);
    DMOD_TEST_EXPECT_TRUE(contains(policy.event_handler, "31_characters"));
    DMOD_TEST_EXPECT_EQ(policy.settle_ms, 50u);
    DMOD_TEST_EXPECT_EQ(policy.poll_interval_ms, 0u);
}

DMOD_TEST_STEP(dmdrvi_input_states_compare_every_field)
{
    dmdrvi_input_state_t a = { 0 };
    dmdrvi_input_state_t b = { 0 };

    DMOD_TEST_EXPECT_TRUE(dmdrvi_input_state_equal(&a, &b));
    b.contacts[DMDRVI_INPUT_MAX_CONTACTS - 1].size = 1;
    DMOD_TEST_EXPECT_FALSE(dmdrvi_input_state_equal(&a, &b));
    b = a;
    b.buttons = DMDRVI_INPUT_BUTTON_LEFT;
    DMOD_TEST_EXPECT_FALSE(dmdrvi_input_state_equal(&a, &b));
}
