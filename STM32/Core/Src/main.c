/**
 * ============================================================
 * Flipper-STM32 Integration Labs — Multi-Mode Firmware
 * ============================================================
 *
 * This is the main firmware for all 5 integration projects.
 * On boot, it displays a mode selection menu over UART. The user
 * picks a mode by sending '1'-'5' or pressing the USER button
 * to cycle through modes.
 *
 * WIRING (UART — Projects 1-3, 5):
 *   PA2 (USART2 TX)  -->  Flipper Pin 14 (RX)
 *   PA3 (USART2 RX)  <--  Flipper Pin 13 (TX)
 *   GND              ---  Flipper Pin 8 or 18 (GND)
 *
 * ADDITIONAL WIRING (SPI — Project 4):
 *   See Docs/wiring_diagrams.md
 *
 * FLIPPER SETUP:
 *   GPIO -> USB-UART Bridge -> Baud 115200 -> Pins 13,14
 *
 * BUILD:
 *   See STM32/SETUP.md for CubeIDE instructions.
 *   This file replaces the auto-generated Core/Src/main.c
 *   Mode source files (modes/*.c) should also be added to the project.
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- Peripheral handles ---- */
UART_HandleTypeDef huart2;

/* ---- Shared globals (used by mode files) ---- */
volatile uint8_t mode_reset_requested = 0;

/* ---- Forward declarations: mode entry points ---- */
extern void mode1_run(void);
/* extern void mode2_run(void);  — uncomment as modes are built */
/* extern void mode3_run(void); */
/* extern void mode4_run(void); */
/* extern void mode5_run(void); */

/* ---- Private function prototypes ---- */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

/* ---- Shared UART helpers (used by all modes) ---- */

void uart_print(const char *msg) {
    HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}

void uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_print(buf);
}

/* ---- Mode selection menu ---- */

static uint8_t menu_rx_byte;
static volatile uint8_t menu_rx_ready = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        menu_rx_ready = 1;
    }
}

#define NUM_MODES 5

static const char *mode_names[NUM_MODES] = {
    "UART Command Console",
    "GPIO Remote Control",
    "ADC Sensor Dashboard",
    "SPI Communication Link",
    "Custom Flipper App Mode"
};

static void print_menu(uint8_t selected) {
    uart_print("\r\n\r\n");
    uart_print("========================================\r\n");
    uart_print("  Flipper-STM32 Integration Labs\r\n");
    uart_print("  Select a mode:\r\n");
    uart_print("----------------------------------------\r\n");

    for (int i = 0; i < NUM_MODES; i++) {
        if (i == selected) {
            uart_printf("  [%d] >> %s <<\r\n", i + 1, mode_names[i]);
        } else {
            uart_printf("  [%d]    %s\r\n", i + 1, mode_names[i]);
        }
    }

    uart_print("========================================\r\n");
    uart_print("Send '1'-'5' or press USER button to cycle.\r\n");
    uart_print("Press USER button long (>1s) to confirm.\r\n");
}

static uint8_t run_menu(void) {
    uint8_t selected = 0;
    uint8_t btn_prev = 0;
    uint32_t btn_press_tick = 0;

    print_menu(selected);

    while (1) {
        /* Check for UART input using polling (short timeout) */
        if (HAL_UART_Receive(&huart2, &menu_rx_byte, 1, 10) == HAL_OK) {
            /* Echo the keystroke so user sees what they typed */
            HAL_UART_Transmit(&huart2, &menu_rx_byte, 1, HAL_MAX_DELAY);

            if (menu_rx_byte >= '1' && menu_rx_byte <= '0' + NUM_MODES) {
                uint8_t choice = menu_rx_byte - '1';
                /* Check if this mode is implemented */
                if (choice == 0) {
                    return choice;
                } else {
                    uart_printf("\r\n  Mode %d is not yet implemented.\r\n", choice + 1);
                    uart_print("  Only Mode 1 is available right now.\r\n");
                    print_menu(selected);
                }
            } else if (menu_rx_byte == '\r' || menu_rx_byte == '\n') {
                /* Enter key — select current highlighted mode */
                if (selected == 0) {
                    return selected;
                } else {
                    uart_printf("\r\n  Mode %d is not yet implemented.\r\n", selected + 1);
                    print_menu(selected);
                }
            }
        }

        /* Check USER button (PA0) */
        uint8_t btn_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

        if (btn_now && !btn_prev) {
            /* Button just pressed — record time */
            btn_press_tick = HAL_GetTick();
        }

        if (!btn_now && btn_prev) {
            /* Button just released */
            uint32_t held = HAL_GetTick() - btn_press_tick;

            if (held > 1000) {
                /* Long press — confirm selection */
                if (selected == 0) {
                    return selected;
                } else {
                    uart_printf("\r\n  Mode %d not yet implemented.\r\n", selected + 1);
                    print_menu(selected);
                }
            } else if (held > 50) {
                /* Short press — cycle to next mode */
                selected = (selected + 1) % NUM_MODES;
                print_menu(selected);
            }
        }

        btn_prev = btn_now;
        HAL_Delay(10);
    }
}

/* ===== MAIN ===== */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* Flash all LEDs briefly to show we're alive */
    HAL_GPIO_WritePin(GPIOD,
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
        GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOD,
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
        GPIO_PIN_RESET);

    while (1) {
        /* Show mode selection menu */
        mode_reset_requested = 0;
        uint8_t mode = run_menu();

        uart_printf("\r\nStarting Mode %d: %s\r\n", mode + 1, mode_names[mode]);

        /* Dispatch to the selected mode */
        switch (mode) {
            case 0:
                mode1_run();
                break;
            /*
            case 1:
                mode2_run();
                break;
            case 2:
                mode3_run();
                break;
            case 3:
                mode4_run();
                break;
            case 4:
                mode5_run();
                break;
            */
            default:
                uart_print("Mode not implemented yet.\r\n");
                break;
        }

        /* When a mode exits (via reset command), we loop back to the menu */
        uart_print("\r\n--- Mode exited. Returning to menu. ---\r\n");
    }
}

/* ===========================================================================
 * PERIPHERAL INIT FUNCTIONS
 * These match CubeMX output. You can replace them with auto-generated
 * versions if you prefer — just make sure the settings match.
 * =========================================================================== */

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* LEDs: PD12 (green), PD13 (orange), PD14 (red), PD15 (blue) */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USER button: PA0 */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* All LEDs off */
    HAL_GPIO_WritePin(GPIOD,
        GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15,
        GPIO_PIN_RESET);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA2 = TX, PA3 = RX */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart2);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
        HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
        for (volatile int i = 0; i < 500000; i++);
    }
}
