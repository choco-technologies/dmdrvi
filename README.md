# dmdrvi - DMOD Driver Interface Module

A DMOD module library that provides a standardized interface for device drivers, enabling uniform access to hardware devices in embedded systems.

## Overview

dmdrvi is a driver interface module designed for embedded systems using the DMOD framework. It provides a consistent API for device driver operations including open, close, read, write, ioctl, and other standard device operations. The module uses device numbers (major/minor) to identify and manage devices.

## Features

- **Standardized Device Interface**: Uniform API for all device drivers
- **Device Identification**: Major/minor device number system
- **Flexible Access Modes**: Read-only, write-only, and read-write support
- **Standard Operations**: open, close, read, write, ioctl, flush, stat
- **Configuration Support**: Integration with dmini for device configuration
- **SAL-Compatible**: Uses only DMOD SAL functions
- **Lightweight**: Minimal memory footprint suitable for embedded systems

## API

### Context Management
- `dmdrvi_create(config, dev_num)` - Create driver context with configuration
- `dmdrvi_free(context)` - Free driver context

### Device Operations
- `dmdrvi_open(context, flags)` - Open device with specified flags
- `dmdrvi_close(context, handle)` - Close device handle
- `dmdrvi_read(context, handle, buffer, size)` - Read data from device
- `dmdrvi_write(context, handle, buffer, size)` - Write data to device
- `dmdrvi_ioctl(context, handle, command, arg)` - Device control operations
- `dmdrvi_flush(context, handle)` - Flush device buffers
- `dmdrvi_stat(context, handle, stat)` - Get device status

### Open Flags
- `DMDRVI_O_RDONLY` - Open for read only
- `DMDRVI_O_WRONLY` - Open for write only
- `DMDRVI_O_RDWR` - Open for read and write

## Usage Example

```c
#include "dmdrvi.h"

// Create driver context - driver assigns device numbers
dmdrvi_dev_num_t dev_num;  // Output parameter
dmdrvi_context_t ctx = dmdrvi_create(NULL, &dev_num);

// Check what numbering scheme the driver uses
if (dev_num.flags == DMDRVI_NUM_NONE) {
    // Device file: /dev/dmclk
    Dmod_Printf("Device: /dev/dmclk\n");
} else if (dev_num.flags == DMDRVI_NUM_MAJOR) {
    // Device file: /dev/dmuart0, /dev/dmuart1, etc.
    Dmod_Printf("Device: /dev/dmuart%d\n", dev_num.major);
} else if (dev_num.flags == (DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR)) {
    // Device file: /dev/dmspi0/0, /dev/dmspi0/1, etc.
    Dmod_Printf("Device: /dev/dmspi%d/%d\n", dev_num.major, dev_num.minor);
}

// Open device for reading and writing
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR);

// Write data to device
char write_buffer[] = "Hello Device";
size_t written = dmdrvi_write(ctx, handle, write_buffer, sizeof(write_buffer));

// Read data from device
char read_buffer[256];
size_t read = dmdrvi_read(ctx, handle, read_buffer, sizeof(read_buffer));

// Get device status
dmdrvi_stat_t stat;
int result = dmdrvi_stat(ctx, handle, &stat);
Dmod_Printf("Device size: %u bytes\n", stat.size);

// Flush buffers
dmdrvi_flush(ctx, handle);

// Close device
dmdrvi_close(ctx, handle);

// Free context
dmdrvi_free(ctx);
```

## Building

```bash
mkdir build
cd build
cmake .. -DDMOD_MODE=DMOD_MODULE
cmake --build .
```

This generates:
- `dmf/dmdrvi.dmf` - The driver interface library module
- `dmf/dmdrvi_version.txt` - Version information

## Integration with Configuration

dmdrvi can work with dmini module for device configuration:

```c
#include "dmini.h"
#include "dmdrvi.h"

// Parse device configuration
dmini_context_t config = dmini_create();
dmini_parse_file(config, "device.ini");

// Create driver context - driver assigns device numbers
dmdrvi_dev_num_t dev_num;  // Output parameter
dmdrvi_context_t driver = dmdrvi_create(config, &dev_num);

// The driver has now assigned major/minor numbers and set the flags
// The filesystem layer can use dev_num to create appropriate device files

// Use driver...

dmdrvi_free(driver);
dmini_destroy(config);
```

## Device Number System

The driver manages device numbering within its own namespace. Each driver decides whether it uses numbering and which numbering scheme to employ:

### Numbering Schemes

1. **No Numbering** (DMDRVI_NUM_NONE):
   - Driver doesn't use device numbers
   - Device file uses driver name only
   - Example: `/dev/dmclk`

2. **Major Number Only** (DMDRVI_NUM_MAJOR):
   - Driver uses major number to identify channels
   - Device files named with major number suffix
   - Example: `/dev/dmuart0`, `/dev/dmuart1`

3. **Major and Minor Numbers** (DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR):
   - Driver uses both major and minor numbers
   - Creates directory for major number, files for each minor number
   - Example: `/dev/dmspi0/0`, `/dev/dmspi0/1`, `/dev/dmspi1/0`

### Device Number Assignment

- **Major number**: Identifies the device channel within a driver (e.g., UART0, UART1, SPI0)
- **Minor number**: Identifies specific configuration for the same channel (e.g., different SPI speeds for different CS lines)
- **Each driver has its own namespace**: Different drivers can use the same major/minor numbers independently

The driver assigns device numbers when creating a context. The filesystem layer receives the assigned numbers and creates appropriate device files.

Example device numbers:
```
Driver  Flags                    Major   Minor   Device Path
-------------------------------------------------------------
CLK     NUM_NONE                 -       -       /dev/dmclk
UART    NUM_MAJOR                0       -       /dev/dmuart0
UART    NUM_MAJOR                1       -       /dev/dmuart1
SPI     NUM_MAJOR|NUM_MINOR      0       0       /dev/dmspi0/0
SPI     NUM_MAJOR|NUM_MINOR      0       1       /dev/dmspi0/1
SPI     NUM_MAJOR|NUM_MINOR      1       0       /dev/dmspi1/0
```

## Error Handling

Most functions return error codes compatible with standard errno values:
- `0` - Success
- Negative values - Error codes (implementation-specific)

For read/write operations:
- Returns number of bytes transferred on success
- Returns 0 or negative value on error

## Memory Footprint

The driver interface module has a minimal footprint suitable for embedded systems. Actual memory usage depends on the underlying driver implementation.

## License

MIT License - see LICENSE file for details
