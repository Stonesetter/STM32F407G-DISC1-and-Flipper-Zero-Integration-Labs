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

*Future entries will be added as we complete each project.*
