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

dmdrvi_ssize_t dmdrvi_read(dmdrvi_context_t context, void* handle,
                           void* buffer, size_t size, dmdrvi_offset_t offset);
dmdrvi_ssize_t dmdrvi_write(dmdrvi_context_t context, void* handle,
                            const void* buffer, size_t size, dmdrvi_offset_t offset);

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
at the non-negative byte position specified by *offset*. It returns the number of
bytes actually read, zero only for EOF or a zero-length request, and a negative
errno-compatible value on failure.

**dmdrvi_write()** writes up to *size* bytes from *buffer* to the device at the
byte position specified by *offset*. It returns the number of bytes actually
written or a negative errno-compatible value on failure.

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
    dmdrvi_size_t size;  //!< 64-bit size of the device/file
    uint32_t mode;       //!< Device mode (permissions)
} dmdrvi_stat_t;
```

`dmdrvi_offset_t`, `dmdrvi_size_t`, and `dmdrvi_ssize_t` are explicit 64-bit
types declared in `dmdrvi_types.h`. The signed I/O result makes EOF (`0`)
unambiguous from an error (`< 0`). A request whose byte count cannot be
represented by `dmdrvi_ssize_t` must fail with `-EOVERFLOW`.

### Block Device Ioctl Commands

Block drivers expose their geometry through
`DMDRVI_IOCTL_BLOCK_GET_INFO`. `logical_block_size` and
`erase_block_size` are byte counts, while `block_count` is the number of
logical blocks. The total capacity is therefore
`logical_block_size * block_count`, checked for overflow by the caller.

| Command | `arg` type | Meaning |
|---------|------------|---------|
| `DMDRVI_IOCTL_BLOCK_GET_INFO` | `dmdrvi_block_info_t*` | Return geometry and capability flags |
| `DMDRVI_IOCTL_BLOCK_ERASE` | `const dmdrvi_block_range_t*` | Physically erase an aligned byte range |
| `DMDRVI_IOCTL_BLOCK_DISCARD` | `const dmdrvi_block_range_t*` | Mark an aligned byte range unused; subsequent contents are unspecified |

Erase and discard are separate operations. A driver advertises support with
`DMDRVI_BLOCK_FLAG_ERASE_SUPPORTED` and
`DMDRVI_BLOCK_FLAG_DISCARD_SUPPORTED`; unsupported controls return an
errno-compatible error. Ranges use 64-bit byte offsets and lengths and must be
aligned to the device's reported requirements.

### Graphics Ioctl Commands

Any device exposing a framebuffer (LCD-TFT controller, SPI display, ...)
implements the standard `DMDRVI_IOCTL_GFX_*` commands (0x300 range), so a
generic graphics library only needs the device path: `GET_INFO` returns a
`dmdrvi_gfx_info_t` (resolution, `dmdrvi_gfx_pixel_format_t`, bytes per pixel,
stride, buffer count). The others are `GET_FRAMEBUFFER`, `SWAP_BUFFERS`,
`WAIT_VSYNC`, `FILL_RECT` (`0xAARRGGBB`, converted by the driver),
`SET/GET_DISPLAY_ENABLED` and `SET/GET_BACKLIGHT`. Reading/writing the node
accesses the drawing buffer at the given byte offset. Commands a driver cannot
honour return `-ENOTSUP`; driver-specific ones use `DMDRVI_IOCTL_CUSTOM_BASE`.

### Input Ioctl Commands

Any device a user touches, moves or presses - a touch panel, a mouse, a set
of buttons - implements the standard `DMDRVI_IOCTL_INPUT_*` commands (0x400
range). Generic code (a GUI library's input driver, a test tool) needs only
the path of the node; a new touch controller or a mouse driver is used the
same way without changing it.

| Command | `arg` type | Meaning |
|---------|------------|---------|
| `DMDRVI_IOCTL_INPUT_GET_INFO` | `dmdrvi_input_info_t*` | What the device reports |
| `DMDRVI_IOCTL_INPUT_GET_STATE` | `dmdrvi_input_state_t*` | Current state - the same as `read()` |
| `DMDRVI_IOCTL_INPUT_WAIT_EVENT` | `const uint32_t*` timeout in ms, `NULL` = forever | Block until the state changes |

`read()` of an input node returns one `dmdrvi_input_state_t` (the buffer must
be at least that large, otherwise `-EINVAL`; the offset is ignored). Driver
specific extras (chip registers, calibration) use commands from
`DMDRVI_IOCTL_CUSTOM_BASE` on; standard commands a driver cannot honour
return `-ENOTSUP`, unknown ones `-ENOTTY`.

#### Info

`dmdrvi_input_info_t` holds the device model (`name`, e.g. `"FT5336"`), its
kind (`DMDRVI_INPUT_TYPE_TOUCHSCREEN`, `_MOUSE`, `_BUTTONS`), the
`DMDRVI_INPUT_CAP_*` bits, the coordinate range of contacts (`width` x
`height`, 0 = unknown), `max_contacts` and `button_count`.

| Capability | The device |
|------------|------------|
| `DMDRVI_INPUT_CAP_CONTACTS` | Reports contacts with screen coordinates |
| `DMDRVI_INPUT_CAP_MOTION` | Reports relative motion (`dx`, `dy`) |
| `DMDRVI_INPUT_CAP_WHEEL` | Reports a scroll wheel |
| `DMDRVI_INPUT_CAP_BUTTONS` | Reports buttons |
| `DMDRVI_INPUT_CAP_PRESSURE` | Fills `pressure` of contacts |
| `DMDRVI_INPUT_CAP_CONTACT_SIZE` | Fills `size` of contacts |
| `DMDRVI_INPUT_CAP_INTERRUPT` | Wakes `WAIT_EVENT` from an interrupt (otherwise it polls) |

#### State

```c
typedef struct {
    uint16_t x, y;      /* screen coordinates */
    uint8_t  id;        /* stays the same while the contact moves */
    uint8_t  event;     /* DMDRVI_INPUT_CONTACT_DOWN / _MOVE / _UP */
    uint8_t  pressure;  /* 0 without DMDRVI_INPUT_CAP_PRESSURE */
    uint8_t  size;      /* 0 without DMDRVI_INPUT_CAP_CONTACT_SIZE */
} dmdrvi_input_contact_t;

