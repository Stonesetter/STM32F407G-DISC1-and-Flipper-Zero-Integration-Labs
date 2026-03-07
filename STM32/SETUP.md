# STM32CubeIDE Project Setup

Step-by-step instructions to get this firmware running on your STM32F407G-DISC1.

## 1. Create a New Project

1. Open **STM32CubeIDE**
2. **File** -> **New** -> **STM32 Project**
3. In the **Board Selector** tab, search for **STM32F407G-DISC1**
4. Select it and click **Next**
5. Name the project `flipper_stm32_labs`
6. Keep defaults (C project, Executable, etc.) and click **Finish**
7. When asked to initialize peripherals in default mode, click **Yes**

## 2. Configure Peripherals (.ioc file)

The .ioc file should open automatically. Configure these:

### USART2 (our UART link to Flipper)
1. In the **Pinout & Configuration** tab, left panel:
   - **Connectivity** -> **USART2**
   - Mode: **Asynchronous**
2. In the **Parameter Settings**:
   - Baud Rate: **115200**
   - Word Length: **8 Bits**
   - Parity: **None**
   - Stop Bits: **1**
3. Verify pins: **PA2** = USART2_TX, **PA3** = USART2_RX

### NVIC (Interrupts)
1. Left panel: **System Core** -> **NVIC**
2. Enable: **USART2 global interrupt** (check the box)

### GPIO (LEDs and Button)
These should already be configured since you selected the Discovery board,
but verify:
- **PD12** = GPIO_Output (green LED)
- **PD13** = GPIO_Output (orange LED)
- **PD14** = GPIO_Output (red LED)
- **PD15** = GPIO_Output (blue LED)
- **PA0** = GPIO_Input (USER button)

### Save to Generate Code
- Press **Ctrl+S** or click the gear icon
- Click **Yes** to generate code

## 3. Add the Source Files

### Option A: Replace main.c (simplest)
1. Copy `STM32/main.c` from this repo
2. Replace the contents of `Core/Src/main.c` in your CubeIDE project
3. Copy all files from `STM32/modes/` into `Core/Src/` in your project

### Option B: Add as additional source files
1. Keep the CubeMX-generated `main.c` for peripheral init only
2. Add our `main.c` and mode files as separate source files
3. Remove duplicate function definitions (HAL_UART_MspInit, USART2_IRQHandler, etc.)
   from the generated files since ours include them

**Option A is recommended** — it's simpler and everything is self-contained.

### Important: Handle Duplicate Definitions
If CubeMX generated `stm32f4xx_hal_msp.c` and `stm32f4xx_it.c`, they may
contain `HAL_UART_MspInit()` and `USART2_IRQHandler()` which conflict with
our main.c. Either:
- **Delete those functions** from the generated files, OR
- **Delete those functions** from our main.c and let the generated ones stay

## 4. Build and Flash

1. Click the **hammer icon** (Build) or press **Ctrl+B**
2. Fix any errors (usually duplicate definitions — see above)
3. Click the **green play button** (Debug) or press **F11**
4. The board will be flashed and start running

## 5. Connect and Test

1. Wire up the Flipper Zero (see README.md for wiring)
2. On the Flipper: **GPIO** -> **USB-UART Bridge**
   - Press LEFT for settings
   - Baud Rate: **115200**
   - UART Pins: **13,14**
   - Press BACK to start the bridge
3. Connect Flipper to PC via USB
4. Open a serial terminal (PuTTY, minicom, etc.) on the Flipper's COM port
   - Settings: 115200 baud, 8N1
5. Reset the Discovery board (black button) — you should see the mode menu

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Multiple definition of..." errors | Remove duplicate functions from CubeMX-generated files (see step 3) |
| No output in terminal | Check TX/RX wiring, verify baud rate matches on both sides |
| Garbled text | Baud rate mismatch — both sides must be 115200 |
| Red LED blinking forever | Error_Handler was called — rebuild and reflash |
| Can't find COM port | Try a different USB cable, check Device Manager |

## Adding Future Modes

When we build out modes 2-5:
1. The mode `.c` file goes in `Core/Src/`
2. Uncomment the corresponding `extern void modeN_run()` in main.c
3. Uncomment the `case N:` in the switch statement in main()
4. Add any new peripheral init (ADC, SPI, etc.) as needed
5. Rebuild and flash — that's it
