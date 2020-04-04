#include "lvgl/lvgl.h"

/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifndef HAN
#define HAN 1
#endif

#if HAN

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t gylph_bitmap[] = {
    /* U+20 " " */

    /* U+C870 "조" */
    0x1, 0xff, 0xff, 0xff, 0xc0, 0x7, 0xff, 0xff,
    0xff, 0x80, 0x0, 0x3, 0xfc, 0x0, 0x0, 0x0,
    0x1f, 0xfe, 0x0, 0x0, 0x3, 0xff, 0xff, 0xf0,
    0x0, 0x7f, 0xe0, 0x1f, 0xfc, 0x0, 0x78, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x3f, 0x80, 0x0, 0x0,
    0x0, 0x7f, 0x0, 0x0, 0x1f, 0xff, 0xff, 0xff,
    0xf0, 0x3f, 0xff, 0xff, 0xff, 0xe0,

    /* U+CCA0 "철" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x7, 0xf8, 0x1,
    0xf8, 0x3f, 0xff, 0xfe, 0x1f, 0xc0, 0x7f, 0xff,
    0xff, 0xfc, 0xf, 0xff, 0xf8, 0xff, 0xc3, 0xfe,
    0x3f, 0xe1, 0xfc, 0x0, 0x0, 0x0, 0xf, 0x0,
    0x7f, 0xff, 0xff, 0xf8, 0x3, 0xff, 0xff, 0xff,
    0xc0, 0xff, 0xff, 0xff, 0xf8, 0xf, 0xff, 0xff,
    0xff, 0x0, 0x7f, 0xff, 0xff, 0xfc, 0x0, 0x0,
    0x0, 0x0, 0x0,

    /* U+D55C "한" */
    0x0, 0x18, 0x0, 0x0, 0x0, 0x3f, 0xff, 0xf8,
    0x3e, 0x1, 0xff, 0xff, 0xf8, 0xfe, 0x0, 0xff,
    0xff, 0xc1, 0xff, 0x81, 0xfc, 0x1f, 0xc3, 0xff,
    0x3, 0xf8, 0x3f, 0x87, 0xf0, 0x3, 0xff, 0xfe,
    0xf, 0xe0, 0x0, 0x7f, 0xc0, 0x1f, 0xc0, 0xf,
    0x80, 0x0, 0x0, 0x0, 0x3f, 0x80, 0x0, 0x0,
    0x0, 0x7f, 0xff, 0xff, 0xfc, 0x0, 0x7f, 0xff,
    0xff, 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 50, .box_w = 6, .box_h = 0, .ofs_x = -1, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 186, .box_w = 39, .box_h = 11, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 54, .adv_w = 192, .box_w = 36, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 113, .adv_w = 198, .box_w = 39, .box_h = 13, .ofs_x = 0, .ofs_y = -3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0xc850, 0xcc80, 0xd53c
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 54589, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 4, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

/*Store all the custom data of the font*/
static lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = gylph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
lv_font_t han = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_HOR,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};

#endif /*#if HAN*/

