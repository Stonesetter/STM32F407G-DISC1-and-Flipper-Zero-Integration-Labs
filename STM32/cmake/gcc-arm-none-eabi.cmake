set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Path to ARM GCC toolchain bundled with STM32CubeIDE
set(TOOLCHAIN_PREFIX "C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools")

set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_AR "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-ar.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-objdump.exe")
set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}/bin/arm-none-eabi-size.exe")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
