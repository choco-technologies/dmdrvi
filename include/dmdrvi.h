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

/**
 * @brief Device number type
 */
typedef struct 
{
    dmdrvi_dev_id_t major;  ///< Major device number
    dmdrvi_dev_id_t minor;  ///< Minor device number
    uint8_t flags;          ///< Device numbering flags (DMDRVI_NUM_*)
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
 * 
 * @return size_t Number of bytes read
 */
dmod_dmdrvi_dif(1.0, size_t, _read, ( dmdrvi_context_t context, void* handle, void* buffer, size_t size ));

/**
 * @brief Write to a device
 *
 * @param context DMDRVI context
 * @param handle Device handle
 * @param buffer Buffer with data to write
 * @param size Number of bytes to write
 * 
 * @return size_t Number of bytes written
 */
dmod_dmdrvi_dif(1.0, size_t, _write, ( dmdrvi_context_t context, void* handle, const void* buffer, size_t size ));

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
 * @param context DMDRVI context
 * @param handle Device handle
 * @param stat Pointer to dmdrvi_stat_t structure to fill with status information
 * 
 * @return int Result of the stat operation (errno)
 */
dmod_dmdrvi_dif(1.0, int, _stat, ( dmdrvi_context_t context, void* handle, dmdrvi_stat_t* stat ));

#endif // DMDRVI_H