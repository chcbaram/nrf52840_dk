/*
 * bsp.c
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "bsp.h"
#include "uart.h"

static volatile uint32_t systick_counter = 0;
extern void swtimerISR(void);


void SysTick_Handler(void)
{
  systick_counter++;
  swtimerISR();
}


void bspInit(void)
{
  nrf_systick_load_set(SystemCoreClock / (1000UL / (uint32_t)1)); // 1Khz
  nrf_systick_csr_set(
      NRF_SYSTICK_CSR_CLKSOURCE_CPU |
      NRF_SYSTICK_CSR_TICKINT_ENABLE |
      NRF_SYSTICK_CSR_ENABLE);
}


int __io_putchar(int ch)
{
  uartWrite(_DEF_UART1, (uint8_t *)&ch, 1);
  return 1;
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
