/*
 * hw.c
 *
 *  Created on: 2020. 3. 20.
 *      Author: Baram
 */




#include "hw.h"




__attribute__((aligned(2048))) __attribute__((used, section(".tag"))) const boot_tag_t boot_tag =
    {
        .boot_name    = "NRF52840_DK_B/D",
        .boot_ver     = "B2000323R1",
        .magic_number = 0x5555AAAA,
    };


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

  logPrintf("\n\n[ Bootloader Begin... ]\r\n");
  logPrintf("Booting..Name \t\t: %s\r\n", boot_tag.boot_name);
  logPrintf("Booting..Ver  \t\t: %s\r\n", boot_tag.boot_ver);

  logPrintf("Tag Addr   \t\t: 0x%X\r\n", (int)&boot_tag);

  flashInit();

  usbInit();
}
