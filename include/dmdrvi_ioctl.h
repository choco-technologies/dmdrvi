#ifndef DMDRVI_IOCTL_H
#define DMDRVI_IOCTL_H

#include <stdint.h>

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
