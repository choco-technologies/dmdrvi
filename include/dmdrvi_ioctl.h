#ifndef DMDRVI_IOCTL_H
#define DMDRVI_IOCTL_H

#include <stdint.h>
#include <stdbool.h>
#include "dmdrvi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Length in bytes of an Ethernet MAC address
 */
#define DMDRVI_NET_MAC_ADDR_LEN    6

/**
 * @brief MAC address type
 */
typedef struct
{
    uint8_t addr[DMDRVI_NET_MAC_ADDR_LEN];    ///< MAC address bytes
} dmdrvi_net_mac_addr_t;

/**
 * @brief Network link status
 */
typedef enum
{
    DMDRVI_NET_LINK_DOWN = 0,    ///< Link is down (disconnected)
    DMDRVI_NET_LINK_UP   = 1,    ///< Link is up (connected)
} dmdrvi_net_link_status_t;

/**
 * @brief Set the device MAC address
 *
 * arg: const dmdrvi_net_mac_addr_t* - MAC address to set (input)
 */
#define DMDRVI_IOCTL_NET_SET_MAC_ADDR       0x01

/**
 * @brief Get the device MAC address
 *
 * arg: dmdrvi_net_mac_addr_t* - buffer to receive the MAC address (output)
 */
#define DMDRVI_IOCTL_NET_GET_MAC_ADDR       0x02

/**
 * @brief Get the current link status
 *
 * arg: dmdrvi_net_link_status_t* - buffer to receive the link status (output)
 */
#define DMDRVI_IOCTL_NET_GET_LINK_STATUS    0x03

/**
 * @brief Start the network interface (begin packet reception/transmission)
 *
 * arg: none
 */
#define DMDRVI_IOCTL_NET_START               0x04

/**
 * @brief Stop the network interface
 *
 * arg: none
 */
#define DMDRVI_IOCTL_NET_STOP                0x05

/** Block device is read-only. */
#define DMDRVI_BLOCK_FLAG_READ_ONLY          (1u << 0)

/** Block device can be removed while the system is running. */
#define DMDRVI_BLOCK_FLAG_REMOVABLE          (1u << 1)

/** Block device implements DMDRVI_IOCTL_BLOCK_ERASE. */
#define DMDRVI_BLOCK_FLAG_ERASE_SUPPORTED    (1u << 2)

/** Block device implements DMDRVI_IOCTL_BLOCK_DISCARD. */
#define DMDRVI_BLOCK_FLAG_DISCARD_SUPPORTED  (1u << 3)

/** Standard geometry and capabilities returned by a block device. */
typedef struct
{
    uint32_t logical_block_size;  /**< Addressable block size in bytes. */
    uint32_t erase_block_size;    /**< Minimum erase unit in bytes, or zero. */
    dmdrvi_size_t block_count;    /**< Number of logical blocks. */
    uint32_t flags;               /**< DMDRVI_BLOCK_FLAG_* capability bits. */
} dmdrvi_block_info_t;

/** Byte range used by erase and discard controls. */
typedef struct
{
    dmdrvi_offset_t offset;  /**< Non-negative byte offset. */
    dmdrvi_size_t length;    /**< Range length in bytes. */
} dmdrvi_block_range_t;

/**
 * Read block geometry and capabilities.
 *
 * arg: dmdrvi_block_info_t* - output buffer
 */
#define DMDRVI_IOCTL_BLOCK_GET_INFO          0x100

/**
 * Physically erase an aligned byte range. Successful completion means the
 * operation has completed on the medium. The post-erase byte value is
 * device-specific.
 *
 * arg: const dmdrvi_block_range_t* - input range
 */
#define DMDRVI_IOCTL_BLOCK_ERASE             0x101

/**
 * Inform the device that an aligned byte range is no longer in use. Reads of
 * discarded data are unspecified until it is written again.
 *
 * arg: const dmdrvi_block_range_t* - input range
 */
#define DMDRVI_IOCTL_BLOCK_DISCARD           0x102

/*
 * Monitor commands - class-independent (a USB host controller node is not a
 * block device). They let a driver have work done over time - presence
 * detection, hot-plug, media or link polling - without creating threads of
 * its own: the driver declares what should trigger it (GET_POLICY), a
 * monitor service waits for that and calls EVENT (urgent, non-blocking) and
 * REFRESH (settles the state). See docs/dmdrvi.md, "Monitor Ioctl Commands",
 * for when each command is called and what a driver may do in it.
 *
 * A driver that needs no monitoring does not implement them (-ENOTTY).
 * All three are called from thread context only, never from an ISR.
 */