typedef struct {
    uint32_t buttons;               /* DMDRVI_INPUT_BUTTON_* bits - bit n is button n */
    int16_t  dx, dy, wheel;         /* relative, since the previous state handed out */
    uint8_t  contact_count;         /* 0 = nothing touches */
    uint8_t  reserved;
    dmdrvi_input_contact_t contacts[DMDRVI_INPUT_MAX_CONTACTS];
} dmdrvi_input_state_t;
```

* Coordinates are screen coordinates: the driver applies its configured axis
  swap, mirroring and clipping. `x < width` and `y < height` when the range
  is known.
* A contact is reported `DOWN` when it touches, `MOVE` while it stays down
  and may be reported once `UP` when it is lifted; after that it is gone.
  A device that does not see the lift simply stops reporting the contact.
* `dx`, `dy` and `wheel` add up from one handed-out state to the next:
  `read()` / `GET_STATE` return the sum and reset it.
* Everything the device does not report, the contacts beyond
  `contact_count` and `reserved` are zero, so two states compare byte by
  byte - `dmdrvi_input_state_equal()` (inline, `dmdrvi_ioctl.h`) does that
  without libc.

#### Waiting for events

`WAIT_EVENT` returns 0 as soon as the state differs from the one handed out
last, or the device signalled an event - whichever the driver can tell. A
change that happens between two waits is not lost: the next wait returns at
once. The caller reads the state afterwards; it may equal the previous one
(the event changed nothing the state shows), so the caller compares:

```c
dmdrvi_input_state_t last = { 0 }, now;
uint32_t timeout = 1000;

