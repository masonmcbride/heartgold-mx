#import <Cocoa/Cocoa.h>
#include "battle_view.h"
#include "game_font.h"
#include <math.h>
#include "compat/resources.h"
#include "filesystem_files_def.h"
const char *hg_pokemon_species_name(unsigned);
struct HGBattleView {HGFont *font;unsigned keys[2];uint8_t sprites[2][160*80*4];};
HGBattleView *hg_battle_view_new(void){HGBattleView *v=calloc(1,sizeof(*v));if(!v)return NULL;v->font=hg_font_new();v->keys[0]=v->keys[1]=~0u;if(!v->font){free(v);return NULL;}return v;}
void hg_battle_view_free(HGBattleView *v){if(v){hg_font_free(v->font);free(v);}}
static int sprite(HGBattleView *v,unsigned side,HGBattleMonView *mon){
    unsigned key=mon->species*8+mon->shiny*4+(mon->gender==1?2:0)+(side==0);
    if(v->keys[side]==key)return 1;
    const uint8_t *bytes;size_t length;if(mon->form||!native_resource_member(HG_RESOURCE_BATTLE_PNG,0,key,&bytes,&length))return 0;
    @autoreleasepool {
        NSBitmapImageRep *image=[[NSBitmapImageRep alloc] initWithData:[NSData dataWithBytes:bytes length:length]];
        if(!image||image.pixelsWide!=160||image.pixelsHigh!=80)return 0;
        for(unsigned y=0;y<80;y++)for(unsigned x=0;x<160;x++){
            NSColor *color=[[image colorAtX:x y:y] colorUsingColorSpace:NSColorSpace.deviceRGBColorSpace];if(!color)return 0;
            CGFloat r,g,b,a;[color getRed:&r green:&g blue:&b alpha:&a];uint8_t *p=v->sprites[side]+4*(y*160+x);
            p[0]=(uint8_t)(r*255);p[1]=(uint8_t)(g*255);p[2]=(uint8_t)(b*255);p[3]=(uint8_t)(a*255);
        }
    }
    v->keys[side]=key;return 1;
}
static void rect(uint8_t *p,unsigned x,unsigned y,unsigned w,unsigned h,uint32_t color){
    for(unsigned yy=y;yy<y+h&&yy<192;yy++)for(unsigned xx=x;xx<x+w&&xx<512;xx++){uint8_t *d=p+4*(yy*512+xx);d[0]=color>>16;d[1]=color>>8;d[2]=color;d[3]=255;}
}
static void text(HGBattleView *v,uint8_t *p,const char *s,int x,int y,unsigned width){char clean[1024];size_t n=0;while(s[n]&&n+1<sizeof(clean)){clean[n]=s[n]=='\r'?'\n':s[n];n++;}clean[n]=0;hg_font_draw(v->font,clean,p,512,192,x,y,width,0x202830,0xe0e4e8);}
int hg_battle_view_render(HGBattleView *v,HGBattle *battle,int state,unsigned choice,uint8_t *out,unsigned width,unsigned height){
    if(!v||!battle||!out||width<512||height<192)return 0;
    uint8_t pixels[512*192*4];rect(pixels,0,0,512,192,0xe8efdf);rect(pixels,0,0,254,50,0xf2f6f4);rect(pixels,256,0,256,192,0xf1f3f5);rect(pixels,254,0,4,192,0x687880);
    /* Native framing; original scene-specific backgrounds remain unported. */
    for(unsigned y=0;y<146;y++)for(unsigned x=0;x<254;x++){
        int a=(int)x-198,b=(int)y-87,c=(int)x-50,d=(int)y-139;
        if(a*a*196+b*b*2500<2500*196||c*c*100+d*d*3844<3844*100)rect(pixels,x,y,1,1,0xb7c99c);
    }
    rect(pixels,4,4,147,33,0x708878);rect(pixels,5,5,145,31,0xfcfcf4);
    rect(pixels,114,96,139,46,0x708878);rect(pixels,115,97,137,44,0xfcfcf4);
    for(unsigned side=0;side<2;side++){
        HGBattleMonView mon;if(!hg_battle_mon(battle,side,&mon))return 0;
        if(!mon.species)continue;if(!sprite(v,side,&mon))return 0;
        unsigned left=side?160:12,top=side?20:66;
        if(mon.hp)for(unsigned y=0;y<80;y++)for(unsigned x=0;x<80;x++){
            const uint8_t *s=v->sprites[side]+4*(y*160+x);uint8_t *d=pixels+4*((top+y)*512+left+x);unsigned a=s[3];for(unsigned c=0;c<3;c++)d[c]=(uint8_t)((s[c]*a+d[c]*(255-a))/255);
        }
        unsigned x=side?9:119,y=side?8:99;char line[96];snprintf(line,sizeof(line),"%s  Lv%u",hg_pokemon_species_name(mon.species),mon.level);text(v,pixels,line,x,y,130);
        rect(pixels,x,y+17,112,5,0x405048);if(mon.max_hp)rect(pixels,x+1,y+18,110*mon.hp/mon.max_hp,3,mon.hp*5<mon.max_hp?0xc84840:mon.hp*2<mon.max_hp?0xd8b838:0x48a868);
        if(!side){snprintf(line,sizeof(line),"HP %u/%u",mon.hp,mon.max_hp);text(v,pixels,line,x,y+23,130);}
    }
    rect(pixels,2,146,250,44,0x687880);rect(pixels,4,148,246,40,0xfcfcfc);text(v,pixels,hg_battle_message(battle),8,151,238);
    text(v,pixels,state==HG_BATTLE_MOVES?"Choose a move":"Battle",270,10,230);
    if(state==HG_BATTLE_COMMAND||state==HG_BATTLE_MOVES){
        HGBattleMonView mon;hg_battle_mon(battle,0,&mon);unsigned count=state==HG_BATTLE_MOVES?4:2;
        for(unsigned i=0;i<count;i++){
            unsigned y=38+28*i;if(i==choice)rect(pixels,266,y-3,236,25,0xd5e5f2);char label[128];
            if(state==HG_BATTLE_COMMAND)snprintf(label,sizeof(label),"%s",i?"Run":"Fight");
            else if(mon.moves[i]){const uint8_t *name;size_t size;if(!native_resource_member(NARC_msgdata_msg,750,mon.moves[i],&name,&size))return 0;snprintf(label,sizeof(label),"%s   PP %u",name,mon.pp[i]);}
            else snprintf(label,sizeof(label),"--");text(v,pixels,label,274,y,224);
        }
        text(v,pixels,"W/S: choose   Enter: confirm",268,170,236);
    }else text(v,pixels,"Enter / Space to continue",270,168,230);
    if(state==HG_BATTLE_MESSAGE&&strstr(hg_battle_message(battle),"HP  ")){rect(pixels,262,30,246,130,0xfcfcfc);text(v,pixels,hg_battle_message(battle),270,34,230);}
    double scale=fmin((double)width/512,(double)height/192);
    unsigned dw=(unsigned)(512*scale),dh=(unsigned)(192*scale),ox=(width-dw)/2,oy=(height-dh)/2;
    for(size_t i=0;i<(size_t)width*height;i++){out[4*i]=28;out[4*i+1]=31;out[4*i+2]=36;out[4*i+3]=255;}
    for(unsigned y=0;y<dh;y++)for(unsigned x=0;x<dw;x++)memcpy(out+4*((size_t)(oy+y)*width+ox+x),pixels+4*(((size_t)y*192/dh)*512+(size_t)x*512/dw),4);
    return 1;
}
