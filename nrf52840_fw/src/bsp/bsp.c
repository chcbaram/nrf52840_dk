/*
 * bsp.c
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "bsp.h"


static volatile uint32_t systick_counter = 0;


void SysTick_Handler(void)
{
  systick_counter++;
}


void bspInit(void)
{
  nrf_systick_load_set(SystemCoreClock / (1000UL / (uint32_t)1)); // 1Khz
  nrf_systick_csr_set(
      NRF_SYSTICK_CSR_CLKSOURCE_CPU |
      NRF_SYSTICK_CSR_TICKINT_ENABLE |
      NRF_SYSTICK_CSR_ENABLE);
}



void delay(uint32_t ms)
{
  uint32_t pre_time = systick_counter;

  while(systick_counter-pre_time < ms);
}

uint32_t millis(void)
{
  return systick_counter;
}