while (ioctl(node, DMDRVI_IOCTL_INPUT_WAIT_EVENT, &timeout) != -ETIMEDOUT) {
    read(node, &now, sizeof(now));
    if (!dmdrvi_input_state_equal(&now, &last)) {
        handle(&now);
        last = now;
    }
}
```

#### Configuration keys

Drivers should read the common settings from their ini section under these
names, so every board file looks the same:

| Key | Meaning |
|-----|---------|
| `width`, `height` | Screen size - range and clipping of the reported coordinates |
| `swap_xy` | `on`: the device's X is the screen's Y and vice versa |
| `invert_x`, `invert_y` | `on`: mirror the axis (applied after `swap_xy`) |
| `interrupt_handler` | dmhaman handler of the device's interrupt pin (empty = poll) |
| `poll_interval_ms` | Polling period of `WAIT_EVENT` without an interrupt |

### Device File System Ioctl Commands

Commands in the 0xF00 range are answered by the file system that exposes
the node (dmdevfs) and never reach the driver. They give every module - not
only drivers, which get `dmdrvi_friend_changed()` - access to what the file
system knows about a node.

| Command | `arg` type | Meaning |
|---------|------------|---------|
| `DMDRVI_IOCTL_DEVFS_GET_FRIEND` | `dmdrvi_devfs_friend_t*` | The `index`-th other member of the node's `friends_group`: its absolute `path` and `role`; `-ENOENT` when there is none |

```c
dmdrvi_devfs_friend_t f = { .index = 0 };
while (Dmod_Ioctl(display, DMDRVI_IOCTL_DEVFS_GET_FRIEND, &f) == 0) {
    /* f.path, f.role */
    f.index++;
}
```

### Monitor Ioctl Commands

Some devices need work done over time: an SD card is inserted or pulled, a
device is plugged into a USB port, a card reader's medium changes, an
Ethernet link goes up or down. A driver must not create threads or
processes for this. Instead it declares what should trigger it, and a
monitor service - one process per monitored node, started by the system -
waits for that and calls the driver back through three class-independent
ioctls:

| Command | `arg` type | Meaning |
|---------|------------|---------|
| `DMDRVI_IOCTL_MONITOR_GET_POLICY` | `dmdrvi_monitor_policy_t*` | What should trigger the monitor for this node |
| `DMDRVI_IOCTL_MONITOR_EVENT` | `NULL` | An event arrived - react now, without blocking |
| `DMDRVI_IOCTL_MONITOR_REFRESH` | `NULL` | Settle the state and announce/withdraw nodes |

```c
#define DMDRVI_MONITOR_HANDLER_NAME_MAX 32u

typedef struct {
    char     event_handler[DMDRVI_MONITOR_HANDLER_NAME_MAX]; /* dmhaman handler, "" = none */
    uint32_t settle_ms;          /* quiet time after the last event before REFRESH */
    uint32_t poll_interval_ms;   /* periodic REFRESH, 0 = no polling */
} dmdrvi_monitor_policy_t;
```

A driver that needs no monitoring does not implement these commands and
returns `-ENOTTY`, like for any unknown command.

#### When each command is called

The monitor sleeps until an event or the poll interval wakes it:

```c
ioctl(node, DMDRVI_IOCTL_MONITOR_GET_POLICY, &policy);
register policy.event_handler with dmhaman;      /* its ISR only posts a semaphore */
ioctl(node, DMDRVI_IOCTL_MONITOR_REFRESH, NULL);  /* initial state */

