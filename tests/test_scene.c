#include "game_font.h"
#include "item.h"
#include "constants/items.h"
#include "game_scene.h"
#include "font_data.h"
#include "ds_scene_graphics.h"
#include "compat/resources.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);return 1; } } while(0)
static DSBlob member(int a,int m) { DSBlob b={0};if(!native_resource_member(a,0,m,&b.data,&b.size))abort();return b; }
int main(int argc,char **argv) {
    /* Original glyph loader and decompression run in direct and lazy modes. */
    GenerateFontHalfRowLookupTable(1,0,2);
    struct FontData *direct=FontData_New(NARC_graphic_font,0,FONTARC_MODE_DIRECT,FALSE,HEAP_ID_DEFAULT),*lazy=FontData_New(NARC_graphic_font,0,FONTARC_MODE_LAZY,FALSE,HEAP_ID_DEFAULT);
    _Alignas(4) GlyphInfo a={0},b={0};TryLoadGlyph(direct,299,&a);TryLoadGlyph(lazy,299,&b);
    CHECK(a.width==6 && a.height==16 && !memcmp(&a,&b,sizeof(a)));
    const u16 text[]={299,333,478,392,EOS};CHECK(GetStringWidth(direct,text,0)==19);
    FontData_ModeSwitch(lazy,FONTARC_MODE_DIRECT,HEAP_ID_DEFAULT);TryLoadGlyph(lazy,299,&b);CHECK(!memcmp(&a,&b,sizeof(a)));
    FontData_Delete(direct);FontData_Delete(lazy);
    HGFont *font=hg_font_new();CHECK(font);CHECK(hg_font_width(font,"Ai é")==19);CHECK(hg_font_width(font,"")==0);
    uint8_t *canvas=calloc(256*192,4);CHECK(canvas);hg_font_draw(font,"Poké Ball",canvas,256,192,4,4,240,0x303030,0xb0b0b0);
    unsigned ink=0;for(unsigned i=3;i<256*192*4;i+=4)ink+=canvas[i]!=0;CHECK(ink>80 && ink<1000);hg_font_free(font);
    DSBlob chars=member(NARC_itemtool_itemdata_item_icon,4),pal=member(NARC_itemtool_itemdata_item_icon,5),cells=member(NARC_itemtool_itemdata_item_icon,1),anim=member(NARC_itemtool_itemdata_item_icon,0);
    memset(canvas,0,256*192*4);CHECK(ds_draw_cell(chars,pal,cells,0,canvas,256,192,16,16));
    uint8_t expected[4096],actual[4096];
    for(unsigned id=0;id<=ITEM_MAX;id++) {
        CHECK(hg_item_icon(id,expected));memset(actual,0,sizeof(actual));
        CHECK(ds_draw_cell(member(NARC_itemtool_itemdata_item_icon,GetItemIndexMapping(id,ITEMNARC_NCGR)),member(NARC_itemtool_itemdata_item_icon,GetItemIndexMapping(id,ITEMNARC_NCLR)),cells,0,actual,32,32,16,16));
        for(unsigned pixel=0;pixel<1024;pixel++) { CHECK(actual[pixel*4+3]==expected[pixel*4+3]);if(actual[pixel*4+3])CHECK(!memcmp(actual+pixel*4,expected+pixel*4,4)); }
    }
    DSAnimFrame frame;CHECK(ds_animation_frame(anim,0,0,&frame));CHECK(frame.cell==0);CHECK(ds_animation_frame(anim,0,UINT64_MAX,&frame));CHECK(!ds_animation_frame(anim,1,0,&frame));
    DSBlob bagpal=member(NARC_native_bag,3),bagchars=member(NARC_native_bag,2),bagcells=member(NARC_native_bag,1),baganim=member(NARC_native_bag,0);
    CHECK(ds_animation_frame(baganim,8,0,&frame));CHECK(frame.rotation!=0 && frame.scale_x==4096);
    for(unsigned p=0;p<8;p++)for(unsigned t=0;t<10;t++)CHECK(ds_draw_animation(bagchars,bagpal,bagcells,baganim,8+p,t,canvas,256,192,128,96));
    CHECK(ds_draw_tilemap(member(NARC_native_bag,7),member(NARC_native_bag,8),member(NARC_native_bag,54),canvas,256,192));
    /* Truncated resources fail; mutate offsets under sanitizers to check bounds. */
    for(size_t length=0;length<cells.size;length++)CHECK(!ds_draw_cell(chars,pal,(DSBlob){cells.data,length},0,canvas,256,192,16,16));
    for(size_t length=0;length<anim.size;length++)CHECK(!ds_animation_frame((DSBlob){anim.data,length},0,0,&frame));
    CHECK(!ds_draw_cell(chars,pal,cells,9999,canvas,256,192,16,16));
    CHECK(!ds_draw_animation(chars,pal,cells,anim,0,0,canvas,256,192,INT32_MAX,INT32_MIN));
    uint8_t *mutated=malloc(baganim.size);CHECK(mutated);u32 random=0x74321;
    for(unsigned i=0;i<2048;i++) {
        memcpy(mutated,baganim.data,baganim.size);random=random*1664525u+1013904223u;size_t offset=24+random%(baganim.size-24);random=random*1664525u+1013904223u;mutated[offset]=(u8)random;
        ds_animation_frame((DSBlob){mutated,baganim.size},8,UINT64_MAX,&frame);
    }
    free(mutated);
    CHECK(ds_draw_tilemap(member(NARC_native_bag,46),member(NARC_native_bag,38),member(NARC_native_bag,39),canvas,256,192));
    CHECK(!ds_draw_cell(chars,pal,(DSBlob){cells.data,32},0,canvas,256,192,16,16));
    CHECK(!ds_draw_tilemap(chars,pal,(DSBlob){0},canvas,256,192));
    HGInventory *inventory=hg_inventory_new();HGScene *scene=hg_scene_new();uint8_t *screens=malloc(HG_SCENE_BYTES);CHECK(screens);
    for(unsigned pocket=0;pocket<8;pocket++) {
        CHECK(hg_scene_render(scene,inventory,0,screens));CHECK(hg_scene_render(scene,inventory,7,screens));
        hg_inventory_action(inventory,HG_NEXT_POCKET);
    }
    CHECK(hg_scene_click(inventory,280,70));CHECK(hg_inventory_pocket(inventory)==0);
    CHECK(hg_scene_click(inventory,340,70));CHECK(hg_inventory_pocket(inventory)==1);
    CHECK(hg_scene_click(inventory,30,60));CHECK(hg_inventory_selected(inventory)==2);
    CHECK(!hg_scene_click(inventory,260,90));
    CHECK(hg_scene_render(scene,inventory,0,screens));
    if(argc==2) {
        FILE *file=fopen(argv[1],"wb");CHECK(file);fprintf(file,"P6\n%d %d\n255\n",HG_SCENE_WIDTH,HG_SCENE_HEIGHT);
        for(unsigned i=0;i<HG_SCENE_WIDTH*HG_SCENE_HEIGHT;i++)CHECK(fwrite(screens+i*4,1,3,file)==3);CHECK(!fclose(file));
    }
    free(screens);hg_scene_free(scene);hg_inventory_free(inventory);
    free(canvas);puts("Original font loading/expansion, NCER placement, NANR transforms and bag tilemaps passed");return 0;
}
