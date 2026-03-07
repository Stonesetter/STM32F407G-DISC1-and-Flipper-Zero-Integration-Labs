/**
 * ============================================================
 * Mode 5: Custom Flipper App Mode (STUB — Not Yet Implemented)
 * ============================================================
 *
 * Planned functionality:
 *   - STM32 side of the custom Flipper Zero app integration
 *   - Structured protocol for Flipper app <-> STM32 communication
 *   - Supports all previous mode features via unified command set
 *   - Flipper app (in FlipperApp/ folder) provides GUI control
 *
 * This mode is the capstone project. The Flipper side is a custom
 * .fap application built with the Flipper Zero SDK (ufbt), installed
 * to the Flipper's SD card.
 *
 * The Flipper app will have:
 *   - A menu to select which "sub-mode" to control
 *   - Live display of sensor data from Mode 3
 *   - Button controls for GPIO from Mode 2
 *   - Status display
 */

#include "main.h"

extern void uart_print(const char *msg);
extern volatile uint8_t mode_reset_requested;

void mode5_run(void) {
    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 5: Custom Flipper App Mode\r\n");
    uart_print("  ** NOT YET IMPLEMENTED **\r\n");
    uart_print("========================================\r\n");
    uart_print("This mode will be built in Phase 3.\r\n");
    uart_print("Returning to menu...\r\n");

    mode_reset_requested = 1;
}
