#include "game_name.h"
#include <string.h>
#include "charmap.inc"
int hg_name_encode(const char *name,uint16_t *out,unsigned max){
    if(!name||!out||!max||max>19)return 0;uint16_t encoded[20]={0};unsigned count=0;const unsigned char *p=(const unsigned char *)name;
    while(*p){if(count==max)return 0;uint32_t cp=*p++;
        if(cp>=128){unsigned tail=cp>=0xe0&&cp<0xf0?2:cp>=0xc2&&cp<0xe0?1:0;if(!tail)return 0;cp&=tail==2?15:31;
            for(unsigned i=0;i<tail;i++){if((*p&0xc0)!=0x80)return 0;cp=(cp<<6)|(*p++&63);}if(cp<(tail==2?0x800u:0x80u)||(cp>=0xd800&&cp<=0xdfff))return 0;}
        unsigned glyph=0;for(unsigned i=0;i<sizeof(glyph_map)/sizeof(*glyph_map);i++)if(glyph_map[i].unicode==cp){glyph=glyph_map[i].glyph;break;}
        if(!glyph||cp<32||cp=='{'||cp=='}')return 0;encoded[count++]=(uint16_t)glyph;
    }
    if(!count)return 0;encoded[count]=0xffff;memcpy(out,encoded,(max+1)*sizeof(*out));return 1;
}
int hg_name_decode(const uint16_t *in,unsigned count,char *out,size_t capacity){
    if(!in||!out||!capacity)return 0;size_t at=0;
    for(unsigned i=0;i<count;i++){if(in[i]==0xffff){out[at]=0;return 1;}uint32_t cp=0;
        for(unsigned j=0;j<sizeof(glyph_map)/sizeof(*glyph_map);j++)if(glyph_map[j].glyph==in[i]){cp=glyph_map[j].unicode;break;}
        if(!cp||cp>=65536||at+(cp<128?1:cp<2048?2:3)>=capacity)return 0;
        if(cp<128)out[at++]=(char)cp;else if(cp<2048){out[at++]=(char)(0xc0|(cp>>6));out[at++]=(char)(0x80|(cp&63));}else{out[at++]=(char)(0xe0|(cp>>12));out[at++]=(char)(0x80|((cp>>6)&63));out[at++]=(char)(0x80|(cp&63));}
    }return 0;
}
