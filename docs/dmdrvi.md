# DMDRVI(3)

## NAME

dmdrvi - DMOD Driver Interface Module

## SYNOPSIS

```c
#include "dmdrvi.h"

dmdrvi_context_t dmdrvi_create(void* config, const dmdrvi_dev_num_t* dev_num);
void dmdrvi_free(dmdrvi_context_t context);

void* dmdrvi_open(dmdrvi_context_t context, int flags);
void dmdrvi_close(dmdrvi_context_t context, void* handle);

size_t dmdrvi_read(dmdrvi_context_t context, void* handle, 
                   void* buffer, size_t size);
size_t dmdrvi_write(dmdrvi_context_t context, void* handle, 
                    const void* buffer, size_t size);

int dmdrvi_ioctl(dmdrvi_context_t context, void* handle, 
                 int command, void* arg);
int dmdrvi_flush(dmdrvi_context_t context, void* handle);
int dmdrvi_stat(dmdrvi_context_t context, void* handle, 
                dmdrvi_stat_t* stat);
```

## DESCRIPTION

The **dmdrvi** module provides a standardized interface for device drivers in 
the DMOD framework. It enables uniform access to hardware devices through a 
consistent API, using a major/minor device numbering system similar to UNIX 
device files.

### Device Number System

Devices are identified using a device number structure containing major and 
minor numbers:

* **Major number** - Identifies the device channel (e.g., UART0, UART1, SPI0)
* **Minor number** - Identifies virtual configuration for the same channel (useful when you need different configurations on the same channel, e.g., different SPI speeds for different chip select lines)

```c
typedef struct {
    dmdrvi_dev_id_t major;  ///< Major device number (channel)
    dmdrvi_dev_id_t minor;  ///< Minor device number (virtual config)
} dmdrvi_dev_num_t;
```

### Access Modes

Device open flags control access permissions:

* **DMDRVI_O_RDONLY** (0x01) - Read-only access
* **DMDRVI_O_WRONLY** (0x02) - Write-only access
* **DMDRVI_O_RDWR** (0x04) - Read and write access

### Context Management

**dmdrvi_create()** creates a new driver context for the specified device. 
The *config* parameter can be NULL or a pointer to a dmini_context object 
containing device configuration. The *dev_num* parameter specifies the device 
major and minor numbers. Returns a context pointer or NULL on error.

**dmdrvi_free()** frees all resources associated with a driver context.

### Device Operations

**dmdrvi_open()** opens the device with the specified access flags. Returns 
a device handle or NULL on error.

**dmdrvi_close()** closes a previously opened device handle and releases 
associated resources.

**dmdrvi_read()** reads up to *size* bytes from the device into *buffer*. 
Returns the number of bytes actually read, or 0 on EOF/error.

**dmdrvi_write()** writes up to *size* bytes from *buffer* to the device. 
Returns the number of bytes actually written, or a negative value on error.

**dmdrvi_ioctl()** performs device-specific control operations. The *command* 
parameter specifies the operation, and *arg* provides operation-specific data. 
Returns 0 on success or an errno-compatible error code.

**dmdrvi_flush()** flushes any pending data in device buffers. Returns 0 on 
success or an errno-compatible error code.

**dmdrvi_stat()** retrieves device status information including size and mode. 
Returns 0 on success or an errno-compatible error code.

### Device Status Structure

```c
typedef struct {
    uint32_t size;  //!< Size of the device/file
    uint32_t mode;  //!< Device mode (permissions)
} dmdrvi_stat_t;
```

## RETURN VALUES

Functions return values as follows:

* **dmdrvi_create()** - Context pointer on success, NULL on error
* **dmdrvi_open()** - Device handle on success, NULL on error
* **dmdrvi_read()/write()** - Number of bytes transferred, or 0/negative on error
* **dmdrvi_ioctl()/flush()/stat()** - 0 on success, errno-compatible error code otherwise

## EXAMPLES

### Basic Device Access

```c
#include "dmdrvi.h"

// Define device (UART0, default config - major=0, minor=0)
dmdrvi_dev_num_t dev_num = { .major = 0, .minor = 0 };

// Create driver context
dmdrvi_context_t ctx = dmdrvi_create(NULL, &dev_num);

// Open device for reading and writing
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR);

// Write data
const char* msg = "Hello Device!\n";
size_t written = dmdrvi_write(ctx, handle, msg, strlen(msg));

// Read response
char buffer[256];
size_t read = dmdrvi_read(ctx, handle, buffer, sizeof(buffer));

// Close and cleanup
dmdrvi_close(ctx, handle);
dmdrvi_free(ctx);
```

### Using Configuration

