/*******************************************************************************
 * Size: 15 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/
#define LV_LVGL_H_INCLUDE_SIMPLE
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FONT15
#define FONT15 1
#endif

#if FONT15

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0045 "E" */
    0x7f, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1,
    0xee, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x1,
    0xfc,

    /* U+0046 "F" */
    0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xf7,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x0,

    /* U+004C "L" */
    0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x0,
    0x0, 0x80, 0x40, 0x20, 0x10, 0x8, 0x3, 0xf8,

    /* U+004D "M" */
    0x0, 0x40, 0xe8, 0x74, 0x7b, 0x6c, 0xb6, 0x13,
    0x1, 0x88, 0xc4, 0x62, 0x31, 0x18, 0x8c, 0x4,
    0x0,

    /* U+004E "N" */
    0x0, 0x60, 0x70, 0x3c, 0x1b, 0xc, 0x86, 0x43,
    0x1, 0x84, 0xc3, 0x60, 0xb0, 0x78, 0x1c, 0x4,
    0x0,

    /* U+004F "O" */
    0x7f, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3,
    0x1, 0x80, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x5,
    0xfc,

    /* U+0050 "P" */
    0x7f, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3,
    0xee, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x0,
    0x0,

    /* U+0052 "R" */
    0x7f, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3,
    0xee, 0x84, 0x43, 0x20, 0x90, 0x68, 0x14, 0x0,
    0x0,

    /* U+0053 "S" */
    0x7f, 0x60, 0x30, 0x1c, 0xb, 0x5, 0x82, 0x41,
    0xef, 0x4, 0x83, 0x40, 0xa0, 0x70, 0x18, 0xd,
    0xfc,

    /* U+0054 "T" */
    0x7f, 0x0, 0x8, 0x8, 0x8, 0x8, 0x8, 0x0,
    0x8, 0x8, 0x8, 0x8, 0x8
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 196, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 196, .box_w = 9, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 196, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 196, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = 2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_0[] = {
    0, 1, 0, 0, 0, 0, 0, 2,
    3, 4, 5, 6, 0, 7, 8, 9
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 69, .range_length = 16, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_0, .list_length = 16, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t font15 = {
#else
lv_font_t font15 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if FONT15*/

