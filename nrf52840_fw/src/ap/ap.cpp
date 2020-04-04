/*
 * ap.cpp
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */



#include "ap.h"



void bootCmdif(void);
static void threadUsb(void const *argument);
static void threadLED(void const *argument);
static void threadLCD(void const *argument);


void apInit(void)
{
  hwInit();

  cmdifOpen(_DEF_UART1, 57600);
  cmdifAdd("boot", bootCmdif);

  osThreadDef(threadUsb, threadUsb, _HW_DEF_RTOS_THREAD_PRI_USB, 0, _HW_DEF_RTOS_THREAD_MEM_USB);
  osThreadCreate(osThread(threadUsb), NULL);

  osThreadDef(threadLED, threadLED, _HW_DEF_RTOS_THREAD_PRI_LED, 0, _HW_DEF_RTOS_THREAD_MEM_LED);
  osThreadCreate(osThread(threadLED), NULL);

  osThreadDef(threadLCD, threadLCD, _HW_DEF_RTOS_THREAD_PRI_LCD, 0, _HW_DEF_RTOS_THREAD_MEM_LCD);
  osThreadCreate(osThread(threadLCD), NULL);
}

void apMain(void)
{
  while(1)
  {
    cmdifMain();
    osThreadYield();
  }
}


static void threadUsb(void const *argument)
{

  for(;;)
  {
    if (tusb_inited())
    {
      tud_task();
    }
    osThreadYield();
  }
}


static void threadLED(void const *argument)
{
  for(;;)
  {
    ledToggle(_DEF_LED1);
    osDelay(500);
  }
}


static void threadLCD(void const *argument)
{
  uint32_t pre_time = 0;
  uint32_t fps_time = 0;
  uint16_t x = 0;
  uint16_t y = 0;

  for(;;)
  {
#if 0
    if (lcdDrawAvailable() > 0)
    {
      pre_time = millis();
      lcdClearBuffer(black);

      lcdDrawFillRect(x, 32, 20, 20, red);
      lcdDrawFillRect(lcdGetWidth() - x, 52, 20, 20, green);
      lcdDrawFillRect(x + 30, 72, 20, 20, blue);

      //lcdPrintf(0, 0, white, "%d ms", millis()-fps_time);
      //lcdPrintf(0, 16, white, "%d fps", 1000/(millis()-fps_time));

      //radioPrintf(RADIO_CH_0,"lcd %d ms, %d fps, draw %d ms\n", millis()-fps_time, 1000/(millis()-fps_time), millis()-pre_time);
      fps_time = millis();

      x += 2;

      x %= lcdGetWidth();
      y %= lcdGetHeight();

      lcdRequestDraw();
    }
#endif
    //lvglUpdate();
    osThreadYield();
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
