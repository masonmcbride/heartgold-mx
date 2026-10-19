#include "ds_brightness.h"
int ds_brightness_apply(uint8_t *pixels,size_t count,int level) {
    if(!pixels||level< -16||level>16||count>SIZE_MAX/4)return 0;
    if(!level)return 1;
    for(size_t i=0;i<count;i++)for(unsigned c=0;c<3;c++) {
        unsigned value=pixels[4*i+c];
        pixels[4*i+c]=(uint8_t)(level<0?value*(16+level)/16:value+(255-value)*level/16);
    }
    return 1;
}
