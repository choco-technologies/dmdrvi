#ifndef DMDRVI_H
#define DMDRVI_H

#include <stdint.h>
#include "dmod.h"
#include "dmdrvi_defs.h"
#include "dmini.h"

/**
 * @brief Opaque context type for DMDRVI module
 */
typedef struct dmdrvi_context* dmdrvi_context_t;

/**
 * @brief Device identifier type
 */
typedef uint8_t dmdrvi_dev_id_t;

/**
 * @brief Open for read only flag
 */
#define DMDRVI_O_RDONLY    0x01    

/**
 * @brief Open for write only flag
 */
#define DMDRVI_O_WRONLY    0x02    

/**
 * @brief Open for read and write flag
 */
#define DMDRVI_O_RDWR      0x04    

/**
 * @brief Device numbering flags
 */
#define DMDRVI_NUM_NONE         0x00    ///< Driver does not use numbering
#define DMDRVI_NUM_MAJOR        0x01    ///< Driver uses major number only
#define DMDRVI_NUM_MINOR        0x02    ///< Driver uses minor number (must be combined with MAJOR)
#define DMDRVI_NUM_ALT_NAME     0x04    ///< Driver provides an alternative file name (alt_name field is valid)

/**
 * @brief Maximum length of the alternative file name (excluding null terminator)
 */
#define DMDRVI_ALT_NAME_MAX_LEN 32

/**
 * @brief Device number type
 */
typedef struct 
{
    dmdrvi_dev_id_t major;                          ///< Major device number
    dmdrvi_dev_id_t minor;                          ///< Minor device number
    uint8_t flags;                                  ///< Device numbering flags (DMDRVI_NUM_*)
    char alt_name[DMDRVI_ALT_NAME_MAX_LEN + 1];    ///< Alternative file name (valid when DMDRVI_NUM_ALT_NAME flag is set)
} dmdrvi_dev_num_t;

/**
 * @brief File status structure
 */
typedef struct 
{
    uint32_t size;       //!< Size of the file
    uint32_t mode;       //!< File mode (permissions)
} dmdrvi_stat_t;

/**
 * @brief Create a DMDRVI context
 *
 * The driver will assign device numbers based on the configuration and return them
 * via the dev_num parameter. The driver also sets flags to indicate which numbering
 * scheme it uses (none, major only, or major+minor).
 *
 * @param config Pointer to dmini_context object with configuration parameters (dmini module required to parse them)
 * @param dev_num Output pointer to device number structure - driver fills in major, minor, and flags (must not be NULL)
 * 
 * @return dmdrvi_context_t Created DMDRVI context
 */
dmod_dmdrvi_dif(1.0, dmdrvi_context_t, _create, ( dmini_context_t config, dmdrvi_dev_num_t* dev_num ));

/**
 * @brief Free a DMDRVI context
 *
 * @param context DMDRVI context to free
 */
dmod_dmdrvi_dif(1.0, void, _free, ( dmdrvi_context_t context ));

/**
 * @brief Open a device
 *
 * @param context DMDRVI context
 * @param flags Open flags  (DMDRVI_O_RDONLY, DMDRVI_O_WRONLY, DMDRVI_O_RDWR)
 * 
 * @return void* Device handle
 */
dmod_dmdrvi_dif(1.0, void*, _open, ( dmdrvi_context_t context, int flags ));

/**
 * @brief Close a device
 *
 * @param context DMDRVI context
 * @param handle Device handle
 */
dmod_dmdrvi_dif(1.0, void, _close, ( dmdrvi_context_t context, void* handle ));

/**
 * @brief Read from a device
 *
 * @param context DMDRVI context
 * @param handle Device handle
 * @param buffer Buffer to read data into
 * @param size Number of bytes to read
 * @param offset Byte offset from the beginning of the device to read from
 * 
 * @return size_t Number of bytes read
 */
dmod_dmdrvi_dif(1.0, size_t, _read, ( dmdrvi_context_t context, void* handle, void* buffer, size_t size, uint32_t offset ));

/**
 * @brief Write to a device
 *
 * @param context DMDRVI context
 * @param handle Device handle
 * @param buffer Buffer with data to write
 * @param size Number of bytes to write
 * @param offset Byte offset from the beginning of the device to write to
 * 
 * @return size_t Number of bytes written
 */
dmod_dmdrvi_dif(1.0, size_t, _write, ( dmdrvi_context_t context, void* handle, const void* buffer, size_t size, uint32_t offset ));

/**
 * @brief Ioctl operation on a device
 *
 * @param context DMDRVI context
 * @param handle Device handle
 * @param command Ioctl command
 * @param arg Argument for the ioctl command
 * 
 * @return int Result of the ioctl operation (errno)
 */
dmod_dmdrvi_dif(1.0, int, _ioctl, ( dmdrvi_context_t context, void* handle, int command, void* arg ));

/**
 * @brief Flush device buffers
 *
 * @param context DMDRVI context
 * @param handle Device handle
 * 
 * @return int Result of the flush operation (errno)
 */
dmod_dmdrvi_dif(1.0, int, _flush, ( dmdrvi_context_t context, void* handle ));

/**
 * @brief Get device status
 *
 * Gets status information for the specified device path without requiring
 * the device to be opened first (similar to POSIX stat() which works with
 * a path without requiring fopen()).
 *
 * @param context DMDRVI context
 * @param path Device path (e.g., "/dev/dmuart0", "/dev/dmspi0/0")
 * @param stat Pointer to dmdrvi_stat_t structure to fill with status information
 * 
 * @return int Result of the stat operation (errno)
 */
dmod_dmdrvi_dif(1.0, int, _stat, ( dmdrvi_context_t context, const char* path, dmdrvi_stat_t* stat ));

/**
 * @brief Put device into sleep mode
 *
 * Puts the device into a low-power sleep state to conserve energy.
 * The device can be awakened later using dmdrvi_wake_up().
 * Device behavior during sleep is driver-specific but typically includes:
 * - Disabling device clocks
 * - Entering low-power mode
 * - Preserving device state for later restoration
 *
 * @param context DMDRVI context
 * 
 * @return int 0 on success, errno-compatible error code otherwise
 */
dmod_dmdrvi_dif(1.0, int, _sleep, ( dmdrvi_context_t context ));

/**
 * @brief Wake device from sleep mode
 *
 * Wakes the device from a low-power sleep state back to normal operation.
 * The device must have been previously put to sleep using dmdrvi_sleep().
 * Device behavior on wake-up is driver-specific but typically includes:
 * - Re-enabling device clocks
 * - Restoring device state
 * - Returning to normal power mode
 *
 * @param context DMDRVI context
 * 
 * @return int 0 on success, errno-compatible error code otherwise
 */
dmod_dmdrvi_dif(1.0, int, _wake_up, ( dmdrvi_context_t context ));

#endif // DMDRVI_H