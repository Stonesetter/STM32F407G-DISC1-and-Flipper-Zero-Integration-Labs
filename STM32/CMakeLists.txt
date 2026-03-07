cmake_minimum_required(VERSION 3.22)

# Toolchain file MUST be set before project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

project(flipper_stm32_labs C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ===========================================================================
# Path to STM32CubeF4 firmware package (HAL drivers, CMSIS)
# ===========================================================================
set(STM32_HAL_PATH "C:/Users/Cleo/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3")

set(HAL_DRIVER_PATH "${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver")
set(CMSIS_DEVICE_PATH "${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F4xx")
set(CMSIS_CORE_PATH "${STM32_HAL_PATH}/Drivers/CMSIS/Include")

# ===========================================================================
# MCU flags for Cortex-M4 (STM32F407VG)
# ===========================================================================
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=soft")
set(CMAKE_C_FLAGS "${MCU_FLAGS} -Wall -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} -x assembler-with-cpp")

# Debug build by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Og -g3 -gdwarf-2)
else()
    add_compile_options(-Os)
endif()

# Preprocessor defines
add_compile_definitions(
    USE_HAL_DRIVER
    STM32F407xx
)

# ===========================================================================
# Source files
# ===========================================================================

# Our application sources
set(APP_SOURCES
    Core/Src/main.c
    Core/Src/system_stm32f4xx.c
    Core/Src/modes/mode1_uart_console.c
    Core/Src/modes/mode2_gpio_remote.c
    Core/Src/modes/mode3_adc_dashboard.c
    Core/Src/modes/mode4_spi_link.c
    Core/Src/modes/mode5_flipper_app.c
)

# HAL driver sources (only what we use)
set(HAL_SOURCES
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_cortex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_dma.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_dma_ex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_flash.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_flash_ex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_gpio.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_pwr.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_pwr_ex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_rcc.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_rcc_ex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_uart.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_exti.c
    # Future modes:
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_adc.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_adc_ex.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_spi.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_tim.c
    ${HAL_DRIVER_PATH}/Src/stm32f4xx_hal_tim_ex.c
)

# Startup assembly
set(STARTUP_SOURCE Core/Startup/startup_stm32f407vgtx.s)

# ===========================================================================
# Include paths
# ===========================================================================
set(INCLUDE_DIRS
    Core/Inc
    ${HAL_DRIVER_PATH}/Inc
    ${CMSIS_DEVICE_PATH}/Include
    ${CMSIS_CORE_PATH}
)

# ===========================================================================
# Linker configuration
# ===========================================================================
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32F407VGTX_FLASH.ld)

set(CMAKE_EXE_LINKER_FLAGS
    "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections -Wl,--print-memory-usage -specs=nano.specs -specs=nosys.specs -lc -lm -lnosys"
)

# ===========================================================================
# Build target
# ===========================================================================
add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${HAL_SOURCES}
    ${STARTUP_SOURCE}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_DIRS})

# Generate .bin and .hex after build
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex  $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating .bin and .hex, printing size"
)

# ===========================================================================
# Flash target (run: cmake --build build --target flash)
# ===========================================================================
set(STM32_PROGRAMMER "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe")

add_custom_target(flash
    COMMAND "${STM32_PROGRAMMER}" -c port=SWD -w ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin 0x08000000 -v -rst
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing to STM32 via ST-Link..."
)