/** Longest event handler name in dmdrvi_monitor_policy_t, including the terminator. */
#define DMDRVI_MONITOR_HANDLER_NAME_MAX      32u

/** What should trigger the monitor for a node. */
typedef struct
{
    /** dmhaman handler whose calls are the node's events ("" = no events).
     *  Fired from interrupt context by another driver (e.g. a dmgpio edge
     *  interrupt) or by the driver's own ISR. */
    char     event_handler[DMDRVI_MONITOR_HANDLER_NAME_MAX];
    uint32_t settle_ms;          /**< Quiet time after the last event before REFRESH */
    uint32_t poll_interval_ms;   /**< Periodic REFRESH interval, 0 = no polling */
} dmdrvi_monitor_policy_t;

/**
 * Read the monitoring policy of a node. Called once when the monitor starts
 * (and again after it restarts). -ENOTTY means the node is not monitored.
 *
 * arg: dmdrvi_monitor_policy_t* - output buffer
 */
#define DMDRVI_IOCTL_MONITOR_GET_POLICY      0x200

/**
 * An event arrived. Called right after every event, before the settle time.
 * Must return quickly and must not wait for in-flight I/O - it may run
 * concurrently with read/write on the same context. It may only record
 * state or set flags (e.g. make a transfer on a pulled card fail at once);
 * nodes are announced or withdrawn only by REFRESH. Return 0 when there is
 * nothing to do.
 *
 * arg: NULL
 */
#define DMDRVI_IOCTL_MONITOR_EVENT           0x201

/**
 * Settle the state behind the node. Called once when the monitor starts,
 * after events once settle_ms passed without a new one, and every
 * poll_interval_ms if non-zero. May block; serialized with I/O. Announces or
 * withdraws nodes through dmdrvi_device_available() /
 * dmdrvi_device_unavailable().
 *
 * Returns 0 when something is attached behind the node, -ENODEV when nothing
 * is, another negative errno value on failure.
 *
 * arg: NULL
 */
#define DMDRVI_IOCTL_MONITOR_REFRESH         0x202

/*
 * Graphics commands - for any device exposing a framebuffer (LCD-TFT
 * controller, SPI display, ...). Generic code needs only the path of the
 * device node: GET_INFO gives resolution, pixel format and stride, so it can
 * draw without knowing which driver is behind the node. Reading/writing the
 * node accesses the drawing buffer at the given byte offset.
 *
 * Optional commands a driver cannot honour return -ENOTSUP, unknown ones
 * -ENOTTY.
 */

/** Pixel format of a framebuffer. */
typedef enum
{
    DMDRVI_GFX_PIXEL_FORMAT_ARGB8888 = 0,  /**< 32 bpp, A in the top byte */
    DMDRVI_GFX_PIXEL_FORMAT_RGB888,        /**< 24 bpp, packed */
    DMDRVI_GFX_PIXEL_FORMAT_RGB565,        /**< 16 bpp */
    DMDRVI_GFX_PIXEL_FORMAT_ARGB1555,      /**< 16 bpp, 1-bit alpha */
    DMDRVI_GFX_PIXEL_FORMAT_ARGB4444,      /**< 16 bpp, 4-bit alpha */

    DMDRVI_GFX_PIXEL_FORMAT_COUNT
} dmdrvi_gfx_pixel_format_t;

/** Geometry and format of a framebuffer device. */
typedef struct
{
    uint16_t                  width;             /**< Active width in pixels */
    uint16_t                  height;            /**< Active height in lines */
    dmdrvi_gfx_pixel_format_t pixel_format;      /**< Framebuffer pixel format */
    uint8_t                   bytes_per_pixel;   /**< Bytes of one pixel */
    uint32_t                  stride;            /**< Bytes of one line */
    uint32_t                  framebuffer_size;  /**< Bytes of one buffer (stride * height) */
    uint8_t                   buffer_count;      /**< 1, or 2 when double buffered */
} dmdrvi_gfx_info_t;

/**
 * Rectangle fill. The rectangle is clipped to the screen. The color is always
 * 0xAARRGGBB and converted to the framebuffer's pixel format by the driver.
 */
typedef struct
{
    uint16_t x;          /**< Left column */
    uint16_t y;          /**< Top line */
    uint16_t width;      /**< Width in pixels */
    uint16_t height;     /**< Height in lines */
    uint32_t color;      /**< 0xAARRGGBB */
} dmdrvi_gfx_fill_rect_t;

/**
 * Read resolution, pixel format, stride and buffer count.
 *
 * arg: dmdrvi_gfx_info_t* - output buffer
 */
