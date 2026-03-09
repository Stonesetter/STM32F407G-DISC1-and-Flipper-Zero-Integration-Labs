# Project Roadmap

## Overview

5 progressive integration projects between Flipper Zero and STM32F407G Discovery.
Each builds on the skills from the previous one.

## Status Key

- [ ] Not started
- [~] In progress
- [x] Complete

---

## Phase 1: Foundation (Projects 1-2)

### Project 1: UART Command Console
- [x] Multi-mode boot menu framework (main.c)
- [x] Command parser (help, led, status, echo)
- [x] Test basic commands via Flipper USB-UART Bridge
- [x] Verify all LED controls work
- [x] Document any issues / lessons learned in JOURNAL.md
- [x] Makefile build system + flash from VS Code

### Project 2: GPIO Remote Control
- [x] Design binary packet protocol (header, command, payload, checksum)
- [x] Implement packet send/receive on STM32
- [x] Individual GPIO pin control (set, clear, toggle, read)
- [x] Bidirectional — STM32 reports button state and pin readings
- [x] Text command fallback for PuTTY testing
- [~] Test with Flipper USB-UART Bridge + PC terminal

## Phase 2: Peripherals (Projects 3-4)

### Project 3: ADC Sensor Dashboard
- [x] Configure ADC for onboard temperature sensor
- [x] Add support for external analog input (PA1)
- [x] Implement periodic sampling with configurable interval
- [x] Stream formatted readings over UART (CSV-like format)
- [x] Add start/stop/rate/single/status commands
- [~] Test live data monitoring from PC terminal

### Project 4: SPI Communication Link
- [ ] Design SPI wiring between boards (MOSI, MISO, SCK, CS)
- [ ] Configure STM32 as SPI master
- [ ] Configure Flipper as SPI slave (or vice versa)
- [ ] Implement request/response over SPI
- [ ] Compare speed and reliability vs UART
- [ ] Document SPI vs UART tradeoffs in JOURNAL.md

## Phase 3: Custom App (Project 5)

### Project 5: Custom Flipper App + Multi-Mode Dashboard
- [ ] Set up Flipper Zero SDK / ufbt build environment
- [ ] Create basic Flipper app skeleton with menu GUI
- [ ] Implement UART communication from Flipper app
- [ ] Add mode selection screen (pick which STM32 project to control)
- [ ] Add live sensor display (reads Project 3 data)
- [ ] Build .fap file and install to Flipper SD card
- [ ] Test full end-to-end integration

## Stretch Goals (Future)
- [ ] I2C sensor integration (accelerometer, OLED display)
- [ ] DMA-based UART for higher throughput
- [ ] Wireless bridge (if Flipper Sub-GHz or IR can be leveraged)
- [ ] RTOS on the STM32 (FreeRTOS) for multitasking modes
- [ ] PCB design for a permanent Flipper-STM32 breakout board

---

## Notes

- The STM32 runs a single multi-mode firmware — no reflashing to switch projects
- The Flipper uses its built-in USB-UART Bridge for Projects 1-4
- Project 5 adds a custom .fap app on the Flipper's SD card
- All wiring for UART projects is the same 3-wire setup
- SPI (Project 4) requires additional wires — see wiring_diagrams.md
