#include "field_events.h"
#include "compat/resources.h"
#include "map_header.h"
#include "constants/maps.h"
static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
int hg_event_decode(const uint8_t *bytes,size_t size,HGEventView *out) {
    if(!bytes||!out||size>=2048)return 0;
    const unsigned widths[]={20,32,12,16};const uint8_t *arrays[4];unsigned counts[4];size_t offset=0;
    for(unsigned i=0;i<4;i++) {
        if(offset>size||size-offset<4)return 0;
        counts[i]=word(bytes+offset);offset+=4;
        if(counts[i]>(size-offset)/widths[i])return 0;
        arrays[i]=bytes+offset;offset+=(size_t)counts[i]*widths[i];
    }
    if(offset!=size)return 0;
    *out=(HGEventView){arrays[0],arrays[1],arrays[2],arrays[3],counts[0],counts[1],counts[2],counts[3]};return 1;
}
int hg_event_load(unsigned map,HGEventView *out) {
    if(map>=MAP_ID_MAX||!out)return 0;const uint8_t *bytes;size_t size;
    return native_resource_member(NARC_fielddata_eventdata_zone_event,0,MapHeader_GetEventsBank(map),&bytes,&size)&&hg_event_decode(bytes,size,out);
}
