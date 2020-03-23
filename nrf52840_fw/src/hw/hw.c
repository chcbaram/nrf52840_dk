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

  swtimerInit();
  cmdifInit();

  ledInit();
  buttonInit();
  vcpInit();
  uartInit();
  uartOpen(_DEF_UART1, 57600);
  uartOpen(_DEF_UART2, 57600);

  logPrintf("\n\n[ Firmware Begin... ]\r\n");

  flashInit();

  usbInit();
}
