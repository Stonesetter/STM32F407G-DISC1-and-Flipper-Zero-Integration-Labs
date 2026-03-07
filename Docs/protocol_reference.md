# Protocol Reference

This document describes the communication protocols used across all 5 projects.

---

## Mode 1: UART Command Console — Text Protocol

Simple human-readable text commands terminated by `\r\n` (Enter key).

### Request Format
```
<command> [arg1] [arg2]\r\n
```

### Commands

| Command | Args | Response | Example |
|---------|------|----------|---------|
| `help` | none | Prints command list | `help` |
| `status` | none | Uptime and LED states | `status` |
| `led` | `<color> <op>` | "OK" or error | `led green on` |
| `echo` | `<text>` | Echoes text back | `echo hello world` |
| `reset` | none | Returns to boot menu | `reset` |

### LED Colors
`green`, `orange`, `red`, `blue`, `all`

### LED Operations
`on`, `off`, `toggle`

---

## Mode 2: GPIO Remote Control — Binary Packet Protocol

*(To be designed when Mode 2 is built)*

### Planned Packet Format

```
  Request:  [0xAA] [CMD] [PIN] [VALUE] [CHECKSUM]
  Response: [0xBB] [CMD] [PIN] [VALUE] [CHECKSUM]

  Bytes:      1      1     1      1        1     = 5 bytes total
```

### Planned Commands

| CMD Byte | Name | Description |
|----------|------|-------------|
| 0x01 | SET | Set pin HIGH |
| 0x02 | CLEAR | Set pin LOW |
| 0x03 | TOGGLE | Toggle pin state |
| 0x04 | READ | Read a single pin |
| 0x05 | READ_ALL | Read all tracked pins |

### Checksum
XOR of all bytes before the checksum byte.

---

## Mode 3: ADC Sensor Dashboard — Streaming Text Protocol

*(To be designed when Mode 3 is built)*

### Planned Format

Commands (text, like Mode 1):
```
start           Start streaming
stop            Stop streaming
rate <ms>       Set sample interval (default 500ms)
single          Take one reading
reset           Return to menu
```

Data stream (CSV-like):
```
T:<tick_ms>,TEMP:<celsius>,ADC1:<raw_value>,V:<voltage>
T:1500,TEMP:27.3,ADC1:2048,V:1.65
T:2000,TEMP:27.4,ADC1:2051,V:1.65
```

---

## Mode 4: SPI Communication Link

*(To be designed when Mode 4 is built)*

SPI is fundamentally different from UART — data is exchanged simultaneously
in both directions, clocked by the master. The protocol here will be a
request/response pattern layered on top of SPI transfers.

---

## Mode 5: Custom Flipper App — Unified Protocol

*(To be designed when Mode 5 is built)*

Will likely combine elements of all previous protocols into a unified
command set that the Flipper app can use to control any mode.
