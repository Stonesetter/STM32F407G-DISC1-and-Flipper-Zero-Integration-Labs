# Learning Journal

A progress journal tracking what we're building, what we learned, and what
concepts matter for hardware engineering. Written for a CS student transitioning
into hardware/embedded systems.

---

## Entry 1 — Project Setup & The Big Picture

**Date:** 2026-03-06
**Status:** Starting Project 1

### What We're Doing

We're building a collection of 5 projects that make a Flipper Zero and an
STM32F407G Discovery board work together. The goal is to learn embedded systems
and hardware engineering concepts progressively — starting with basic serial
communication and building up to writing custom firmware apps.

### Why These Two Devices?

**STM32F407G Discovery** — This is a real embedded development board with an
ARM Cortex-M4 processor. It's the same family of chips used in industrial
controllers, drones, medical devices, and automotive systems. Learning STM32
is directly applicable to hardware engineering careers.

**Flipper Zero** — Acts as our Swiss Army knife for hardware interaction. It
has GPIO pins, UART, SPI, I2C, and more. Instead of needing a bunch of separate
test equipment, the Flipper can act as a serial terminal, logic probe, or even
a custom controller for the STM32.

### Key Concept: UART (Universal Asynchronous Receiver-Transmitter)

UART is the simplest way two devices can talk to each other. It uses just 2
data wires (TX and RX) plus a ground connection.

**How it works:**
- Data is sent one bit at a time (serial), at a pre-agreed speed (baud rate)
- Both sides must agree on the baud rate (we use 115200 bits/second)
- TX (transmit) on one device connects to RX (receive) on the other
- There's no clock wire — both sides just have to agree on timing (that's the
  "asynchronous" part)
- Each byte is framed with a start bit, 8 data bits, and a stop bit (8N1)

**Why it matters:**
- UART is everywhere — GPS modules, Bluetooth chips, sensors, debug consoles
- It's the first protocol you learn in embedded systems
- Understanding UART gives you the foundation for SPI and I2C later

### Key Concept: Multi-Mode Firmware

Instead of flashing a different program for each project, we built a single
firmware that contains all 5 projects as "modes." On boot, it shows a menu
and lets you pick which mode to run.

**Why this matters in the real world:**
- Production devices often have diagnostic/test modes alongside normal operation
- Bootloaders work on a similar principle (run a menu, then jump to user code)
- It teaches you about function pointers and program architecture in C

### Architecture Decision: Why One Binary?

When you flash firmware to an STM32, you need:
1. A USB cable connected to a computer
2. STM32CubeIDE (or ST-Link utility) running
3. A few seconds to erase and rewrite flash memory

That's annoying if you want to quickly switch between demo projects. By putting
all projects in one binary with a selection menu, you can switch modes just by
resetting the board and pressing a button — no computer needed.

### Hardware Concepts Encountered

| Concept | What It Is | Where We Used It |
|---------|-----------|-----------------|
| GPIO | General Purpose Input/Output — pins you can set high/low or read | LEDs (PD12-15), USER button (PA0) |
| UART | Serial communication protocol | PA2 (TX) and PA3 (RX) to Flipper |
| Interrupts | Hardware signals that pause normal code to handle an event | UART receive triggers a callback |
| HAL | Hardware Abstraction Layer — ST's library for controlling peripherals | All peripheral init and control |
| Baud Rate | Speed of serial communication in bits/second | 115200 for our UART link |
| Pull-up/Pull-down | Resistors that define a pin's default state | UART RX pin uses pull-up |

---

## Entry 2 — Project 1: UART Command Console

**Date:** 2026-03-06
**Status:** Building

### What We're Building

A command-line interface (CLI) that runs on the STM32 and is accessible through
the Flipper Zero's UART bridge. You type commands in a terminal on your PC,
they travel through USB to the Flipper, through UART to the STM32, get parsed,
and the STM32 responds.

```
  [PC Terminal] --USB--> [Flipper Zero] --UART--> [STM32 Discovery]
       ^                                               |
       |                                               |
       +----------<---UART---<---USB---<---------------+
```

### Commands Available

| Command | What It Does |
|---------|-------------|
| `help` | Shows list of all commands |
| `status` | Reports uptime, mode, and LED states |
| `led <color> <on/off/toggle>` | Controls the 4 onboard LEDs |
| `echo <text>` | Echoes back whatever you type |
| `reset` | Jumps back to mode selection menu |

### What You Learn From This

1. **String parsing in C** — Embedded systems rarely have fancy libraries.
   You learn to tokenize and compare strings manually with `strcmp`, `strtok`.
2. **State management** — Tracking which LEDs are on/off, uptime, etc.
3. **Command pattern** — This is the same architecture used in AT commands
   (modems), G-code (3D printers), and SCPI (test equipment).

### New Concepts

