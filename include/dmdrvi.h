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
 * A single driver context can expose more than one device (e.g. a
 * dynamically discovered sub-device announced via dmdrvi_device_available()),
 * so dev_num identifies which one of the context's devices is being opened.
 *
 * @param context DMDRVI context
 * @param flags Open flags  (DMDRVI_O_RDONLY, DMDRVI_O_WRONLY, DMDRVI_O_RDWR)
 * @param dev_num Device number identifying which device within the context to open
 *
 * @return void* Device handle
 */
dmod_dmdrvi_dif(1.0, void*, _open, ( dmdrvi_context_t context, int flags, const dmdrvi_dev_num_t* dev_num ));

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
 * @brief Notify that a new device is available within a driver context
 *
 * This is a MAL (Module Abstraction Layer) interface, implemented by the
 * dmdevfs layer - the opposite direction of the other DMDRVI functions
 * above, which are called by dmdevfs and implemented by the driver. It
 * allows a driver to actively inform dmdevfs that a new device became
 * available within an already existing context (e.g. a hot-plugged
 * sub-device or a dynamically discovered channel), so that dmdevfs can
 * expose a corresponding device file for it.
 *
 * The driver does not create a new context for this device - context is
 * where dmdevfs first learned about the driver (via dmdrvi_create()), which
 * is also what ties the notification to the right dmdevfs instance in
 * setups where dmdevfs is mounted more than once. dev_num identifies the
 * new device within that context and must later be passed to
 * dmdrvi_open() to open it.
 *
 * @param context DMDRVI context the new device belongs to
 * @param dev_num Device number identifying the newly available device
 */
dmod_dmdrvi_mal(1.0, void, _device_available, ( dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num ));

/**
 * @brief Notify that a device is no longer available within a driver context
 *
 * This is a MAL (Module Abstraction Layer) interface, implemented by the
 * dmdevfs layer. It is the counterpart of dmdrvi_device_available() and
 * allows a driver to inform dmdevfs that a device previously announced
 * (or present from the initial dmdrvi_create() call) is no longer valid -
 * for example because the underlying hot-plugged sub-device was removed.
 * dmdevfs should remove the corresponding device file; the context itself
 * remains valid and is only freed via dmdrvi_free().
 *
 * @param context DMDRVI context the device belongs to
 * @param dev_num Device number identifying the device that is no longer available
 */
dmod_dmdrvi_mal(1.0, void, _device_unavailable, ( dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num ));

/**
 * @brief Notify a driver that the absolute path of one of its devices is now known
 *
 * This is a regular DIF (implemented by the driver, called by dmdevfs) - the
 * opposite direction of the other DMDRVI functions above, which are called
 * by dmdevfs and implemented by the driver, but same direction as those:
 * dmdevfs pushes the path to the driver as soon as it is available, rather
 * than the driver having to pull/poll for it.
 *
 * A device's absolute path cannot be resolved until the dmdevfs mount that
 * owns it has itself been fully mounted (see dmfsi_mounted() in dmfsi.h) -
 * which for devices created during the initial config-driven setup happens
 * well after dmdrvi_create() returns for them. This callback fires once that
 * precondition is met: either shortly after dmdevfs itself becomes mounted
 * (for devices already registered by then), or immediately upon
 * registration (for devices announced via dmdrvi_device_available(), or any
 * other driver registered with an already-mounted dmdevfs instance).
 *
 * Implementing this dif is optional - a driver that doesn't need its own
 * absolute path can simply not implement it. A driver that does need it
 * should cache the value passed here, e.g. for use later inside
 * dmdrvi_ioctl() or dmdrvi_open() - there is no on-demand/pull alternative.
 *
 * @param context DMDRVI context the device belongs to
 * @param dev_num Device number identifying the device
 * @param path Absolute, null-terminated path under which the device is now exposed
 */
dmod_dmdrvi_dif(1.0, void, _path_ready, ( dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num, const char* path ));

#endif // DMDRVI_H