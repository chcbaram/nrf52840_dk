/*
 * ap.cpp
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "ap.h"





void apInit(void)
{
  hwInit();

  cmdifOpen(_DEF_UART1, 57600);
}

void apMain(void)
{
  uint32_t pre_time;

  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();

      ledToggle(_DEF_LED1);
    }
    cmdifMain();

    if ( tusb_inited() )
    {
      tud_task();
    }

    if (uartAvailable(_DEF_UART2) > 0)
    {
      //uartPrintf(_DEF_UART2, " : 0x%X\n", uartRead(_DEF_UART2));
      uint8_t ch;

      ch = uartRead(_DEF_UART2);
      //uartPutch(_DEF_UART1, ch);
      uartPutch(_DEF_UART2, ch);
    }
  }
}

