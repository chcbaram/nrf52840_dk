/*
 * ap.cpp
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "ap.h"



void bootCmdif(void);


void apInit(void)
{
  hwInit();

  cmdifOpen(_DEF_UART1, 57600);
  cmdifAdd("boot", bootCmdif);
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
      uartPrintf(_DEF_UART2, "rx : 0x%X\n", uartRead(_DEF_UART2));
    }
  }
}






void bootCmdif(void)
{
  bool ret = true;


  if (cmdifGetParamCnt() == 1 && cmdifHasString("reset", 0) == true)
  {
    bspDeInit();
    NVIC_SystemReset();
  }
  else
  {
    ret = false;
  }

  if (ret == false)
  {
    cmdifPrintf( "boot reset \n");
  }
}