| Concept | Explanation |
|---------|------------|
| Ring buffer | A circular array for storing incoming UART bytes without losing data |
| String tokenization | Splitting "led green on" into ["led", "green", "on"] |
| Volatile keyword | Tells the compiler a variable can change outside normal program flow (e.g., in an interrupt) |
| Function pointers | Used in our mode system — each mode is a function, we store a pointer to the active one |

---

## Entry 3 — Bugs and Debugging (Project 1 Hardware Test)

**Date:** 2026-03-06
**Status:** Project 1 COMPLETE

### Bug 1: Board Frozen on Boot — Missing SysTick_Handler

**Symptom:** Firmware flashed successfully but the board did nothing — no LED
flash, no UART output, no response to buttons.

**Root cause:** We forgot to define `SysTick_Handler()`. The HAL uses SysTick
(a hardware timer that fires every 1ms) for timekeeping — `HAL_Delay()`,
`HAL_GetTick()`, and internal timeouts all depend on it. Without the handler,
the first SysTick interrupt jumped to `Default_Handler` (an infinite loop in
the startup assembly), freezing the board instantly.

**Fix:** Added `SysTick_Handler` that calls `HAL_IncTick()`:
```c
void SysTick_Handler(void) {
    HAL_IncTick();
}
```

**Lesson:** On bare metal, there's no OS to set up interrupt handlers for you.
Every interrupt the hardware generates needs a handler — or at minimum, the
startup code's weak default handler catches it (which just loops forever).
SysTick is especially critical because HAL depends on it internally.

### Bug 2: UART Receive Not Working — Interrupt Callback Issue

**Symptom:** The STM32 could transmit (we saw the boot menu) but never
responded to keyboard input.

**Root cause:** We used `HAL_UART_Receive_IT()` (interrupt-based receive) but
the callback architecture was wrong. HAL only calls one global callback:
`HAL_UART_RxCpltCallback()`. Mode 1 defined its own function
`mode1_rx_callback()` which HAL never called.

**Fix:** Switched to polling with `HAL_UART_Receive()` using a short 10ms
timeout. Simpler, more reliable, and no callback conflicts between modes.

**Lesson:** Interrupt-based UART receive is more efficient but adds complexity.
For a learning project at 115200 baud, polling works perfectly fine. We can
revisit interrupts later when we need higher performance (Mode 4: SPI).

### Concepts Reinforced

| Concept | What We Learned |
|---------|----------------|
| Interrupt handlers | Must be defined or the default handler freezes the MCU |
| SysTick | The heartbeat of the HAL — increment every 1ms, required for delays |
| Polling vs Interrupts | Polling is simpler; interrupts are efficient but need careful architecture |
| Weak symbols | Startup code declares handlers as "weak" — your definitions override them |
| Build-flash-test cycle | `make` to build, `make flash` to deploy, reset to test |

---

## Entry 4 — Project 2: GPIO Remote Control

**Date:** 2026-03-06
**Status:** Code complete, needs hardware testing

### What We Built

A GPIO remote control system that lets you control the STM32's LEDs and read
the USER button from a remote terminal. The big new concept here is **dual
protocol support** — the same mode accepts both human-readable text commands
AND a binary packet protocol.

### Why Two Protocols?

In the real world, embedded devices often need to talk to both humans and
machines. Think about it:
- **Text protocol** — Great for debugging, manual testing, and development.
  You open a serial terminal and type commands. Easy to understand and test.
- **Binary protocol** — What real devices use to talk to each other. Compact
  (5 bytes vs ~20 chars), fast to parse (no string comparison), and includes
  error detection (checksums).

Mode 2 auto-detects which protocol is being used: if the first byte is `0xAA`,
it treats the incoming data as a binary packet. Otherwise, it's a text command.

### The Binary Packet Protocol

```
  Request:  [0xAA] [CMD] [PIN] [VALUE] [CHECKSUM]
  Response: [0xBB] [STATUS] [PIN] [VALUE] [CHECKSUM]
```

This 5-byte packet format teaches several important concepts:

1. **Header byte (0xAA)** — Also called a "magic number" or "sync byte." It
   marks the start of a packet. Without it, the receiver wouldn't know where
   one message ends and the next begins. The value 0xAA (10101010 in binary)
   is common because it creates a distinctive alternating pattern.

2. **Command byte** — Encodes what action to take. Using a single byte instead
   of a text string ("set", "toggle") saves bandwidth and parsing time.

3. **Checksum** — XOR of all preceding bytes. This catches transmission errors.
   If a byte gets corrupted during transmission, the checksum won't match and
   the STM32 rejects the packet. Real protocols use CRC for better detection,
   but XOR is simple and teaches the concept.

### Pin Mapping

| ID | Hardware | Notes |
|----|----------|-------|
| 0 | PD12 Green LED | Output |
| 1 | PD13 Orange LED | Output |
| 2 | PD14 Red LED | Output |
| 3 | PD15 Blue LED | Output |
| 4 | PA0 USER Button | Input, read-only |

