/*
 * ili9341.h
 *
 *  Created on: 2020. 3. 26.
 *      Author: HanCheol Cho
 */

#ifndef SRC_COMMON_HW_INCLUDE_ILI9341_H_
#define SRC_COMMON_HW_INCLUDE_ILI9341_H_



#ifdef __cplusplus
 extern "C" {
#endif



#include "hw_def.h"


#ifdef _USE_HW_ILI9341

#include "lcd.h"
#include "ili9341_regs.h"



#define HW_LCD_WIDTH      320
#define HW_LCD_HEIGHT     240




bool ili9341Init(void);
bool ili9341InitDriver(lcd_driver_t *p_driver);
bool ili9341DrawAvailable(void);
bool ili9341RequestDraw(void);
void ili9341SetFrameBuffer(uint16_t *p_buf);
void ili9341SetWindow(int32_t x, int32_t y, int32_t w, int32_t h);

uint32_t ili9341GetFps(void);
uint32_t ili9341GetFpsTime(void);

uint16_t ili9341GetWidth(void);
uint16_t ili9341GetHeight(void);

bool ili9341SetCallBack(void (*p_func)(void));
bool ili9341SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms);

#endif

#ifdef __cplusplus
}
#endif



#endif /* SRC_COMMON_HW_INCLUDE_ILI9341_H_ */