for (;;) {
    if (wait(semaphore, policy.poll_interval_ms ? policy.poll_interval_ms : forever) == event) {
        do {
            ioctl(node, DMDRVI_IOCTL_MONITOR_EVENT, NULL);
            sleep(policy.settle_ms);
        } while (more events arrived);            /* bounded number of rounds */
    }
    ioctl(node, DMDRVI_IOCTL_MONITOR_REFRESH, NULL);
}
```

| Command | Called | The driver |
|---------|--------|------------|
| `GET_POLICY` | Once when the monitor starts, and again after it restarts | Fills the policy, normally from its own ini section. `-ENOTTY`: the node is not monitored |
| `EVENT` | Immediately after every event, before the settle time - possibly several times per burst (contact bounce) | Must return quickly and **must not wait for in-flight I/O**: it may run concurrently with `read`/`write` on the same context, so it must not take a lock that I/O holds for long. It may only record state or set flags - typically to make an operation on a medium that is going away fail at once instead of running into its timeouts. It never announces or withdraws nodes. Returns 0 when there is nothing to do |
| `REFRESH` | 1) once when the monitor starts - this also covers events that happened before the monitor ran; 2) after events, once `settle_ms` passed without a new one; 3) every `poll_interval_ms` when non-zero | May block and is serialized with I/O. Determines the actual state (probe, identify, verify, detach) and is the only place that calls `dmdrvi_device_available()` / `dmdrvi_device_unavailable()`. Returns 0 when something is attached behind the node, `-ENODEV` when nothing is, another negative errno value on failure |

All three are called from thread context only, never from an interrupt.
`EVENT` has no meaning without `event_handler`; with neither an event
handler nor a poll interval, the monitor calls `REFRESH` once and exits.

#### Event source

An event is a call of the dmhaman handler named in `event_handler`. The
monitor registers that handler; it runs in interrupt context and only wakes
the monitor. Who fires it does not matter:

* another driver - for example dmgpio calling its `interrupt_handler` on a
  card detect edge, with the GPIO device in the same friends group;
* the driver's own ISR - for example a USB host controller calling
  `dmhaman_call_handler()` on a port-change interrupt.

#### Examples

| Driver | `event_handler` | `poll_interval_ms` | `EVENT` | `REFRESH` |
|--------|-----------------|--------------------|---------|-----------|
| SD host with a card detect pin | dmgpio edge interrupt of the pin | 0 | Sample the pin without the I/O lock; if the slot is empty, flag the card as removed so a running transfer aborts with `-ENODEV` | Identify, verify (CMD13) or detach the card; announce/withdraw the card node |
| SD host without card detect | `""` | e.g. 1000 | - | Same as above, periodically |
| USB host controller | port-change interrupt, fired by the driver's own ISR | 0 | Remember which ports changed | Enumerate new devices, remove disconnected ones (control transfers with delays - hence not in the ISR) |
| USB mass-storage card reader | `""` | e.g. 1000 | - | TEST UNIT READY; announce/withdraw the medium |
| Ethernet PHY | PHY interrupt, or `""` | 0 or e.g. 1000 | - | Read the link state |

#### Configuration keys

The policy belongs to the device's configuration. Drivers should read it
from their ini section under these names, so every board file looks the
same:

| Key | Policy field |
|-----|--------------|
| `monitor_event_handler` | `event_handler` |
| `monitor_settle_ms` | `settle_ms` |
| `poll_interval_ms` | `poll_interval_ms` |

#### Implementing the commands

```c
dmod_dmdrvi_dif_api_declaration(2.0, mydrv, int, _ioctl,
    ( dmdrvi_context_t ctx, void* handle, int command, void* arg ))
{
    switch (command)
    {
        case DMDRVI_IOCTL_MONITOR_GET_POLICY:
            *(dmdrvi_monitor_policy_t*)arg = ctx->policy;   /* read in _create */
            return 0;
        case DMDRVI_IOCTL_MONITOR_EVENT:
            /* no I/O lock here */
            if (!medium_present(ctx)) { ctx->removal_pending = true; }
            return 0;
        case DMDRVI_IOCTL_MONITOR_REFRESH:
        {
            lock(ctx);
            int ret = rescan(ctx);   /* calls dmdrvi_device_available/_unavailable */
            unlock(ctx);
            return ret;
        }
        /* ... */
    }
    return -ENOTTY;
}
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
    dmdrvi_ssize_t received = dmdrvi_read(ctx, handle, rx_buffer, sizeof(rx_buffer), 0);
}