```c
#include "dmini.h"
#include "dmdrvi.h"

// Parse device configuration file
dmini_context_t config = dmini_create();
dmini_parse_file(config, "device.ini");

// Get device numbers from config
int major = dmini_get_int(config, "uart", "major", 0);  // UART channel
int minor = dmini_get_int(config, "uart", "minor", 0);  // Configuration variant

dmdrvi_dev_num_t dev_num = { .major = major, .minor = minor };

// Create driver with configuration
dmdrvi_context_t driver = dmdrvi_create(config, &dev_num);

// Open and use device
void* handle = dmdrvi_open(driver, DMDRVI_O_RDWR);
// ... perform operations ...
dmdrvi_close(driver, handle);

// Cleanup
dmdrvi_free(driver);
dmini_destroy(config);
```

### Device Status Query

```c
// Open device
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDONLY);

// Get device status
dmdrvi_stat_t stat;
int result = dmdrvi_stat(ctx, handle, &stat);

if (result == 0) {
    Dmod_Printf("Device size: %u bytes\n", stat.size);
    Dmod_Printf("Device mode: 0x%08X\n", stat.mode);
}

dmdrvi_close(ctx, handle);
```

### Device Control (ioctl)

```c
// Open device
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR);

// Set baud rate (example ioctl command)
#define IOCTL_SET_BAUDRATE 0x5001
uint32_t baudrate = 115200;
int result = dmdrvi_ioctl(ctx, handle, IOCTL_SET_BAUDRATE, &baudrate);

if (result == 0) {
    Dmod_Printf("Baud rate set successfully\n");
} else {
    Dmod_Printf("Error setting baud rate: %d\n", result);
}

dmdrvi_close(ctx, handle);
```

### Multiple Device Access

```c
// Create contexts for different device channels and configurations
dmdrvi_dev_num_t uart0 = { .major = 0, .minor = 0 };  // UART channel 0, default config
dmdrvi_dev_num_t uart1 = { .major = 1, .minor = 0 };  // UART channel 1, default config
dmdrvi_dev_num_t spi0_cs0 = { .major = 0, .minor = 0 };  // SPI channel 0, CS0 config
dmdrvi_dev_num_t spi0_cs1 = { .major = 0, .minor = 1 };  // SPI channel 0, CS1 config (e.g., different speed)

dmdrvi_context_t uart0_ctx = dmdrvi_create(NULL, &uart0);
dmdrvi_context_t uart1_ctx = dmdrvi_create(NULL, &uart1);
dmdrvi_context_t spi0_cs0_ctx = dmdrvi_create(NULL, &spi0_cs0);
dmdrvi_context_t spi0_cs1_ctx = dmdrvi_create(NULL, &spi0_cs1);

// Open all devices
void* uart0_handle = dmdrvi_open(uart0_ctx, DMDRVI_O_RDWR);
void* uart1_handle = dmdrvi_open(uart1_ctx, DMDRVI_O_RDWR);
void* spi0_cs0_handle = dmdrvi_open(spi0_cs0_ctx, DMDRVI_O_RDWR);
void* spi0_cs1_handle = dmdrvi_open(spi0_cs1_ctx, DMDRVI_O_RDWR);

// Use devices...

// Cleanup all
dmdrvi_close(uart0_ctx, uart0_handle);
dmdrvi_close(uart1_ctx, uart1_handle);
dmdrvi_close(spi0_cs0_ctx, spi0_cs0_handle);
dmdrvi_close(spi0_cs1_ctx, spi0_cs1_handle);

dmdrvi_free(uart0_ctx);
dmdrvi_free(uart1_ctx);
dmdrvi_free(spi0_cs0_ctx);
dmdrvi_free(spi0_cs1_ctx);
```

## DEVICE CHANNELS AND CONFIGURATIONS

The major number identifies the device channel, while the minor number identifies 
the virtual configuration for that channel.

Example device channels:

| Device Channel   | Major | Description                     |
|------------------|-------|---------------------------------|
| UART0            | 0     | First UART channel              |
| UART1            | 1     | Second UART channel             |
| SPI0             | 0     | First SPI channel               |
| SPI1             | 1     | Second SPI channel              |
| GPIO0            | 0     | First GPIO controller           |
| I2C0             | 0     | First I2C channel               |

Minor numbers identify virtual configurations for the same channel. For example, 
SPI0 with minor=0 might use 1MHz clock speed, while SPI0 with minor=1 uses 10MHz 
for a different chip select line.

Example SPI configurations:
```
Channel  Major  Minor  Description
-------------------------------------
SPI0     0      0      Default speed (1MHz)
SPI0     0      1      High speed (10MHz) for different CS
SPI0     0      2      Custom config for another device
```

## CONFIGURATION

Example device configuration using dmini (device.ini):

```ini
[uart0_default]
major=0
minor=0
baudrate=115200
databits=8
parity=none

[spi0_slow]
major=0
minor=0
speed=1000000
mode=0

[spi0_fast]
major=0
minor=1
speed=10000000
mode=0
```

## SEE ALSO

dmod(3), dmini(3), dmod_loader(1)

## AUTHOR

Patryk Kubiak

## LICENSE

MIT License - Copyright (c) 2025 Choco-Technologies
