#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct { const uint8_t *data;size_t size; } DSBlob;
typedef struct { uint16_t cell,rotation;int32_t scale_x,scale_y;int16_t x,y; } DSAnimFrame;
/* Native resource decoders; no DS pointers are dereferenced. Output must hold w*h*4 bytes. */
bool ds_draw_tilemap(DSBlob chars,DSBlob palette,DSBlob screen,uint8_t *rgba,unsigned w,unsigned h);
bool ds_animation_frame(DSBlob animation,unsigned sequence,uint64_t tick,DSAnimFrame *frame);
bool ds_draw_cell(DSBlob chars,DSBlob palette,DSBlob cells,unsigned cell,uint8_t *rgba,unsigned w,unsigned h,int x,int y);
bool ds_draw_animation(DSBlob chars,DSBlob palette,DSBlob cells,DSBlob animation,unsigned sequence,uint64_t tick,uint8_t *rgba,unsigned w,unsigned h,int x,int y);
