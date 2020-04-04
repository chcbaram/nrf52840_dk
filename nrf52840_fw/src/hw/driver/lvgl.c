/*
 * lvgl.c
 *
 *  Created on: 2020. 4. 3.
 *      Author: HanCheol Cho
 */




#include "lvgl.h"
#include "lv_conf.h"

#include "lvgl/lvgl.h"
#include "porting/lv_port_disp.h"

#include "cmdif.h"



void lvglCmdif(void);



bool lvglInit(void)
{
  uint32_t pre_time;

  lv_init();
  lv_port_disp_init();


  pre_time = millis();
  while(millis()-pre_time < 100)
  {
    lvglUpdate();
  }

  cmdifAdd("lvgl", lvglCmdif);

  return true;
}

bool lvglUpdate(void)
{
  lv_task_handler();
  return true;
}



void btn_event_cb(lv_obj_t * btn, lv_event_t event)
{
    if(event == LV_EVENT_CLICKED) {
        printf("Clicked\n");
    }
}


static lv_obj_t * btn;

static lv_obj_t * img_loop;
static void img_looper(lv_task_t *param) {

  (void)param;
  static int x_pos = 0;

  static uint32_t pre_time;


  lv_obj_set_pos(btn, x_pos, 0);

  x_pos = (x_pos + 2) % 160;

  cmdifPrintf("%d ms\n", millis()-pre_time);
  pre_time = millis();
}


LV_FONT_DECLARE(han);


void lvglCmdif(void)
{
  bool ret = true;



  if (cmdifGetParamCnt() == 1 && cmdifHasString("run", 0) == true)
  {
    lv_obj_t * scr = lv_disp_get_scr_act(NULL);

#if 1
    btn = lv_btn_create(scr, NULL);     /*Add a button the current screen*/
    lv_obj_set_pos(btn,0, 0);                            /*Set its position*/
    lv_obj_set_size(btn, 100, 50);                          /*Set its size*/
    lv_obj_set_event_cb(btn, btn_event_cb);                 /*Assign a callback to the button*/

    //style.text.font = &my_font_name;
    //scr->style_p->text.font = &han;

    lv_obj_t * label = lv_label_create(btn, NULL);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");                     /*Set the labels text*/
#endif


    lv_task_create( img_looper, 100, LV_TASK_PRIO_LOWEST, NULL );


    while(cmdifRxAvailable() == 0)
    {
      lv_task_handler();
      //delay(5);
    }
  }
  else
  {
    ret = false;
  }

  if (ret == false)
  {
    cmdifPrintf( "lvgl run \n");
  }
}
