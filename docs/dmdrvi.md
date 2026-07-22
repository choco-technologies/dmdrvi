# DMDRVI(3)

## NAME

dmdrvi - DMOD Driver Interface Module

## SYNOPSIS

```c
#include "dmdrvi.h"

dmdrvi_context_t dmdrvi_create(void* config, const dmdrvi_dev_num_t* dev_num);
void dmdrvi_free(dmdrvi_context_t context);

void* dmdrvi_open(dmdrvi_context_t context, int flags, const dmdrvi_dev_num_t* dev_num);
void dmdrvi_close(dmdrvi_context_t context, void* handle);

size_t dmdrvi_read(dmdrvi_context_t context, void* handle, 
                   void* buffer, size_t size, uint32_t offset);
size_t dmdrvi_write(dmdrvi_context_t context, void* handle, 
                    const void* buffer, size_t size, uint32_t offset);

int dmdrvi_ioctl(dmdrvi_context_t context, void* handle, 
                 int command, void* arg);
int dmdrvi_flush(dmdrvi_context_t context, void* handle);
int dmdrvi_stat(dmdrvi_context_t context, const char* path, 
                dmdrvi_stat_t* stat);

/* MAL interfaces - implemented by dmdevfs, called by drivers */
void dmdrvi_device_available(dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num);
void dmdrvi_device_unavailable(dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num);
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
    dmdrvi_dev_id_t major;                        ///< Major device number (channel)
    dmdrvi_dev_id_t minor;                        ///< Minor device number (specific config)
    uint8_t flags;                                ///< Device numbering flags
    char alt_name[DMDRVI_ALT_NAME_MAX_LEN + 1];  ///< Alternative file name (valid when DMDRVI_NUM_ALT_NAME is set)
} dmdrvi_dev_num_t;
```

### Device Numbering Flags

* **DMDRVI_NUM_NONE** (0x00) - Driver does not use numbering (e.g., `/dev/dmclk`)
* **DMDRVI_NUM_MAJOR** (0x01) - Driver uses major number only (e.g., `/dev/dmuart0`)
* **DMDRVI_NUM_MINOR** (0x02) - Driver uses minor number; must be combined with major (e.g., `/dev/dmspi0/0`)
* **DMDRVI_NUM_ALT_NAME** (0x04) - Driver provides an alternative file name via the `alt_name` field (e.g., `/dev/my_sensor`)

The driver manages its own namespace and assigns device numbers when creating 
a context. Each driver can independently use the same major/minor numbers without 
conflicts. When a driver uses minor numbers, the DMDRVI_NUM_MINOR flag is set 
along with DMDRVI_NUM_MAJOR (combined as DMDRVI_NUM_MAJOR | DMDRVI_NUM_MINOR).

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

**dmdrvi_open()** opens the device with the specified access flags. *dev_num*
identifies which device within the context to open - this is normally the
dev_num returned by dmdrvi_create(), but for a device announced dynamically
via dmdrvi_device_available() it is the dev_num passed to that call, since
the driver does not create a separate context per device. Returns a device
handle or NULL on error.

**dmdrvi_close()** closes a previously opened device handle and releases 
associated resources.

**dmdrvi_read()** reads up to *size* bytes from the device into *buffer*, starting
at the byte position specified by *offset*. Returns the number of bytes actually
read, or 0 on error.

**dmdrvi_write()** writes up to *size* bytes from *buffer* to the device at the
byte position specified by *offset*. Returns the number of bytes actually written,
or a negative value on error.

**dmdrvi_ioctl()** performs device-specific control operations. The *command* 
parameter specifies the operation, and *arg* provides operation-specific data. 
Returns 0 on success or an errno-compatible error code.

**dmdrvi_flush()** flushes any pending data in device buffers. Returns 0 on 
success or an errno-compatible error code.

**dmdrvi_stat()** retrieves device status information including size and mode. 
Unlike other operations, stat does not require opening the device first - it 
takes a device path parameter (similar to how POSIX stat() takes a file path 
without requiring fopen()). The path identifies which device to query (e.g., 
"/dev/dmuart0", "/dev/dmspi0/0"). Returns 0 on success or an errno-compatible 
error code.

### Dynamic Device Notifications (MAL Interface)

All functions described so far are DIF (Dmod Interface) functions: dmdevfs
(or an application) calls them, and each driver module provides its own
implementation. `dmdrvi_device_available()` and `dmdrvi_device_unavailable()`
go the opposite direction: they are MAL (Module Abstraction Layer)
functions, called *by the driver* and implemented once, by dmdevfs.

