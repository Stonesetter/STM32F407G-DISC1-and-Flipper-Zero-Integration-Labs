# Wiring Diagrams

## Flipper Zero GPIO Header Reference

Looking at the Flipper from the top, pins left to right:

```
  Pin:  1    2    3    4    5    6    7    8    9
        5V   A7   A6   A4   B3   B2   C3   GND  3V3

  Pin:  10   11   12   13   14   15   16   17   18
        SWC  GND  SIO  TX   RX   C1   C0   1W   GND
```

## STM32F407G-DISC1 Key Pins

```
  Onboard LEDs:
    PD12 = Green    PD13 = Orange
    PD14 = Red      PD15 = Blue

  USER Button: PA0

  USART2:  PA2 = TX,  PA3 = RX
  SPI1:    PA5 = SCK, PA6 = MISO, PA7 = MOSI
  ADC1:    PA1 = Channel 1 (analog input)
```

---

## Projects 1-3 & 5: UART Wiring (3 wires)

This is the base wiring used for most projects.

```
  STM32 Discovery          Flipper Zero
  ================         ============
  PA2 (USART2 TX) -------> Pin 14 (RX)
  PA3 (USART2 RX) <------- Pin 13 (TX)
  GND            ---------- Pin 8 or 18 (GND)
```

**Notes:**
- TX connects to RX and vice versa (crossover) — this is correct
- Both boards are 3.3V logic — no level shifter needed
- Use female-to-female jumper wires
- Always connect GND first before data lines

**Flipper settings:** GPIO -> USB-UART Bridge -> Baud 115200 -> Pins 13,14

---

## Project 3 (optional): Potentiometer for ADC

Add a potentiometer for analog input testing.

```
  Potentiometer                STM32 Discovery
  =============                ================
  Left pin  ---- 3V3 (from Discovery or Flipper Pin 9)
  Wiper     ---- PA1 (ADC1 Channel 1)
  Right pin ---- GND

  (Plus the 3 UART wires from above)
```

**Note:** The STM32 also has an internal temperature sensor on ADC1 Channel 16.
Mode 3 can read that without any extra wiring.

---

## Project 4: SPI Wiring (4 additional wires)

SPI requires the UART wires (for the menu and fallback) PLUS 4 SPI wires.

```
  STM32 Discovery          Flipper Zero
  ================         ============

  UART (keep these connected):
  PA2 (USART2 TX) -------> Pin 14 (RX)
  PA3 (USART2 RX) <------- Pin 13 (TX)
  GND            ---------- Pin 8 or 18 (GND)

  SPI (add these):
  PA5 (SPI1 SCK)  -------> Pin 5  (B3 / SCK)
  PA7 (SPI1 MOSI) -------> Pin 3  (A6 / MOSI)
  PA6 (SPI1 MISO) <------- Pin 4  (A4 / MISO)
  PB6 (GPIO / CS)  ------> Pin 6  (B2 / CS)
```

**Total wires for Project 4:** 7 (3 UART + 4 SPI)

**SPI concepts:**
- SCK = Clock (master controls timing)
- MOSI = Master Out Slave In (STM32 sends data)
- MISO = Master In Slave Out (Flipper sends data)
- CS = Chip Select (active low — pulled low to start a transaction)

---

## Wire Color Suggestions

Using consistent colors makes debugging much easier:

| Wire | Suggested Color | Why |
|------|----------------|-----|
| GND | Black | Standard convention |
| TX -> RX | Yellow | Data going out |
| RX <- TX | Green | Data coming in |
| SCK | Blue | Clock signal |
| MOSI | Orange | Master out |
| MISO | White | Master in |
| CS | Red | Chip select / attention |
