#define DMOD_ENABLE_REGISTRATION ON
#define ENABLE_DIF_REGISTRATIONS

#include <stddef.h>
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
               DMDRVI_IOCTL_GFX_PRESENT < DMDRVI_IOCTL_CUSTOM_BASE,
               "graphics commands must stay in their own standard range");
/* The values of the pixel formats are what the drivers report: a new format goes at the end */
_Static_assert(DMDRVI_GFX_PIXEL_FORMAT_ARGB8888 == 0 && DMDRVI_GFX_PIXEL_FORMAT_RGB888 == 1 &&
               DMDRVI_GFX_PIXEL_FORMAT_RGB565 == 2 && DMDRVI_GFX_PIXEL_FORMAT_ARGB1555 == 3 &&
               DMDRVI_GFX_PIXEL_FORMAT_ARGB4444 == 4 && DMDRVI_GFX_PIXEL_FORMAT_MONO1 == 5 &&
               DMDRVI_GFX_PIXEL_FORMAT_COUNT == 6,
               "pixel formats must keep their values, new ones are appended");
_Static_assert(DMDRVI_IOCTL_INPUT_GET_INFO >= 0x400 &&
               DMDRVI_IOCTL_INPUT_GET_LIGHT < 0x500,
               "input commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_AUDIO_GET_INFO >= 0x500 &&
               DMDRVI_IOCTL_AUDIO_DRAIN < 0x600,
               "audio commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_WIFI_GET_INFO >= 0x700 &&
               DMDRVI_IOCTL_WIFI_SET_ENABLED < 0x800,
               "Wi-Fi commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_MODEM_GET_INFO >= 0x800 &&
               DMDRVI_IOCTL_MODEM_WAIT_EVENT < 0x900,
               "modem commands must stay in their own standard range");
_Static_assert(DMDRVI_MODEM_NUMBER_MAX >= 22 && DMDRVI_MODEM_IMEI_MAX == 16 && DMDRVI_MODEM_PIN_MAX == 9,
               "a number has '+' and up to 20 digits; an IMEI has 15; a PIN or PUK 4 to 8");
_Static_assert(DMDRVI_IOCTL_DEVFS_GET_FRIEND >= 0xF00 &&
               DMDRVI_IOCTL_DEVFS_GET_FRIEND < DMDRVI_IOCTL_CUSTOM_BASE,
               "device file system commands must stay in their own standard range");
/* what a driver written before the layout existed fills is where it was: the new fields are at the end */
_Static_assert(offsetof(dmdrvi_input_info_t, layout) > offsetof(dmdrvi_input_info_t, button_count) &&
               offsetof(dmdrvi_input_info_t, module) == offsetof(dmdrvi_input_info_t, layout) + DMDRVI_INPUT_LAYOUT_MAX &&
               sizeof(dmdrvi_input_info_t) >= offsetof(dmdrvi_input_info_t, module) + DMDRVI_INPUT_MODULE_MAX,
               "the layout and the module of a keyboard are appended to dmdrvi_input_info_t");
_Static_assert(sizeof(dmdrvi_input_contact_t) == 8,
               "input contacts must have no padding");
_Static_assert(sizeof(dmdrvi_devfs_group_t) == DMDRVI_DEVFS_GROUP_NAME_MAX,
               "the group name is the whole structure");
_Static_assert(sizeof(dmdrvi_key_event_t) == 8 && offsetof(dmdrvi_key_event_t, code) == 4,
               "key events must have no padding and a stable layout on 32/64-bit targets");
_Static_assert(sizeof(dmdrvi_key_events_t) == 4 + 8 * DMDRVI_KEY_MAX_EVENTS,
               "key events are read in blocks without padding");
_Static_assert(DMDRVI_INPUT_TYPE_BUTTONS == 2 && DMDRVI_INPUT_TYPE_KEYBOARD == 3,
               "input types must keep their values, new ones are appended");
_Static_assert(sizeof(dmdrvi_input_state_t) == 12 + 8 * DMDRVI_INPUT_MAX_CONTACTS,
               "input states must have no padding, so they compare byte by byte");
_Static_assert(sizeof(((dmdrvi_monitor_policy_t*)0)->event_handler) ==
               DMDRVI_MONITOR_HANDLER_NAME_MAX,
               "monitor policy must carry the handler name inline");

_Static_assert(sizeof(dmdrvi_rtc_time_t) == 16 &&
               offsetof(dmdrvi_rtc_time_t, nanosecond) == 12,
               "RTC calendar layout must be stable across 32/64-bit targets");
_Static_assert(sizeof(dmdrvi_rtc_info_t) == 24 && sizeof(dmdrvi_rtc_status_t) == 8,
               "RTC info and status must use fixed-width fields");