Both functions are scoped to an existing `context` - the one dmdevfs
obtained earlier from `dmdrvi_create()` - rather than to a driver name or a
fresh configuration. This matters because dmdevfs can be mounted more than
once in the filesystem; since `context` was handed out by the specific
dmdevfs instance that originally created it, a notification tied to that
context always reaches the right instance, with no separate lookup needed.
Consequently the driver does not create a new context for a dynamically
discovered device - it reuses its existing context and identifies the
device with a `dmdrvi_dev_num_t`, which must later be passed to
`dmdrvi_open()` (see above) to open that specific device.

**dmdrvi_device_available()** is called by a driver when it detects that a
new device has become available at runtime within an existing context (e.g.
a hot-plugged sub-device or a dynamically discovered channel). `dev_num`
identifies the new device (major/minor/alt_name, as usual). dmdevfs can use
this notification to expose a corresponding device file, which drivers can
later open by passing this same `dev_num` to `dmdrvi_open()`.

**dmdrvi_device_unavailable()** is the counterpart, called by a driver to
inform dmdevfs that a device previously announced (or present since the
initial `dmdrvi_create()` call) is no longer valid (e.g. the hot-plugged
sub-device was removed). dmdevfs should remove the corresponding device
file; the context itself remains valid and is only freed via
`dmdrvi_free()`.

```c
void dmdrvi_device_available(dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num);
void dmdrvi_device_unavailable(dmdrvi_context_t context, const dmdrvi_dev_num_t* dev_num);
```

### Device Status Structure

```c
typedef struct {
    uint32_t size;  //!< Size of the device/file
    uint32_t mode;  //!< Device mode (permissions)
} dmdrvi_stat_t;
```

### Network Driver Ioctl Commands

`dmdrvi_ioctl.h` defines control-plane ioctl commands intended for network
(e.g. Ethernet) drivers built on top of dmdrvi. Packet payloads are
transferred through the regular `dmdrvi_read()`/`dmdrvi_write()` calls (one
call transfers one frame); these ioctls only cover what doesn't fit that
model: MAC address configuration, link status, and interface start/stop.

| Command                            | `arg` direction | `arg` type                     | Description                    |
|-------------------------------------|------------------|----------------------------------|---------------------------------|
| `DMDRVI_IOCTL_NET_SET_MAC_ADDR`     | in               | `const dmdrvi_net_mac_addr_t*`  | Set the device MAC address      |
| `DMDRVI_IOCTL_NET_GET_MAC_ADDR`     | out              | `dmdrvi_net_mac_addr_t*`        | Read the device MAC address     |
| `DMDRVI_IOCTL_NET_GET_LINK_STATUS`  | out              | `dmdrvi_net_link_status_t*`     | Read the current link state     |
| `DMDRVI_IOCTL_NET_START`            | -                | `NULL`                          | Start the interface             |
| `DMDRVI_IOCTL_NET_STOP`             | -                | `NULL`                          | Stop the interface              |

```c
#define DMDRVI_NET_MAC_ADDR_LEN 6

typedef struct {
    uint8_t addr[DMDRVI_NET_MAC_ADDR_LEN];
} dmdrvi_net_mac_addr_t;

typedef enum {
    DMDRVI_NET_LINK_DOWN = 0,
    DMDRVI_NET_LINK_UP   = 1,
} dmdrvi_net_link_status_t;
```

The expected bring-up sequence for a network device is:

1. `dmdrvi_open()` the device.
2. `DMDRVI_IOCTL_NET_SET_MAC_ADDR` to configure the MAC address.
3. `DMDRVI_IOCTL_NET_START` to enable packet reception/transmission.
4. `DMDRVI_IOCTL_NET_GET_LINK_STATUS` to check the link before relying on it
   - dmdrvi has no event/notification mechanism for link changes, so this
     must be polled.
5. `dmdrvi_write()` / `dmdrvi_read()` to send/receive frames.
6. `DMDRVI_IOCTL_NET_STOP` before `dmdrvi_close()`, if the driver needs a
   clean shutdown of the peripheral.