#define DMDRVI_IOCTL_GFX_GET_INFO             0x300

/**
 * Get the buffer to draw into (for direct drawing; flush the cache afterwards
 * if the core has one).
 *
 * arg: void** - receives the buffer address (output)
 */
#define DMDRVI_IOCTL_GFX_GET_FRAMEBUFFER      0x301

/**
 * Show the drawing buffer from the next frame on and draw into the other one.
 * -ENOTSUP when the device has a single buffer.
 *
 * arg: NULL
 */
#define DMDRVI_IOCTL_GFX_SWAP_BUFFERS         0x302

/**
 * Block until the next vertical blanking. -ETIMEDOUT on timeout, -EAGAIN
 * while the display is disabled.
 *
 * arg: const uint32_t* - timeout in ms, or NULL for the driver default
 */
#define DMDRVI_IOCTL_GFX_WAIT_VSYNC           0x303

/**
 * Fill a rectangle with a color.
 *
 * arg: const dmdrvi_gfx_fill_rect_t* - input
 */
#define DMDRVI_IOCTL_GFX_FILL_RECT            0x304

/**
 * Enable or disable scan-out.
 *
 * arg: const bool* - input
 */
#define DMDRVI_IOCTL_GFX_SET_DISPLAY_ENABLED  0x305

/**
 * Read whether scan-out is enabled.
 *
 * arg: bool* - output
 */
#define DMDRVI_IOCTL_GFX_GET_DISPLAY_ENABLED  0x306

/**
 * Switch the backlight (only lit while the display is enabled).
 *
 * arg: const bool* - input
 */
#define DMDRVI_IOCTL_GFX_SET_BACKLIGHT        0x307

/**
 * Read the backlight state.
 *
 * arg: bool* - output
 */
#define DMDRVI_IOCTL_GFX_GET_BACKLIGHT        0x308

/*
 * Input commands - for any device a user touches, moves or presses (touch
 * panel, mouse, buttons). Generic code (a GUI library, a test tool) needs
 * only the path of the device node: GET_INFO tells what the device reports,
 * read() and GET_STATE return its current state as one dmdrvi_input_state_t
 * whatever the driver, WAIT_EVENT sleeps until that state changes.
 *
 * Coordinates are always screen coordinates: the driver applies its own
 * configuration (axis swap, mirroring, clipping) before reporting them.
 * Commands a driver cannot honour return -ENOTSUP, unknown ones -ENOTTY.
 */

/** Most contacts one dmdrvi_input_state_t carries. */
#define DMDRVI_INPUT_MAX_CONTACTS            10u

/** Longest device name in dmdrvi_input_info_t, including the terminator. */
#define DMDRVI_INPUT_NAME_MAX                16u

/** Kind of input device. */
typedef enum
{
    DMDRVI_INPUT_TYPE_TOUCHSCREEN = 0,  /**< Contacts on top of a display */
    DMDRVI_INPUT_TYPE_MOUSE,            /**< Relative motion and buttons */
    DMDRVI_INPUT_TYPE_BUTTONS,          /**< Buttons or keys only */

    DMDRVI_INPUT_TYPE_COUNT
} dmdrvi_input_type_t;

/** Reports contacts with screen coordinates (contacts, contact_count). */
#define DMDRVI_INPUT_CAP_CONTACTS            (1u << 0)

/** Reports relative motion (dx, dy). */
#define DMDRVI_INPUT_CAP_MOTION              (1u << 1)

/** Reports a scroll wheel (wheel). */
#define DMDRVI_INPUT_CAP_WHEEL               (1u << 2)

/** Reports buttons (buttons). */
#define DMDRVI_INPUT_CAP_BUTTONS             (1u << 3)

/** Contacts carry a pressure (otherwise it is 0). */
#define DMDRVI_INPUT_CAP_PRESSURE            (1u << 4)

/** Contacts carry a size (otherwise it is 0). */
#define DMDRVI_INPUT_CAP_CONTACT_SIZE        (1u << 5)

/** WAIT_EVENT is woken by an interrupt; without it the driver polls. */
#define DMDRVI_INPUT_CAP_INTERRUPT           (1u << 6)

/** Bits of dmdrvi_input_state_t::buttons - bit n is button n. */
#define DMDRVI_INPUT_BUTTON_LEFT             (1u << 0)
#define DMDRVI_INPUT_BUTTON_RIGHT            (1u << 1)
#define DMDRVI_INPUT_BUTTON_MIDDLE           (1u << 2)

