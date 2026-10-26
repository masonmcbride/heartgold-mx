#pragma once
#include <stddef.h>
#include <stdint.h>
/* Register backing shared by native calls and bounded ARM mappings. No clock,
 * command FIFO, renderer, or VRAM address is inferred from these registers. */
typedef struct {
    uint8_t engine[2][0x70];
    uint16_t banks[10];
    uint8_t vram[0xa4000],palette[2][0x400];
    uint8_t oam[2][0x400],display_select;
    uint16_t alpha_reference,fog_offset,clear_depth;
    uint32_t clear_color;
    /* NNS global software state: not a submitted geometry command. */
    uint32_t global_polygon_attr;
    uint8_t viewport[4],sort_mode,buffer_mode;
    unsigned geometry_initialized,matrix_epoch;
} DSVideo;
void ds_video_blend_none(DSVideo *,unsigned);
void ds_video_windows(DSVideo *,unsigned,unsigned);
uint32_t ds_video_display(const DSVideo *,unsigned);
void ds_video_set_display(DSVideo *,unsigned,uint32_t);
/* GX bank enums identify physical A..I banks by bit; return previous routing.
 * Invalid masks or an already assigned physical bank reject without mutation. */
int ds_video_route(DSVideo *,unsigned,unsigned,unsigned *);
/* Physical bank backing; transfers resolve the currently selected opening
 * BG/OBJ layouts. Unsupported layouts reject, never alias separate stores. */
int ds_video_transfer(DSVideo *,unsigned,size_t,const void *,size_t);
