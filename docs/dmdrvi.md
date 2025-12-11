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

* **Major number** - Identifies the device driver type (e.g., serial, GPIO, SPI)
* **Minor number** - Identifies the specific device instance (e.g., UART0, UART1)

```c
typedef struct {
    dmdrvi_dev_id_t major;  // Major device number
    dmdrvi_dev_id_t minor;  // Minor device number
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
    uint32_t size;  // Size of the device/file
    uint32_t mode;  // Device mode (permissions)
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

// Define device (UART0 - major=1, minor=0)
dmdrvi_dev_num_t dev_num = { .major = 1, .minor = 0 };

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
int major = dmini_get_int(config, "uart", "major", 1);
int minor = dmini_get_int(config, "uart", "minor", 0);

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
// Create contexts for different devices
dmdrvi_dev_num_t uart0 = { .major = 1, .minor = 0 };
dmdrvi_dev_num_t uart1 = { .major = 1, .minor = 1 };
dmdrvi_dev_num_t gpio = { .major = 2, .minor = 0 };

dmdrvi_context_t uart0_ctx = dmdrvi_create(NULL, &uart0);
dmdrvi_context_t uart1_ctx = dmdrvi_create(NULL, &uart1);
dmdrvi_context_t gpio_ctx = dmdrvi_create(NULL, &gpio);

// Open all devices
void* uart0_handle = dmdrvi_open(uart0_ctx, DMDRVI_O_RDWR);
void* uart1_handle = dmdrvi_open(uart1_ctx, DMDRVI_O_RDWR);
void* gpio_handle = dmdrvi_open(gpio_ctx, DMDRVI_O_WRONLY);

// Use devices...

// Cleanup all
dmdrvi_close(uart0_ctx, uart0_handle);
dmdrvi_close(uart1_ctx, uart1_handle);
dmdrvi_close(gpio_ctx, gpio_handle);

dmdrvi_free(uart0_ctx);
dmdrvi_free(uart1_ctx);
dmdrvi_free(gpio_ctx);
```

## DEVICE TYPES

Common device types and their typical major numbers:

| Device Type      | Major | Description                    |
|------------------|-------|--------------------------------|
| Serial/UART      | 1     | Serial communication ports     |
| GPIO             | 2     | General Purpose I/O            |
| SPI              | 3     | Serial Peripheral Interface    |
| I2C              | 4     | Inter-Integrated Circuit       |
| ADC              | 5     | Analog-to-Digital Converter    |
| DAC              | 6     | Digital-to-Analog Converter    |
| Timer            | 7     | Hardware timers                |
| PWM              | 8     | Pulse Width Modulation         |

Minor numbers typically identify specific instances (e.g., UART0, UART1).

## CONFIGURATION

Example device configuration using dmini (device.ini):

```ini
[uart]
major=1
minor=0
baudrate=115200
databits=8
parity=none

[gpio]
major=2
minor=0
direction=output
initial_state=low
```

## SEE ALSO

dmod(3), dmini(3), dmod_loader(1)

## AUTHOR

Patryk Kubiak

## LICENSE

MIT License - Copyright (c) 2025 Choco-Technologies