_Static_assert(DMDRVI_IOCTL_RTC_GET_TIME >= 0x600 && DMDRVI_IOCTL_RTC_INVALIDATE < 0x700,
               "RTC commands must stay in their standard category");

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
        DMDRVI_IOCTL_GFX_GET_INFO, DMDRVI_IOCTL_GFX_GET_FRAMEBUFFER, DMDRVI_IOCTL_GFX_SWAP_BUFFERS,
        DMDRVI_IOCTL_GFX_WAIT_VSYNC, DMDRVI_IOCTL_GFX_FILL_RECT, DMDRVI_IOCTL_GFX_SET_DISPLAY_ENABLED,
        DMDRVI_IOCTL_GFX_GET_DISPLAY_ENABLED, DMDRVI_IOCTL_GFX_SET_BACKLIGHT, DMDRVI_IOCTL_GFX_GET_BACKLIGHT,
        DMDRVI_IOCTL_GFX_PRESENT,
        DMDRVI_IOCTL_INPUT_GET_INFO, DMDRVI_IOCTL_INPUT_GET_STATE, DMDRVI_IOCTL_INPUT_WAIT_EVENT,
        DMDRVI_IOCTL_INPUT_KEY_READ, DMDRVI_IOCTL_INPUT_SET_LIGHT, DMDRVI_IOCTL_INPUT_GET_LIGHT,
        DMDRVI_IOCTL_AUDIO_GET_INFO, DMDRVI_IOCTL_AUDIO_CONFIGURE,
        DMDRVI_IOCTL_AUDIO_SET_VOLUME, DMDRVI_IOCTL_AUDIO_SET_MUTE,
        DMDRVI_IOCTL_AUDIO_GET_STREAM, DMDRVI_IOCTL_AUDIO_START, DMDRVI_IOCTL_AUDIO_STOP,
        DMDRVI_IOCTL_AUDIO_GET_STATUS, DMDRVI_IOCTL_AUDIO_SET_TIMEOUT,
        DMDRVI_IOCTL_AUDIO_GET_TIMEOUT, DMDRVI_IOCTL_AUDIO_DRAIN,
        DMDRVI_IOCTL_DEVFS_GET_FRIEND, DMDRVI_IOCTL_DEVFS_GET_GROUP,
        DMDRVI_IOCTL_RTC_GET_TIME, DMDRVI_IOCTL_RTC_SET_TIME,
        DMDRVI_IOCTL_RTC_GET_STATUS, DMDRVI_IOCTL_RTC_GET_INFO, DMDRVI_IOCTL_RTC_INVALIDATE,
        DMDRVI_IOCTL_WIFI_GET_INFO, DMDRVI_IOCTL_WIFI_GET_STATUS, DMDRVI_IOCTL_WIFI_CONNECT,
        DMDRVI_IOCTL_WIFI_DISCONNECT, DMDRVI_IOCTL_WIFI_SCAN, DMDRVI_IOCTL_WIFI_SET_ENABLED,
        DMDRVI_IOCTL_MODEM_GET_INFO, DMDRVI_IOCTL_MODEM_GET_STATUS, DMDRVI_IOCTL_MODEM_SET_ENABLED,
        DMDRVI_IOCTL_MODEM_SET_PIN, DMDRVI_IOCTL_MODEM_AT, DMDRVI_IOCTL_MODEM_SMS_SEND,
        DMDRVI_IOCTL_MODEM_SMS_LIST, DMDRVI_IOCTL_MODEM_SMS_READ, DMDRVI_IOCTL_MODEM_SMS_DELETE,
        DMDRVI_IOCTL_MODEM_WAIT_EVENT,
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

/* The key codes are the USB HID usages, a modifier is a key like the others, an event carries no state */
DMOD_TEST_STEP(dmdrvi_key_events_describe_keys)
{
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_A + 25u, DMDRVI_KEY_Z);
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_1 + 9u, DMDRVI_KEY_0);
    DMOD_TEST_EXPECT_TRUE(DMDRVI_KEY_UP > DMDRVI_KEY_RIGHT && DMDRVI_KEY_UP < DMDRVI_KEY_VENDOR_BASE);
    DMOD_TEST_EXPECT_TRUE(DMDRVI_KEY_LEFT_SHIFT < DMDRVI_KEY_VENDOR_BASE);
    /* the codes of the keys a USB keyboard has none of are the same for every driver: they must not move */
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_SYM, 0x100u);
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_MIC, 0x101u);
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_DOLLAR, 0x102u);

    dmdrvi_key_events_t block = { 0 };
    DMOD_TEST_EXPECT_EQ(block.count, 0u);               /* nothing waits: count 0, not an error */
    block.events[0] = (dmdrvi_key_event_t){ 10u, DMDRVI_KEY_LEFT_SHIFT, DMDRVI_KEY_ACTION_DOWN, 0 };
    block.events[1] = (dmdrvi_key_event_t){ 20u, DMDRVI_KEY_A, DMDRVI_KEY_ACTION_DOWN, 0 };
    block.events[DMDRVI_KEY_MAX_EVENTS - 1].code = DMDRVI_KEY_VENDOR_BASE;
    block.count = 2;
    DMOD_TEST_EXPECT_EQ(block.events[1].code, DMDRVI_KEY_A);
    DMOD_TEST_EXPECT_EQ(block.events[DMDRVI_KEY_MAX_EVENTS - 1].code, DMDRVI_KEY_VENDOR_BASE);
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_ACTION_OVERFLOW, 2);
}
