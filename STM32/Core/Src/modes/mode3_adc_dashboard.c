/**
 * ============================================================
 * Mode 3: ADC Sensor Dashboard
 * ============================================================
 *
 * Reads analog signals and streams them over UART. Demonstrates:
 *   - ADC (Analog-to-Digital Converter) peripheral
 *   - Internal temperature sensor
 *   - External analog input (potentiometer)
 *   - Periodic sampling with configurable interval
 *   - Streaming data in CSV-like format
 *
 * Two ADC channels:
 *   1. Internal temperature sensor (ADC1 Channel 18)
 *      - Built into the STM32, no wiring needed
 *      - Reads die temperature (~25-35C at room temp)
 *
 *   2. External analog input on PA1 (ADC1 Channel 1)
 *      - Optional: connect a potentiometer
 *      - Wiper to PA1, ends to 3V3 and GND
 *      - If nothing connected, reads floating noise (that's fine)
 *
 * TEXT COMMANDS:
 *   help              - Show available commands
 *   start             - Start continuous streaming
 *   stop              - Stop streaming
 *   single            - Take one reading and print it
 *   rate <ms>         - Set sample interval (50-10000 ms)
 *   status            - Show current settings
 *   reset             - Return to boot menu
 *
 * STREAM FORMAT (CSV-like, one line per sample):
 *   T:<tick_ms>,TEMP:<celsius>,RAW_T:<adc_raw>,EXT:<voltage>V,RAW_E:<adc_raw>
 *
 * Wiring: Same 3-wire UART as Mode 1
 *         (Optional) Potentiometer on PA1
 */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Externs from main.c */
extern UART_HandleTypeDef huart2;
extern void uart_print(const char *msg);
extern void uart_printf(const char *fmt, ...);
extern volatile uint8_t mode_reset_requested;

/* ---- ADC handle ---- */
static ADC_HandleTypeDef hadc1;

/* ---- Sampling state ---- */
static uint8_t  streaming = 0;
static uint32_t sample_interval_ms = 500;  /* Default: 2 samples/sec */
static uint32_t last_sample_tick = 0;
static uint32_t sample_count = 0;

/* ---- ADC calibration constants for STM32F407 ---- */
/*
 * The internal temp sensor is connected to ADC1_IN18.
 * From the STM32F407 datasheet (section 6.3.22):
 *   V_25   = 0.76V  (voltage at 25C)
 *   Avg_slope = 2.5 mV/C
 *
 * Formula:
 *   Temperature(C) = ((V_25 - V_sense) / Avg_slope) + 25
 *
 * Where V_sense = (ADC_raw / 4095) * 3.3V
 *
 * The ADC is 12-bit (0-4095), reference voltage is 3.3V.
 */
#define ADC_RESOLUTION  4095.0f
#define V_REF           3.3f
#define V_25            0.76f
#define AVG_SLOPE       0.0025f  /* 2.5 mV/C */

/* ---- Init ADC for single-conversion polling mode ---- */
static void ADC1_Init(void) {
    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    HAL_ADC_Init(&hadc1);
}

/* ---- Configure PA1 as analog input ---- */
static void ADC_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

/* ---- Read a single ADC channel ---- */
static uint32_t read_adc_channel(uint32_t channel, uint32_t sample_time) {
    ADC_ChannelConfTypeDef config = {0};
    config.Channel = channel;
    config.Rank = 1;
    config.SamplingTime = sample_time;
    HAL_ADC_ConfigChannel(&hadc1, &config);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return raw;
}

/* ---- Read internal temperature sensor ---- */
static uint32_t read_temp_raw(void) {
    /* Temperature sensor needs longer sample time (min 10us -> ADC_SAMPLETIME_144CYCLES) */
    return read_adc_channel(ADC_CHANNEL_TEMPSENSOR, ADC_SAMPLETIME_144CYCLES);
}

/* ---- Read external analog on PA1 ---- */
static uint32_t read_ext_raw(void) {
    return read_adc_channel(ADC_CHANNEL_1, ADC_SAMPLETIME_84CYCLES);
}

/* ---- Convert raw ADC to temperature in Celsius ---- */
static float raw_to_temperature(uint32_t raw) {
    float v_sense = ((float)raw / ADC_RESOLUTION) * V_REF;
    return ((V_25 - v_sense) / AVG_SLOPE) + 25.0f;
}

/* ---- Convert raw ADC to voltage ---- */
static float raw_to_voltage(uint32_t raw) {
    return ((float)raw / ADC_RESOLUTION) * V_REF;
}

/* ---- Print one sample ---- */
static void print_sample(void) {
    uint32_t temp_raw = read_temp_raw();
    uint32_t ext_raw = read_ext_raw();

    float temp_c = raw_to_temperature(temp_raw);
    float ext_v = raw_to_voltage(ext_raw);

    sample_count++;

    /* Integer-based printing to avoid pulling in full float printf.
     * temp_c and ext_v are converted to fixed-point. */
    int temp_whole = (int)temp_c;
    int temp_frac = (int)((temp_c - (float)temp_whole) * 10.0f);
    if (temp_frac < 0) temp_frac = -temp_frac;

    int ext_whole = (int)ext_v;
    int ext_frac = (int)((ext_v - (float)ext_whole) * 100.0f);
    if (ext_frac < 0) ext_frac = -ext_frac;

    uart_printf("T:%lu,TEMP:%d.%dC,RAW_T:%lu,EXT:%d.%02dV,RAW_E:%lu\r\n",
                HAL_GetTick(), temp_whole, temp_frac, temp_raw,
                ext_whole, ext_frac, ext_raw);
}

