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

    static uint32_t lcd_pre_time;
    static uint32_t fps_time;
    static uint32_t fps;
    static uint32_t x = 0;
    static uint32_t y = 0;

    if (lcdDrawAvailable() > 0)
    {
      lcd_pre_time = micros();
      lcdClearBuffer(black);
      logPrintf("%d us\n", micros()-lcd_pre_time);
      lcdDrawFillRect(x, 32, 30, 30, lcdSwap16(red));
      lcdDrawFillRect(lcdGetWidth()-x, 62, 30, 30, lcdSwap16(green));
      lcdDrawFillRect(x + 30, 92, 30, 30, lcdSwap16(blue));

      x += 4;

      x %= lcdGetWidth();
      y %= lcdGetHeight();

      lcdRequestDraw();
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
  else if (cmdifGetParamCnt() == 1 && cmdifHasString("spi", 0) == true)
  {
    uint8_t data[2] = {1, 2};
    uint32_t pre_time;

    pre_time = millis();
    if (spiDmaTransfer(_DEF_SPI1, (void *)data, 2, 100) == true)
    {
      cmdifPrintf("spi tx ok, %d ms\n", millis()-pre_time);
    }
    else
    {
      cmdifPrintf("spi tx fail\n");
    }
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