```c
#include "dmdrvi.h"
#include "dmdrvi_ioctl.h"

void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR, &dev_num);

dmdrvi_net_mac_addr_t mac = { .addr = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 } };
dmdrvi_ioctl(ctx, handle, DMDRVI_IOCTL_NET_SET_MAC_ADDR, &mac);
dmdrvi_ioctl(ctx, handle, DMDRVI_IOCTL_NET_START, NULL);

dmdrvi_net_link_status_t link;
dmdrvi_ioctl(ctx, handle, DMDRVI_IOCTL_NET_GET_LINK_STATUS, &link);

if (link == DMDRVI_NET_LINK_UP) {
    uint8_t frame[64] = { /* ... */ };
    dmdrvi_write(ctx, handle, frame, sizeof(frame), 0);

    uint8_t rx_buffer[1518];
    size_t received = dmdrvi_read(ctx, handle, rx_buffer, sizeof(rx_buffer), 0);
}

dmdrvi_ioctl(ctx, handle, DMDRVI_IOCTL_NET_STOP, NULL);
dmdrvi_close(ctx, handle);
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
} else if (dev_num.flags & DMDRVI_NUM_MINOR) {
    // Device uses both major and minor (directory structure)
    Dmod_Printf("Device: /dev/dmspi%d/%d\n", dev_num.major, dev_num.minor);
} else if (dev_num.flags & DMDRVI_NUM_MAJOR) {
    // Device uses major number only
    Dmod_Printf("Device: /dev/dmuart%d\n", dev_num.major);
}

// Check if driver provides an alternative file name
if (dev_num.flags & DMDRVI_NUM_ALT_NAME) {
    Dmod_Printf("Alternative name: /dev/%s\n", dev_num.alt_name);
}

// Open device for reading and writing
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR, &dev_num);

// Write data
const char* msg = "Hello Device!\n";
size_t written = dmdrvi_write(ctx, handle, msg, strlen(msg), 0);

// Read response
char buffer[256];
size_t read = dmdrvi_read(ctx, handle, buffer, sizeof(buffer), 0);

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
void* handle = dmdrvi_open(driver, DMDRVI_O_RDWR, &dev_num);
// ... perform operations ...
dmdrvi_close(driver, handle);

// Cleanup
dmdrvi_free(driver);
dmini_destroy(config);
```

### Device Status Query

```c
// Get device status (does not require opening)
dmdrvi_stat_t stat;
int result = dmdrvi_stat(ctx, "/dev/dmuart0", &stat);

if (result == 0) {
    Dmod_Printf("Device size: %u bytes\n", stat.size);
    Dmod_Printf("Device mode: 0x%08X\n", stat.mode);
}
```

### Device Control (ioctl)

```c
// Open device
void* handle = dmdrvi_open(ctx, DMDRVI_O_RDWR, &dev_num);

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
void* clk_handle = dmdrvi_open(clk_ctx, DMDRVI_O_RDWR, &clk_num);
void* uart0_handle = dmdrvi_open(uart0_ctx, DMDRVI_O_RDWR, &uart0_num);
void* uart1_handle = dmdrvi_open(uart1_ctx, DMDRVI_O_RDWR, &uart1_num);
void* spi0_cs0_handle = dmdrvi_open(spi0_cs0_ctx, DMDRVI_O_RDWR, &spi0_cs0_num);
void* spi0_cs1_handle = dmdrvi_open(spi0_cs1_ctx, DMDRVI_O_RDWR, &spi0_cs1_num);

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

### Dynamic Device Notification (Hot-Plug)

```c
// A bus driver (e.g. USB host) has a single context for the whole
// controller and dynamically discovers sub-devices at runtime.
dmdrvi_dev_num_t bus_num;
dmdrvi_context_t bus_ctx = dmdrvi_create(NULL, &bus_num);

// A new sub-device is detected on the bus - the driver picks a dev_num
// for it (e.g. bus_num.major with a new minor) and notifies dmdevfs.
// It does NOT create a separate context for it.
dmdrvi_dev_num_t child_num = bus_num;
child_num.flags |= DMDRVI_NUM_MINOR;
child_num.minor = 0;
dmdrvi_device_available(bus_ctx, &child_num);

// dmdevfs exposes a device file for child_num and, when opened, dmdevfs/
// the application passes the same dev_num back through the shared context:
void* child_handle = dmdrvi_open(bus_ctx, DMDRVI_O_RDWR, &child_num);
// ... use child_handle ...
dmdrvi_close(bus_ctx, child_handle);

// Later, the sub-device is unplugged:
dmdrvi_device_unavailable(bus_ctx, &child_num);

// The bus context itself stays valid until the driver is torn down
dmdrvi_free(bus_ctx);
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

**Alternative File Name (DMDRVI_NUM_ALT_NAME)**
* Driver provides a human-friendly alternative name for the device file (max 32 characters)
* The `alt_name` field in `dmdrvi_dev_num_t` contains the alternative name
* Can be combined with other flags
* Example: `/dev/my_sensor` (a GPIO pin driver configured for a specific sensor)

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

## IMPLEMENTING A NETWORK DRIVER

A network driver is a regular dmdrvi driver module: a separate DMOD module
that implements the DIFs declared in `dmdrvi.h` (`_create`, `_open`,
`_close`, `_read`, `_write`, `_ioctl`, ...) using
`dmod_dmdrvi_dif_api_declaration()`, plus the `DMDRVI_IOCTL_NET_*` commands
from `dmdrvi_ioctl.h` inside its `_ioctl` implementation. It does not
implement `dmdrvi.c` itself - that file only defines the interface.

### Device numbering

