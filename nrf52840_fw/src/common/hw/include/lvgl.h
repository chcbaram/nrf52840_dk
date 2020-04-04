/*
 * lvgl.h
 *
 *  Created on: 2020. 4. 3.
 *      Author: HanCheol Cho
 */

#ifndef SRC_COMMON_HW_INCLUDE_LVGL_H_
#define SRC_COMMON_HW_INCLUDE_LVGL_H_


#ifdef __cplusplus
 extern "C" {
#endif


#include "hw_def.h"

#ifdef _USE_HW_LVGL


bool lvglInit(void);
bool lvglUpdate(void);


#ifdef __cplusplus
 }
#endif

#endif



#endif /* SRC_COMMON_HW_INCLUDE_LVGL_H_ */
