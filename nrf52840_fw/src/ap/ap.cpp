/*
 * ap.cpp
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "ap.h"
#include "image.h"
#include "image2.h"
#include "image3.h"

#define TJPGD_WORKSPACE_SIZE  4096
uint8_t workspace[TJPGD_WORKSPACE_SIZE] __attribute__((aligned(4)));

uint8_t *jpg_buf = NULL;
uint32_t jpg_buf_i = 0;
uint8_t jpgScale = 0;

uint16_t jd_input(JDEC* jdec, uint8_t* buf, uint16_t len)
{
  memcpy(buf, (const uint8_t *)(jpg_buf + jpg_buf_i), len);
  jpg_buf_i += len;

  return len;
}


uint16_t jd_output(JDEC* jdec, void* bitmap, JRECT* jrect)
{
  jdec = jdec; // Supress warning as ID is not used

  uint16_t *p_buf = lcdGetFrameBuffer();
  uint16_t *p_bitmap = (uint16_t *)bitmap;

  // Retrieve rendering parameters and add any offset
  int16_t  x = jrect->left + 0;
  int16_t  y = jrect->top  + 0;
  uint16_t w = jrect->right  + 1 - jrect->left;
  uint16_t h = jrect->bottom + 1 - jrect->top;

  uint32_t index = 0;
  for (int i=y; i<y+h; i++)
  {
    for (int j=x; j<x+w; j++)
    {
      lcdDrawPixel(j, i, p_bitmap[index++]);
    }
  }
  //lcdSetWindow(x, y, w, h);

  // Pass the image block and rendering parameters in a callback to the sketch
  //return thisPtr->tft_output(x, y, w, h, (uint16_t*)bitmap);

  return 1;
}


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
  uint8_t *p_image[8];
  static uint8_t  index = 0;
  bool update = true;


  p_image[0] = (uint8_t *)image1;
  p_image[1] = (uint8_t *)image2;
  p_image[2] = (uint8_t *)image3;
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


    if (lcdDrawAvailable() > 0 && update == true)
    {
      JDEC jdec;
      JRESULT jresult = JDR_OK;

      /*
      lcd_pre_time = micros();
      lcdClearBuffer(black);
      logPrintf("%d us\n", micros()-lcd_pre_time);
      lcdDrawFillRect(x, 32, 30, 30, lcdSwap16(red));
      lcdDrawFillRect(lcdGetWidth()-x, 62, 30, 30, lcdSwap16(green));
      lcdDrawFillRect(x + 30, 92, 30, 30, lcdSwap16(blue));

      x += 4;

      x %= lcdGetWidth();
      y %= lcdGetHeight();
      */
      jdec.swap = true;

      //JRESULT jd_prepare (JDEC*, uint16_t(*)(JDEC*,uint8_t*,uint16_t), void*, uint16_t, void*);
      //JRESULT jd_decomp (JDEC*, uint16_t(*)(JDEC*,void*,JRECT*), uint8_t);

      lcd_pre_time = millis();
      jpg_buf = (uint8_t *)p_image[index];
      //index = (index+1) % 2;

      jpg_buf_i = 0;
      // Analyse input data
      jresult = jd_prepare(&jdec, jd_input, workspace, TJPGD_WORKSPACE_SIZE, 0);

      // Extract image and render
      if (jresult == JDR_OK)
      {
        jresult = jd_decomp(&jdec, jd_output, jpgScale);
        logPrintf("jpg %d ms\n", millis()-lcd_pre_time);
      }

      lcdRequestDraw();
      update = false;
    }

    if (buttonGetReleasedEvent(0) && buttonGetPressedTime(0) > 100)
    {
      index = (index+1) % 3;
      update = true;
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
