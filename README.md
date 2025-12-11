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

// Define device number (UART channel 0, default config)
dmdrvi_dev_num_t dev_num = {
    .major = 0,
    .minor = 0
};

// Create driver context (config can be NULL or dmini context)
dmdrvi_context_t ctx = dmdrvi_create(NULL, &dev_num);

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

// Get device numbers from config
int major = dmini_get_int(config, "device", "major", 0);  // Channel number
int minor = dmini_get_int(config, "device", "minor", 0);  // Config variant

dmdrvi_dev_num_t dev_num = { .major = major, .minor = minor };

// Create driver context with configuration
dmdrvi_context_t driver = dmdrvi_create(config, &dev_num);

// Use driver...

dmdrvi_free(driver);
dmini_destroy(config);
```

## Device Number System

The module uses a major/minor device numbering system:
- **Major number**: Identifies the device channel (e.g., UART0, UART1, SPI0)
- **Minor number**: Identifies virtual configuration for the same channel (useful when you need different configurations, e.g., different SPI speeds for different CS lines)

Example device numbers:
```
Device Channel      Major   Minor   Description
---------------------------------------------------
UART0 default       0       0       Default configuration
UART0 alt config    0       1       Alternative configuration
UART1 default       1       0       Default configuration
SPI0 CS0            0       0       SPI channel 0, config 0
SPI0 CS1            0       1       SPI channel 0, config 1 (e.g., different speed)
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