### New Concepts

| Concept | Explanation |
|---------|------------|
| Binary protocol | Sending raw bytes instead of text — compact and machine-friendly |
| Sync/Header byte | A known byte value that marks the start of a packet |
| XOR checksum | Simple error detection — XOR all bytes, compare with expected |
| Pin abstraction | Using a struct array to map pin IDs to hardware, making the code generic |
| Dual protocol | Accepting both text and binary input on the same channel |
| Bitmask | Using individual bits to represent multiple pin states in one byte (READ_ALL) |

### Testing Plan

1. Flash firmware (`make flash` or VS Code Flash task)
2. Start Flipper UART Bridge, open PuTTY on the Flipper's COM port
3. Select Mode 2 from boot menu (press '2')
4. Test text commands: `help`, `set green on`, `toggle blue`, `read button`, `status`
5. Press the USER button and run `read button` to verify it reads HIGH
6. Type `reset` to return to menu

---

## Entry 5 — Project 3: ADC Sensor Dashboard

**Date:** 2026-03-06
**Status:** Code complete, needs hardware testing

### What We Built

A sensor dashboard that reads analog signals and streams them over UART.
It reads two channels: the STM32's **internal temperature sensor** (no wiring
needed!) and an **external analog input on PA1** (optional potentiometer).

### Key Concept: ADC (Analog-to-Digital Converter)

This is a big one for hardware engineering. The real world is analog — voltage,
temperature, pressure, light, sound are all continuous signals. But a processor
only understands digital numbers (0s and 1s). The ADC bridges this gap.

**How it works:**
- The ADC measures a voltage on a pin (0V to 3.3V on the STM32)
- It converts that voltage to a number: 0 = 0V, 4095 = 3.3V (12-bit resolution)
- 12-bit means 2^12 = 4096 possible values
- Resolution: 3.3V / 4096 = 0.0008V (0.8mV) per step — pretty precise!

**Why 12-bit?** More bits = more precision. An 8-bit ADC would only give 256
steps (12.9mV resolution). The STM32F407 has a 12-bit ADC, which is standard
for most microcontrollers. High-end audio/measurement systems use 16-bit or
even 24-bit ADCs.

### Internal Temperature Sensor

The STM32 has a temperature sensor built right into the chip die. It's connected
to ADC channel 18 (not a physical pin — it's internal).

**Conversion formula (from the STM32F407 datasheet):**
```
V_sense = (ADC_raw / 4095) * 3.3V
Temperature(C) = ((0.76V - V_sense) / 0.0025) + 25
```

The numbers 0.76V and 0.0025 come straight from the datasheet — they're
calibration values specific to this chip family. In production, you'd use
factory calibration values stored in flash for better accuracy.

Note: this reads the **die** temperature (the chip itself), not room temperature.
It'll typically read a few degrees higher than ambient because the chip generates heat.

### External Analog Input

PA1 is configured as an analog input. You can connect anything that produces
a 0-3.3V signal:
- **Potentiometer** — the simplest test. Turn the knob, watch the voltage change
- **Light sensor (LDR)** — voltage varies with brightness
- **Flex sensor** — voltage varies with bending
- Or leave it disconnected — it'll read random floating noise (fun to watch!)

### Sampling and Streaming

The mode uses **polling-based** ADC reads — when it's time to sample, it:
1. Configures the ADC for the temperature channel
2. Starts conversion, waits for completion, reads the value
3. Reconfigures for the PA1 channel
4. Starts conversion, waits, reads
5. Formats both readings into a CSV-like string and sends over UART

The `rate` command controls how often this happens (50ms to 10000ms).

### New Concepts

| Concept | Explanation |
|---------|------------|
| ADC | Converts analog voltages to digital numbers |
| 12-bit resolution | 4096 discrete levels between 0V and 3.3V |
| ADC channel | Which input the ADC reads (pin or internal sensor) |
| Sample time | How long the ADC measures before converting — longer = more accurate |
| Internal temp sensor | Temperature sensor built into the chip die, no wiring needed |
| Analog input mode | GPIO pin configured to read voltage instead of digital HIGH/LOW |
| Streaming protocol | Continuously sending data at a set interval (vs request/response) |

### Testing Plan

1. Flash firmware (you'll do this yourself this time!)
2. Start Flipper UART Bridge, open PuTTY
3. Select Mode 3 from boot menu (press '3')
4. It shows an initial reading right away
5. Try: `help`, `single`, `start`, `stop`, `rate 200`, `status`
6. Watch the temperature reading — it should be roughly room temp (maybe a bit higher)
7. If you have a potentiometer, connect it to PA1 and watch EXT change as you turn it
8. Type `reset` to return to menu

---

*Future entries will be added as we complete each project.*
