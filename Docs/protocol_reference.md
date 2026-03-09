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

## Mode 2: GPIO Remote Control — Dual Protocol (Text + Binary)

Mode 2 accepts both human-readable text commands and binary packets.
The STM32 auto-detects which protocol is in use: bytes starting with `0xAA`
are treated as binary packets; everything else is text.

### Text Commands

| Command | Args | Response | Example |
|---------|------|----------|---------|
| `help` | none | Prints command list | `help` |
| `set` | `<pin> <on\|off>` | Confirmation | `set green on` |
| `toggle` | `<pin>` | New state | `toggle blue` |
| `read` | `<pin>` | Pin state | `read button` |
| `read all` | none | All pin states | `read all` |
| `status` | none | Same as `read all` | `status` |
| `reset` | none | Returns to boot menu | `reset` |

**Pin names:** `green`, `orange`, `red`, `blue`, `button`, `all`
(`button` is read-only, `all` applies to all LEDs)

### Binary Packet Format

```
  Request:  [0xAA] [CMD] [PIN] [VALUE] [CHECKSUM]
  Response: [0xBB] [STATUS] [PIN] [VALUE] [CHECKSUM]

  Bytes:      1      1     1      1        1     = 5 bytes total
```

### CMD Bytes

| CMD Byte | Name | Description |
|----------|------|-------------|
| 0x01 | SET | Set pin HIGH |
| 0x02 | CLEAR | Set pin LOW |
| 0x03 | TOGGLE | Toggle pin state |
| 0x04 | READ | Read a single pin |
| 0x05 | READ_ALL | Read all pins (PIN byte ignored) |

### Pin IDs

| ID | Pin | Hardware | Type |
|----|-----|----------|------|
| 0x00 | PD12 | Green LED | Output |
| 0x01 | PD13 | Orange LED | Output |
| 0x02 | PD14 | Red LED | Output |
| 0x03 | PD15 | Blue LED | Output |
| 0x04 | PA0 | USER Button | Input (read-only) |
| 0xFF | — | All LEDs | Output |

### Response STATUS Byte

| Value | Meaning |
|-------|---------|
| 0x00 | OK — command executed successfully |
| 0x01 | ERROR — invalid pin, read-only pin, or bad checksum |

### Checksum
XOR of all preceding bytes in the packet (4 bytes before checksum).

### READ_ALL Response
The VALUE byte contains a bitmask of all pin states:
- Bit 0 = green, Bit 1 = orange, Bit 2 = red, Bit 3 = blue, Bit 4 = button

### Example Binary Exchange
```
  Turn on green LED:
    TX: AA 01 00 00 AA    (SET, pin 0, value 0, checksum)
    RX: BB 00 00 01 BA    (OK, pin 0, now HIGH, checksum)

  Read button state:
    TX: AA 04 04 00 AE    (READ, pin 4, value 0, checksum)
    RX: BB 00 04 01 BE    (OK, pin 4, HIGH=pressed, checksum)
```

---

## Mode 3: ADC Sensor Dashboard — Streaming Text Protocol

Text commands control sampling; data streams in a CSV-like format
suitable for logging or graphing.

### Commands

| Command | Args | Response | Example |
|---------|------|----------|---------|
| `help` | none | Prints command list | `help` |
| `start` | none | Begin continuous streaming | `start` |
| `stop` | none | Stop streaming, show count | `stop` |
| `single` | none | Take one reading | `single` |
| `rate` | `<ms>` | Set interval (50-10000 ms) | `rate 200` |
| `status` | none | Settings + one reading | `status` |
| `reset` | none | Returns to boot menu | `reset` |

Default sample interval: 500 ms (2 samples/sec)

### ADC Channels

| Channel | Source | Notes |
|---------|--------|-------|
| TEMP | ADC1 Ch18 — Internal temp sensor | Built-in, no wiring needed |
| EXT | ADC1 Ch1 — PA1 analog input | Optional potentiometer |

### Stream Format

One line per sample, CSV-like fields:
```
T:<tick_ms>,TEMP:<celsius>,RAW_T:<adc_raw>,EXT:<voltage>V,RAW_E:<adc_raw>
```

Example output:
```
T:1500,TEMP:27.3C,RAW_T:943,EXT:1.65V,RAW_E:2048
T:2000,TEMP:27.4C,RAW_T:944,EXT:1.64V,RAW_E:2045
T:2500,TEMP:27.3C,RAW_T:943,EXT:0.82V,RAW_E:1020
```

### Temperature Conversion

The STM32F407 internal temp sensor formula (from datasheet):
```
V_sense = (ADC_raw / 4095) * 3.3V
Temp(C) = ((0.76V - V_sense) / 0.0025) + 25
```

### Optional Wiring (Potentiometer)

To get meaningful EXT readings, connect a potentiometer:
- One outer leg to 3V3
- Other outer leg to GND
- Wiper (middle) to PA1

Turning the pot sweeps PA1 from 0V to 3.3V, giving ADC values 0-4095.
If nothing is connected to PA1, it reads floating noise (random values).

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