/* ---- Process text commands ---- */
static void process_command(char *cmd) {
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    /* Split into tokens */
    char *argv[2] = {0};
    int argc = 0;
    char *p = cmd;
    while (*p && argc < 2) {
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) { *p = '\0'; p++; }
        while (*p == ' ') p++;
    }

    if (strcmp(argv[0], "help") == 0) {
        uart_print("\r\n--- ADC Sensor Dashboard Commands ---\r\n");
        uart_print("  start          - Start continuous streaming\r\n");
        uart_print("  stop           - Stop streaming\r\n");
        uart_print("  single         - Take one reading\r\n");
        uart_print("  rate <ms>      - Set sample interval (50-10000)\r\n");
        uart_print("  status         - Show current settings\r\n");
        uart_print("  reset          - Return to menu\r\n");
        uart_print("\r\n  Stream format:\r\n");
        uart_print("  T:<ms>,TEMP:<C>,RAW_T:<raw>,EXT:<V>,RAW_E:<raw>\r\n");
        uart_print("\r\n  Channels:\r\n");
        uart_print("    TEMP = Internal die temperature sensor\r\n");
        uart_print("    EXT  = PA1 analog input (connect pot or leave floating)\r\n");
        return;
    }

    if (strcmp(argv[0], "reset") == 0) {
        streaming = 0;
        mode_reset_requested = 1;
        return;
    }

    if (strcmp(argv[0], "start") == 0) {
        streaming = 1;
        sample_count = 0;
        last_sample_tick = HAL_GetTick();
        uart_print("  Streaming started. Type 'stop' to end.\r\n");
        return;
    }

    if (strcmp(argv[0], "stop") == 0) {
        streaming = 0;
        uart_printf("  Streaming stopped. %lu samples taken.\r\n", sample_count);
        uart_print("> ");
        return;
    }

    if (strcmp(argv[0], "single") == 0) {
        uart_print("  ");
        print_sample();
        return;
    }

    if (strcmp(argv[0], "rate") == 0) {
        if (argc < 2) {
            uart_printf("  Current rate: %lu ms\r\n", sample_interval_ms);
            uart_print("  Usage: rate <50-10000>\r\n");
            return;
        }
        /* Parse number manually */
        uint32_t val = 0;
        char *s = argv[1];
        while (*s >= '0' && *s <= '9') {
            val = val * 10 + (*s - '0');
            s++;
        }
        if (val < 50) val = 50;
        if (val > 10000) val = 10000;
        sample_interval_ms = val;
        uart_printf("  Sample rate set to %lu ms\r\n", sample_interval_ms);
        return;
    }

    if (strcmp(argv[0], "status") == 0) {
        uart_printf("  Streaming: %s\r\n", streaming ? "ON" : "OFF");
        uart_printf("  Interval:  %lu ms\r\n", sample_interval_ms);
        uart_printf("  Samples:   %lu\r\n", sample_count);
        uart_print("  Channels:  TEMP (internal), EXT (PA1)\r\n");
        uart_print("  Taking a single reading:\r\n  ");
        print_sample();
        return;
    }

    uart_printf("  Unknown command: %s (type 'help')\r\n", argv[0]);
}

/* ---- Main mode loop ---- */

#define CMD_BUF_SIZE 64

void mode3_run(void) {
    char cmd_buf[CMD_BUF_SIZE];
    uint8_t cmd_pos = 0;
    uint8_t rx_byte;

    /* Initialize ADC */
    ADC_GPIO_Init();
    ADC1_Init();

    /* Reset state */
    streaming = 0;
    sample_count = 0;
    sample_interval_ms = 500;

    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Mode 3: ADC Sensor Dashboard\r\n");
    uart_print("========================================\r\n");
    uart_print("Reads internal temp sensor + PA1 analog.\r\n");
    uart_print("Type 'help' for commands.\r\n\r\n");

    /* Show one reading right away so user knows it's working */
    uart_print("  Initial reading: ");
    print_sample();
    uart_print("\r\n> ");

    while (!mode_reset_requested) {
        /* Check for UART input */
        if (HAL_UART_Receive(&huart2, &rx_byte, 1, 10) == HAL_OK) {
            if (rx_byte == '\r' || rx_byte == '\n') {
                uart_print("\r\n");
                cmd_buf[cmd_pos] = '\0';
                if (cmd_pos > 0) {
                    process_command(cmd_buf);
                }
                cmd_pos = 0;
                if (!mode_reset_requested && !streaming) {
                    uart_print("> ");
                }
            } else if (rx_byte == 0x7F || rx_byte == '\b') {
                if (cmd_pos > 0) {
                    cmd_pos--;
                    uart_print("\b \b");
                }
            } else if (cmd_pos < CMD_BUF_SIZE - 1) {
                cmd_buf[cmd_pos++] = (char)rx_byte;
                HAL_UART_Transmit(&huart2, &rx_byte, 1, HAL_MAX_DELAY);
            }
        }

        /* Stream samples at configured interval */
        if (streaming) {
            uint32_t now = HAL_GetTick();
            if (now - last_sample_tick >= sample_interval_ms) {
                last_sample_tick = now;
                print_sample();
            }
        }
    }

    /* Cleanup: de-init ADC so it doesn't interfere with other modes */
    HAL_ADC_DeInit(&hadc1);
    __HAL_RCC_ADC1_CLK_DISABLE();
}