An Ethernet driver typically identifies interfaces by major number only
(`DMDRVI_NUM_MAJOR`), one major number per MAC peripheral - e.g.
`/dev/dmeth0`, `/dev/dmeth1`. There's no minor-number concept needed for a
plain byte-in/byte-out network interface.

### Skeleton

```c
#define DMOD_ENABLE_REGISTRATION    ON
#include "dmdrvi.h"
#include "dmdrvi_ioctl.h"

typedef struct dmdrvi_context {
    dmdrvi_net_mac_addr_t mac;
    bool                  running;
} eth_context_t;

// Assign device numbers and allocate the context
dmod_dmdrvi_dif_api_declaration(1.0, ETH, dmdrvi_context_t, _create,
                                 (dmini_context_t config, dmdrvi_dev_num_t* dev_num))
{
    eth_context_t* ctx = Dmod_Malloc(sizeof(*ctx));
    *ctx = (eth_context_t){0};

    dev_num->major = 0;
    dev_num->flags = DMDRVI_NUM_MAJOR;    // -> /dev/dmeth0

    return (dmdrvi_context_t)ctx;
}

// Handle the network ioctl commands
dmod_dmdrvi_dif_api_declaration(1.0, ETH, int, _ioctl,
                                 (dmdrvi_context_t context, void* handle, int command, void* arg))
{
    eth_context_t* ctx = (eth_context_t*)context;

    switch (command) {
    case DMDRVI_IOCTL_NET_SET_MAC_ADDR:
        ctx->mac = *(const dmdrvi_net_mac_addr_t*)arg;
        eth_hw_set_mac_addr(ctx, ctx->mac.addr);
        return 0;

    case DMDRVI_IOCTL_NET_GET_MAC_ADDR:
        *(dmdrvi_net_mac_addr_t*)arg = ctx->mac;
        return 0;

    case DMDRVI_IOCTL_NET_GET_LINK_STATUS:
        *(dmdrvi_net_link_status_t*)arg =
            eth_hw_link_is_up(ctx) ? DMDRVI_NET_LINK_UP : DMDRVI_NET_LINK_DOWN;
        return 0;

    case DMDRVI_IOCTL_NET_START:
        eth_hw_start(ctx);
        ctx->running = true;
        return 0;

    case DMDRVI_IOCTL_NET_STOP:
        eth_hw_stop(ctx);
        ctx->running = false;
        return 0;

    default:
        return -1;    // unsupported command
    }
}

// One dmdrvi_write() call transmits one frame
dmod_dmdrvi_dif_api_declaration(1.0, ETH, size_t, _write,
                                 (dmdrvi_context_t context, void* handle,
                                  const void* buffer, size_t size, uint32_t offset))
{
    eth_context_t* ctx = (eth_context_t*)context;
    if (!ctx->running) return 0;
    return eth_hw_transmit(ctx, buffer, size);
}

// One dmdrvi_read() call receives one frame
dmod_dmdrvi_dif_api_declaration(1.0, ETH, size_t, _read,
                                 (dmdrvi_context_t context, void* handle,
                                  void* buffer, size_t size, uint32_t offset))
{
    eth_context_t* ctx = (eth_context_t*)context;
    if (!ctx->running) return 0;
    return eth_hw_receive(ctx, buffer, size);
}
```

`eth_hw_*` above stand for the peripheral-specific driver code (register
access, DMA, interrupts, ...) - dmdrvi only defines the interface shape, not
the hardware access itself.

### Design notes

- `_write()`/`_read()` map naturally to "transmit one frame" / "receive one
  frame". There is no packet-chain object to assemble as in some RTOS
  Ethernet drivers - dmdrvi already works with flat buffers, so a single
  `dmdrvi_write()`/`dmdrvi_read()` call is one frame.
- If the MAC hardware requires the address to be programmed before the
  peripheral starts, either reject `DMDRVI_IOCTL_NET_START` when no MAC
  address has been set yet, or apply the cached address as part of handling
  `DMDRVI_IOCTL_NET_START` - matching the bring-up sequence recommended above
  (`SET_MAC_ADDR` then `START`).
- Link-state changes are only observable by polling
  `DMDRVI_IOCTL_NET_GET_LINK_STATUS` - dmdrvi has no built-in event or
  notification mechanism for it. A driver that needs to push link-change
  events to upper layers has to do so through its own mechanism, outside of
  the dmdrvi interface.
- `_read()`/`_write()` returning `0` is used above for "interface not
  running"; drivers should otherwise follow the same return-value contract
  as any other dmdrvi driver (see RETURN VALUES).

## SEE ALSO

dmod(3), dmini(3), dmod_loader(1)

## AUTHOR

Patryk Kubiak

## LICENSE

MIT License - Copyright (c) 2025 Choco-Technologies
