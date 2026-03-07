/**
 * Minimal system_stm32f4xx.c
 * Provides SystemInit() and SystemCoreClock for the HAL.
 * Using HSI at 16 MHz (no PLL, simple config).
 */

#include "stm32f4xx.h"

uint32_t SystemCoreClock = 16000000U;  /* HSI = 16 MHz */

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void) {
    /* FPU settings — enable CP10 and CP11 coprocessors */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
#endif

    /* Reset the RCC clock configuration to the default reset state */
    RCC->CR |= (uint32_t)0x00000001;   /* Set HSION bit */
    RCC->CFGR = 0x00000000;            /* Reset CFGR register */
    RCC->CR &= (uint32_t)0xFEF6FFFF;   /* Reset HSEON, CSSON, PLLON bits */
    RCC->PLLCFGR = 0x24003010;         /* Reset PLLCFGR register */
    RCC->CR &= (uint32_t)0xFFFBFFFF;   /* Reset HSEBYP bit */
    RCC->CIR = 0x00000000;             /* Disable all interrupts */

    /* Vector table in internal FLASH */
    SCB->VTOR = FLASH_BASE;
}
