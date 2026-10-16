# RTC device contract

RTC devices expose `DMDRVI_IOCTL_RTC_*` from `dmdrvi_ioctl.h`, independent of
MCU, transport and driver name. Clients open a configured device path through
VFS; they do not depend on dmrtc or its context lifecycle. GET_TIME, GET_STATUS
and GET_INFO are mandatory. SET_TIME and INVALIDATE are optional capabilities.

The value is a Gregorian civil calendar, with no implicit timezone. It is not a
Unix timestamp, libc `struct tm`, or evidence of authentic UTC. The generic year
range is 1–9999; GET_INFO reports the narrower physical range of each device.
Weekday is computed by the driver. Leap seconds are not encoded. Reserved
fields are zero and permit deterministic validation. Structures use fixed-width
fields and have the same documented layout on 32/64-bit supported targets.

`resolution_ns` reports representable granularity, not accuracy or trust. A
SET_TIME request outside the supported year/precision range returns -ERANGE;
invalid Gregorian dates return -EINVAL. No silent truncation is allowed. Input
validation happens before hardware mutation; read failures clear the output.

TIME_SET is an initialized usable calendar; RUNNING reports clock operation.
TIME_LOST is a latched hardware-detected loss of continuity, meaningful only
when LOSS_DETECTION is advertised. A clear TIME_LOST on an RTC without that
capability gives no assurance of uninterrupted operation. INVALIDATE only clears
TIME_SET; successful SET_TIME clears a reported TIME_LOST. No flag attests UTC
accuracy or authenticity. A malformed/lost/unset clock must not supply a usable
GET_TIME snapshot.

A future dmutc layer selects the device, interprets/configures its calendar as
UTC, converts to epoch time, synchronizes from other sources and owns trust
policy. Network/GNSS synchronization sources do not have to emulate RTC devices.
Hardware-specific oscillator configuration stays in each driver's port/config.

Read/write handle access is enforced for ioctls. Unknown commands return
-ENOTTY; recognized unsupported optional commands return -ENOTSUP. Negative
hardware/timeout errors propagate. See the header for pointer types and flags.
