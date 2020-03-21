/*
 * hw.c
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "hw.h"





void hwInit(void)
{
  bspInit();

  cmdifInit();

  ledInit();
  uartInit();
  uartOpen(_DEF_UART1, 57600);
}
