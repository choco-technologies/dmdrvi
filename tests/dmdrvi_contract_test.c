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
               DMDRVI_IOCTL_WIFI_SCAN < 0x800,
               "Wi-Fi commands must stay in their own standard range");
_Static_assert(DMDRVI_IOCTL_DEVFS_GET_FRIEND >= 0xF00 &&
               DMDRVI_IOCTL_DEVFS_GET_FRIEND < DMDRVI_IOCTL_CUSTOM_BASE,
               "device file system commands must stay in their own standard range");
_Static_assert(sizeof(dmdrvi_input_contact_t) == 8,
               "input contacts must have no padding");
_Static_assert(sizeof(dmdrvi_key_event_t) == 8 && offsetof(dmdrvi_key_event_t, code) == 4,
               "key events must have no padding and a stable layout on 32/64-bit targets");
_Static_assert(sizeof(dmdrvi_key_events_t) == 4 + 8 * DMDRVI_KEY_MAX_EVENTS,
               "key events are read in blocks without padding");
_Static_assert(sizeof(dmdrvi_key_config_t) == 8,
               "key settings must have no padding");
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
        DMDRVI_IOCTL_INPUT_KEY_READ, DMDRVI_IOCTL_INPUT_KEY_LISTEN, DMDRVI_IOCTL_INPUT_KEY_GET_CONFIG,
        DMDRVI_IOCTL_INPUT_KEY_SET_CONFIG, DMDRVI_IOCTL_INPUT_SET_LIGHT, DMDRVI_IOCTL_INPUT_GET_LIGHT,
        DMDRVI_IOCTL_AUDIO_GET_INFO, DMDRVI_IOCTL_AUDIO_CONFIGURE,
        DMDRVI_IOCTL_AUDIO_SET_VOLUME, DMDRVI_IOCTL_AUDIO_SET_MUTE,
        DMDRVI_IOCTL_AUDIO_GET_STREAM, DMDRVI_IOCTL_AUDIO_START, DMDRVI_IOCTL_AUDIO_STOP,
        DMDRVI_IOCTL_AUDIO_GET_STATUS, DMDRVI_IOCTL_AUDIO_SET_TIMEOUT,
        DMDRVI_IOCTL_AUDIO_GET_TIMEOUT, DMDRVI_IOCTL_AUDIO_DRAIN,
        DMDRVI_IOCTL_DEVFS_GET_FRIEND,
        DMDRVI_IOCTL_RTC_GET_TIME, DMDRVI_IOCTL_RTC_SET_TIME,
        DMDRVI_IOCTL_RTC_GET_STATUS, DMDRVI_IOCTL_RTC_GET_INFO, DMDRVI_IOCTL_RTC_INVALIDATE,
        DMDRVI_IOCTL_WIFI_GET_INFO, DMDRVI_IOCTL_WIFI_GET_STATUS, DMDRVI_IOCTL_WIFI_CONNECT,
        DMDRVI_IOCTL_WIFI_DISCONNECT, DMDRVI_IOCTL_WIFI_SCAN,
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

static int heard;

static void count_key(void* context, const dmdrvi_key_event_t* event)
{
    *(int*)context += (event->action == DMDRVI_KEY_ACTION_DOWN);
    heard++;
}

/* The key codes are the USB HID usages, the modifiers are separate bits, a listener is a handler and its context */
DMOD_TEST_STEP(dmdrvi_key_events_describe_keys)
{
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_A + 25u, DMDRVI_KEY_Z);
    DMOD_TEST_EXPECT_EQ(DMDRVI_KEY_1 + 9u, DMDRVI_KEY_0);
    DMOD_TEST_EXPECT_TRUE(DMDRVI_KEY_UP > DMDRVI_KEY_RIGHT && DMDRVI_KEY_UP < DMDRVI_KEY_VENDOR_BASE);

    const uint8_t mods[] = { DMDRVI_KEY_MOD_SHIFT, DMDRVI_KEY_MOD_CTRL, DMDRVI_KEY_MOD_ALT,
                             DMDRVI_KEY_MOD_SYM, DMDRVI_KEY_MOD_CAPS };
    uint8_t all = 0;
    for( int i = 0; i < 5; ++i )
    {
        DMOD_TEST_EXPECT_EQ(all & mods[i], 0);          /* one bit each */
        all |= mods[i];
    }

    int downs = 0;
    dmdrvi_key_listener_t listener = { count_key, &downs };
    dmdrvi_key_event_t down = { 10u, DMDRVI_KEY_A, DMDRVI_KEY_ACTION_DOWN, DMDRVI_KEY_MOD_SHIFT };
    dmdrvi_key_event_t up = { 20u, DMDRVI_KEY_A, DMDRVI_KEY_ACTION_UP, DMDRVI_KEY_MOD_SHIFT };
    listener.handler(listener.context, &down);
    listener.handler(listener.context, &up);
    DMOD_TEST_EXPECT_EQ(downs, 1);
    DMOD_TEST_EXPECT_EQ(heard, 2);

    dmdrvi_key_events_t block = { 0 };
    DMOD_TEST_EXPECT_EQ(block.count, 0u);               /* nothing waits: count 0, not an error */
    block.events[DMDRVI_KEY_MAX_EVENTS - 1].code = DMDRVI_KEY_VENDOR_BASE;
    DMOD_TEST_EXPECT_EQ(block.events[DMDRVI_KEY_MAX_EVENTS - 1].code, DMDRVI_KEY_VENDOR_BASE);
}