/** What happened to a contact. */
typedef enum
{
    DMDRVI_INPUT_CONTACT_DOWN = 0,  /**< Has just touched */
    DMDRVI_INPUT_CONTACT_MOVE,      /**< Stays down (moved or not) */
    DMDRVI_INPUT_CONTACT_UP,        /**< Has just been lifted - its last report */
} dmdrvi_input_contact_event_t;

/** What an input device reports. */
typedef struct
{
    char                name[DMDRVI_INPUT_NAME_MAX]; /**< Device model, e.g. "FT5336" */
    dmdrvi_input_type_t type;           /**< Kind of device */
    uint32_t            capabilities;   /**< DMDRVI_INPUT_CAP_* bits */
    uint16_t            width;          /**< Contact x is 0..width-1, 0 = unknown */
    uint16_t            height;         /**< Contact y is 0..height-1, 0 = unknown */
    uint8_t             max_contacts;   /**< Simultaneous contacts, <= DMDRVI_INPUT_MAX_CONTACTS */
    uint8_t             button_count;   /**< Number of buttons, <= 32 */
} dmdrvi_input_info_t;

/** One contact (finger, stylus), in screen coordinates. */
typedef struct
{
    uint16_t x;          /**< Column */
    uint16_t y;          /**< Line */
    uint8_t  id;         /**< Stays the same while the contact moves */
    uint8_t  event;      /**< dmdrvi_input_contact_event_t */
    uint8_t  pressure;   /**< 0 without DMDRVI_INPUT_CAP_PRESSURE */
    uint8_t  size;       /**< 0 without DMDRVI_INPUT_CAP_CONTACT_SIZE */
} dmdrvi_input_contact_t;

/**
 * State of an input device - what read() of the node and GET_STATE return.
 *
 * Fields the device does not report, contacts beyond contact_count and
 * reserved are zero, so two states compare byte by byte
 * (dmdrvi_input_state_equal()). dx, dy and wheel add up since the previous
 * state was handed out - handing it out resets them.
 */
typedef struct
{
    uint32_t               buttons;         /**< DMDRVI_INPUT_BUTTON_* bits, set = pressed */
    int16_t                dx;              /**< Motion to the right */
    int16_t                dy;              /**< Motion down */
    int16_t                wheel;           /**< Scroll steps, positive = away from the user */
    uint8_t                contact_count;   /**< 0 = nothing touches */
    uint8_t                reserved;
    dmdrvi_input_contact_t contacts[DMDRVI_INPUT_MAX_CONTACTS];
} dmdrvi_input_state_t;

/**
 * Compare two input states byte by byte. dmod modules have no libc
 * memcmp(); the state has no padding, so this compares every field.
 */
static inline bool dmdrvi_input_state_equal(const dmdrvi_input_state_t* a, const dmdrvi_input_state_t* b)
{
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;

    for (uint32_t i = 0; i < (uint32_t)sizeof(dmdrvi_input_state_t); i++)
    {
        if (pa[i] != pb[i])
            return false;
    }
    return true;
}

/**
 * Read what the device reports.
 *
 * arg: dmdrvi_input_info_t* - output buffer
 */
#define DMDRVI_IOCTL_INPUT_GET_INFO           0x400

/**
 * Read the current state - the same as read() of the node.
 *
 * arg: dmdrvi_input_state_t* - output buffer
 */
#define DMDRVI_IOCTL_INPUT_GET_STATE          0x401

/**
 * Block until the state differs from the one handed out last, or an event
 * arrived. A change between two waits is not lost. Read the state afterwards
 * - it may also equal the previous one (the event changed nothing visible).
 * -ETIMEDOUT on timeout.
 *
 * arg: const uint32_t* - timeout in ms, or NULL to wait forever
 */
#define DMDRVI_IOCTL_INPUT_WAIT_EVENT         0x402

/**
 * @brief Start of the reserved range for driver-specific custom ioctl commands
 *
 * A driver built on dmdrvi (e.g. a network driver needing something beyond
 * DMDRVI_IOCTL_NET_*) should number its own private commands starting from
 * this base, not from "last standard command + 1" - the standard command
 * set above is expected to grow over time, and a driver numbering its own
 * commands relative to whichever one happens to be last today would silently
 * collide with a new standard command added later. Standard categories are
 * spaced 0x100 apart (network 0x01, block 0x100, monitor 0x200, graphics
 * 0x300, input 0x400), which leaves generous headroom before reaching this
 * base.
 */
#define DMDRVI_IOCTL_CUSTOM_BASE              0x1000

#ifdef __cplusplus
}
#endif

#endif // DMDRVI_IOCTL_H
