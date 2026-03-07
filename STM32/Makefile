##############################################################################
# Flipper-STM32 Integration Labs — Makefile
#
# Build:  make
# Flash:  make flash
# Clean:  make clean
# Size:   make size
##############################################################################

# Project name
TARGET = flipper_stm32_labs

# Build directory
BUILD_DIR = build

# Debug (1=debug, 0=release)
DEBUG = 1

##############################################################################
# Toolchain paths (from STM32CubeIDE installation)
##############################################################################
TOOLCHAIN = C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin

CC  = "$(TOOLCHAIN)/arm-none-eabi-gcc.exe"
AS  = "$(TOOLCHAIN)/arm-none-eabi-gcc.exe" -x assembler-with-cpp
CP  = "$(TOOLCHAIN)/arm-none-eabi-objcopy.exe"
SZ  = "$(TOOLCHAIN)/arm-none-eabi-size.exe"
GDB = "$(TOOLCHAIN)/arm-none-eabi-gdb.exe"

# STM32 Programmer for flashing
PROGRAMMER = "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"

# HAL firmware package
HAL_PATH = C:/Users/Cleo/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3
HAL_DRV  = $(HAL_PATH)/Drivers/STM32F4xx_HAL_Driver
CMSIS_DEV = $(HAL_PATH)/Drivers/CMSIS/Device/ST/STM32F4xx
CMSIS_INC = $(HAL_PATH)/Drivers/CMSIS/Include

##############################################################################
# MCU configuration
##############################################################################
CPU = -mcpu=cortex-m4
FPU =
FLOAT-ABI = -mfloat-abi=soft
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Linker script
LDSCRIPT = STM32F407VGTX_FLASH.ld

##############################################################################
# Source files
##############################################################################
# Application sources
C_SOURCES = \
Core/Src/main.c \
Core/Src/system_stm32f4xx.c \
Core/Src/modes/mode1_uart_console.c \
Core/Src/modes/mode2_gpio_remote.c \
Core/Src/modes/mode3_adc_dashboard.c \
Core/Src/modes/mode4_spi_link.c \
Core/Src/modes/mode5_flipper_app.c

# HAL driver sources (only what we need)
C_SOURCES += \
$(HAL_DRV)/Src/stm32f4xx_hal.c \
$(HAL_DRV)/Src/stm32f4xx_hal_cortex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_dma.c \
$(HAL_DRV)/Src/stm32f4xx_hal_dma_ex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_flash.c \
$(HAL_DRV)/Src/stm32f4xx_hal_flash_ex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_gpio.c \
$(HAL_DRV)/Src/stm32f4xx_hal_pwr.c \
$(HAL_DRV)/Src/stm32f4xx_hal_pwr_ex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_rcc.c \
$(HAL_DRV)/Src/stm32f4xx_hal_rcc_ex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_uart.c \
$(HAL_DRV)/Src/stm32f4xx_hal_exti.c \
$(HAL_DRV)/Src/stm32f4xx_hal_adc.c \
$(HAL_DRV)/Src/stm32f4xx_hal_adc_ex.c \
$(HAL_DRV)/Src/stm32f4xx_hal_spi.c \
$(HAL_DRV)/Src/stm32f4xx_hal_tim.c \
$(HAL_DRV)/Src/stm32f4xx_hal_tim_ex.c

# Startup assembly
ASM_SOURCES = Core/Startup/startup_stm32f407vgtx.s

##############################################################################
# Include paths
##############################################################################
C_INCLUDES = \
-ICore/Inc \
-I$(HAL_DRV)/Inc \
-I$(CMSIS_DEV)/Include \
-I$(CMSIS_INC)

##############################################################################
# Compiler & linker flags
##############################################################################
# C defines
C_DEFS = -DUSE_HAL_DRIVER -DSTM32F407xx

# Compiler flags
ifeq ($(DEBUG), 1)
  OPT = -Og -g3 -gdwarf-2
else
  OPT = -Os
endif

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -std=c11
ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

# Linker flags
LDFLAGS = $(MCU) -T$(LDSCRIPT) -specs=nano.specs -specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -lc -lm -lnosys

##############################################################################
# Build rules
##############################################################################

# List of object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Default target
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex size

# Compile C files
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Create .bin
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CP) -O binary $< $@

# Create .hex
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(CP) -O ihex $< $@

# Create build directory
$(BUILD_DIR):
	mkdir -p $@

# Print firmware size
size: $(BUILD_DIR)/$(TARGET).elf
	$(SZ) $<

# Flash via ST-Link
flash: $(BUILD_DIR)/$(TARGET).bin
	$(PROGRAMMER) -c port=SWD -w $< 0x08000000 -v -rst

# Erase chip
erase:
	$(PROGRAMMER) -c port=SWD -e all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean flash erase size
