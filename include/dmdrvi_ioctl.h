#ifndef DMDRVI_IOCTL_H
#define DMDRVI_IOCTL_H

#include <stdint.h>
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

/**
 * @brief Start of the reserved range for driver-specific custom ioctl commands
 *
 * A driver built on dmdrvi (e.g. a network driver needing something beyond
 * DMDRVI_IOCTL_NET_*) should number its own private commands starting from
 * this base, not from "last standard command + 1" - the standard command
 * set above is expected to grow over time, and a driver numbering its own
 * commands relative to whichever one happens to be last today would silently
 * collide with a new standard command added later. Leaves generous headroom
 * (4095 possible standard commands per category) before reaching this base.
 */
#define DMDRVI_IOCTL_CUSTOM_BASE              0x1000

#ifdef __cplusplus
}
#endif

#endif // DMDRVI_IOCTL_H
