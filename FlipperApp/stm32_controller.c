/**
 * ============================================================
 * STM32 Controller — Custom Flipper Zero App (STUB)
 * ============================================================
 *
 * This will be a custom Flipper Zero application that provides
 * a GUI to control and monitor the STM32F407G Discovery board.
 *
 * Planned features:
 *   - Mode selection menu (mirrors the STM32 boot menu)
 *   - LED control buttons
 *   - Live sensor data display
 *   - SPI communication option
 *
 * Built with the Flipper Zero SDK using ufbt.
 * Install: copy the .fap file to /ext/apps/GPIO/ on the Flipper's SD card.
 *
 * This file is a placeholder — will be implemented in Project 5.
 */

#include <furi.h>
#include <gui/gui.h>

/* Placeholder entry point */
int32_t stm32_controller_app(void *p) {
    UNUSED(p);

    /* TODO: Implement in Project 5
     *
     * 1. Open UART connection (pins 13, 14 at 115200)
     * 2. Show mode selection menu
     * 3. Send commands to STM32
     * 4. Display responses on Flipper screen
     */

    return 0;
}
