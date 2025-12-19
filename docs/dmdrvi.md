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
minor numbers, along with flags that indicate which numbering scheme the driver uses:

* **Major number** - Identifies the device channel (e.g., UART0, UART1, SPI0)
* **Minor number** - Identifies specific configuration for the same channel (e.g., different SPI speeds for different chip select lines)
* **Flags** - Indicate which numbering scheme is used (none, major only, or major+minor)

```c
typedef struct {
    dmdrvi_dev_id_t major;  ///< Major device number (channel)
    dmdrvi_dev_id_t minor;  ///< Minor device number (specific config)
    uint8_t flags;          ///< Device numbering flags
} dmdrvi_dev_num_t;
```

### Device Numbering Flags

* **DMDRVI_NUM_NONE** (0x00) - Driver does not use numbering (e.g., `/dev/dmclk`)
* **DMDRVI_NUM_MAJOR** (0x01) - Driver uses major number only (e.g., `/dev/dmuart0`)
* **DMDRVI_NUM_MINOR** (0x02) - Driver uses minor number; requires major (e.g., `/dev/dmspi0/0`)

The driver manages its own namespace and assigns device numbers when creating 
a context. Each driver can independently use the same major/minor numbers without 
conflicts.

### Access Modes

Device open flags control access permissions:

* **DMDRVI_O_RDONLY** (0x01) - Read-only access
* **DMDRVI_O_WRONLY** (0x02) - Write-only access
* **DMDRVI_O_RDWR** (0x04) - Read and write access

### Context Management

**dmdrvi_create()** creates a new driver context for the specified device. 
The *config* parameter can be NULL or a pointer to a dmini_context object 
containing device configuration. The *dev_num* parameter is an output parameter - 
the driver will assign device numbers (major, minor) and set the flags to indicate 
which numbering scheme it uses. Returns a context pointer or NULL on error.

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

// Create driver context - driver assigns device numbers
dmdrvi_dev_num_t dev_num;  // Output parameter
dmdrvi_context_t ctx = dmdrvi_create(NULL, &dev_num);

// Check the numbering scheme
if (dev_num.flags == DMDRVI_NUM_NONE) {
    Dmod_Printf("Device: /dev/dmclk\n");
} else if (dev_num.flags == DMDRVI_NUM_MAJOR) {
    Dmod_Printf("Device: /dev/dmuart%d\n", dev_num.major);
} else if (dev_num.flags == (DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR)) {
    Dmod_Printf("Device: /dev/dmspi%d/%d\n", dev_num.major, dev_num.minor);
}

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

// Create driver with configuration - driver assigns device numbers
dmdrvi_dev_num_t dev_num;  // Output parameter
dmdrvi_context_t driver = dmdrvi_create(config, &dev_num);

// The driver has now assigned device numbers and set flags
// Config contains device-specific settings (baudrate, mode, speed, etc.)

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
// Create contexts for different drivers and configurations
// Each driver manages its own device number namespace

// Example 1: Driver without numbering (clock driver)
dmdrvi_dev_num_t clk_num;
dmdrvi_context_t clk_ctx = dmdrvi_create(NULL, &clk_num);
// clk_num.flags == DMDRVI_NUM_NONE
// Device file: /dev/dmclk

// Example 2: Driver with major numbering only (UART driver)
dmdrvi_dev_num_t uart0_num, uart1_num;
dmdrvi_context_t uart0_ctx = dmdrvi_create(uart0_config, &uart0_num);
dmdrvi_context_t uart1_ctx = dmdrvi_create(uart1_config, &uart1_num);
// uart0_num.flags == DMDRVI_NUM_MAJOR, uart0_num.major == 0
// uart1_num.flags == DMDRVI_NUM_MAJOR, uart1_num.major == 1
// Device files: /dev/dmuart0, /dev/dmuart1

// Example 3: Driver with major+minor numbering (SPI driver)
dmdrvi_dev_num_t spi0_cs0_num, spi0_cs1_num;
dmdrvi_context_t spi0_cs0_ctx = dmdrvi_create(spi0_cs0_config, &spi0_cs0_num);
dmdrvi_context_t spi0_cs1_ctx = dmdrvi_create(spi0_cs1_config, &spi0_cs1_num);
// spi0_cs0_num.flags == (DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR)
// spi0_cs0_num.major == 0, spi0_cs0_num.minor == 0
// spi0_cs1_num.major == 0, spi0_cs1_num.minor == 1
// Device files: /dev/dmspi0/0, /dev/dmspi0/1