dmdrvi_ioctl(ctx, handle, DMDRVI_IOCTL_NET_STOP, NULL);
dmdrvi_close(ctx, handle);
```

## RETURN VALUES

Functions return values as follows:

* **dmdrvi_create()** - Context pointer on success, NULL on error
* **dmdrvi_open()** - Device handle on success, NULL on error
* **dmdrvi_read()/write()** - Non-negative byte count, or a negative errno-compatible error; `0` from read means EOF
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
dmdrvi_ssize_t written = dmdrvi_write(ctx, handle, msg, strlen(msg), 0);

// Read response
char buffer[256];
dmdrvi_ssize_t read = dmdrvi_read(ctx, handle, buffer, sizeof(buffer), 0);

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
    Dmod_Printf("Device size: %llu bytes\n", (unsigned long long)stat.size);
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
#include <errno.h>

typedef struct dmdrvi_context {
    dmdrvi_net_mac_addr_t mac;
    bool                  running;
} eth_context_t;

// Assign device numbers and allocate the context
dmod_dmdrvi_dif_api_declaration(2.0, ETH, dmdrvi_context_t, _create,
                                 (dmini_context_t config, dmdrvi_dev_num_t* dev_num))
{
    eth_context_t* ctx = Dmod_Malloc(sizeof(*ctx));
    *ctx = (eth_context_t){0};

    dev_num->major = 0;
    dev_num->flags = DMDRVI_NUM_MAJOR;    // -> /dev/dmeth0

    return (dmdrvi_context_t)ctx;
}

// Handle the network ioctl commands
dmod_dmdrvi_dif_api_declaration(2.0, ETH, int, _ioctl,
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
dmod_dmdrvi_dif_api_declaration(2.0, ETH, dmdrvi_ssize_t, _write,
                                 (dmdrvi_context_t context, void* handle,
                                  const void* buffer, size_t size, dmdrvi_offset_t offset))
{
    eth_context_t* ctx = (eth_context_t*)context;
    if (!ctx->running) return -EIO;
    return eth_hw_transmit(ctx, buffer, size);
}

// One dmdrvi_read() call receives one frame
dmod_dmdrvi_dif_api_declaration(2.0, ETH, dmdrvi_ssize_t, _read,
                                 (dmdrvi_context_t context, void* handle,
                                  void* buffer, size_t size, dmdrvi_offset_t offset))
{
    eth_context_t* ctx = (eth_context_t*)context;
    if (!ctx->running) return -EIO;
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
- Upper layers read the link state with
  `DMDRVI_IOCTL_NET_GET_LINK_STATUS`; dmdrvi does not push link-change
  events to them. If the driver itself has to follow the PHY over time
  (poll it, or service a PHY interrupt in thread context), it implements the
  monitor commands (see "Monitor Ioctl Commands") instead of creating a
  thread.
- `_read()`/`_write()` must return a negative errno-compatible value when the
  interface is not running. Zero from `_read()` is reserved for EOF/no frame
  and zero from `_write()` is only valid for a zero-length request.

## VERSION 2.0 MIGRATION

Version 2.0 is intentionally ABI-incompatible with 1.x. Driver DIF
implementations must change their declaration version to `2.0`, replace
`uint32_t` offsets with `dmdrvi_offset_t`, and return `dmdrvi_ssize_t` from
read/write. Callers must handle negative results before converting a byte count
to `size_t`. The MAL device-availability callbacks remain at version 1.0 because
their signatures did not change.

## VERSION 2.1

Adds the monitor ioctl commands (`DMDRVI_IOCTL_MONITOR_*`,
`dmdrvi_monitor_policy_t`). The change is purely additive: no DIF or MAL
signature changed, drivers keep their `2.0` declarations, and a driver that
does not implement the new commands keeps answering `-ENOTTY`.

## SEE ALSO

dmod(3), dmini(3), dmod_loader(1)

## AUTHOR

Patryk Kubiak

## LICENSE

MIT License - Copyright (c) 2025 Choco-Technologies