// Open all devices
void* clk_handle = dmdrvi_open(clk_ctx, DMDRVI_O_RDWR);
void* uart0_handle = dmdrvi_open(uart0_ctx, DMDRVI_O_RDWR);
void* uart1_handle = dmdrvi_open(uart1_ctx, DMDRVI_O_RDWR);
void* spi0_cs0_handle = dmdrvi_open(spi0_cs0_ctx, DMDRVI_O_RDWR);
void* spi0_cs1_handle = dmdrvi_open(spi0_cs1_ctx, DMDRVI_O_RDWR);

// Use devices...

// Cleanup all
dmdrvi_close(clk_ctx, clk_handle);
dmdrvi_close(uart0_ctx, uart0_handle);
dmdrvi_close(uart1_ctx, uart1_handle);
dmdrvi_close(spi0_cs0_ctx, spi0_cs0_handle);
dmdrvi_close(spi0_cs1_ctx, spi0_cs1_handle);

dmdrvi_free(clk_ctx);
dmdrvi_free(uart0_ctx);
dmdrvi_free(uart1_ctx);
dmdrvi_free(spi0_cs0_ctx);
dmdrvi_free(spi0_cs1_ctx);
```

## DEVICE CHANNELS AND CONFIGURATIONS

Each driver manages its own device number namespace. The driver decides which 
numbering scheme to use based on its needs:

### Numbering Schemes

**No Numbering (DMDRVI_NUM_NONE)**
* Driver doesn't use device numbers
* Device file uses driver name only
* Example: `/dev/dmclk` (clock driver)

**Major Number Only (DMDRVI_NUM_MAJOR)**
* Driver uses major number to identify channels
* Device files named with major number suffix
* Example: `/dev/dmuart0`, `/dev/dmuart1` (UART driver channels)

**Major and Minor Numbers (DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR)**
* Driver uses both major and minor numbers
* Creates directory for major number, files for each minor number
* Example: `/dev/dmspi0/0`, `/dev/dmspi0/1` (SPI driver with different configs)

### Device Number Namespaces

Each driver has its own independent namespace. Different drivers can use the 
same major/minor numbers without conflicts:

| Driver Type | Flags                    | Major | Minor | Device Path     |
|-------------|--------------------------|-------|-------|-----------------|
| CLK         | NUM_NONE                 | -     | -     | /dev/dmclk      |
| UART        | NUM_MAJOR                | 0     | -     | /dev/dmuart0    |
| UART        | NUM_MAJOR                | 1     | -     | /dev/dmuart1    |
| SPI         | NUM_MAJOR \| NUM_MINOR   | 0     | 0     | /dev/dmspi0/0   |
| SPI         | NUM_MAJOR \| NUM_MINOR   | 0     | 1     | /dev/dmspi0/1   |
| SPI         | NUM_MAJOR \| NUM_MINOR   | 1     | 0     | /dev/dmspi1/0   |
| I2C         | NUM_MAJOR                | 0     | -     | /dev/dmi2c0     |

The major number identifies the device channel within a driver (e.g., UART0 vs UART1).
The minor number identifies specific configurations for the same channel (e.g., 
different SPI speeds for different chip select lines).

## CONFIGURATION

Example device configuration using dmini (device.ini):

```ini
[uart0]
baudrate=115200
databits=8
parity=none
stopbits=1

[spi0_slow]
speed=1000000
mode=0
bits_per_word=8

[spi0_fast]
speed=10000000
mode=0
bits_per_word=8
```

The driver reads the configuration and assigns device numbers based on its 
internal logic. The filesystem layer then creates device files according to the 
numbering scheme used by the driver.

## SEE ALSO

dmod(3), dmini(3), dmod_loader(1)

## AUTHOR

Patryk Kubiak

## LICENSE

MIT License - Copyright (c) 2025 Choco-Technologies
