#include "field_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* JSON strings emitted without assuming diagnostics contain no punctuation. */
static void quote(const char *s) {
    putchar('"');for(const unsigned char *p=(const unsigned char *)s;*p;p++) {
        if(*p=='"'||*p=='\\')putchar('\\');
        if(*p<32)printf("\\u%04x",*p);else putchar(*p);
    }putchar('"');
}
int main(int argc,char **argv) {
    int characters=argc==3 && !strcmp(argv[1],"--character");
    if(argc!=2 && !characters)return 2;
    if(characters)argv++;
    char *end;unsigned long id=strtoul(argv[1],&end,10);
    if(!argv[1][0] || *end || id>=(characters?hg_character_count():hg_field_map_count()))return 2;
    char error[256];HGFieldView *view=characters?hg_character_view_load((unsigned)id,error,sizeof(error)):hg_field_view_load((unsigned)id,error,sizeof(error));
    size_t triangles=hg_field_view_triangles(view);int rendered=0;
    if(view) { unsigned char pixels[96*96*4];DSCamera camera;hg_field_view_camera(view,&camera,96,96);
        rendered=hg_field_view_render(view,&camera,pixels,96,96);if(!rendered)snprintf(error,sizeof(error),"Rasterization failed"); }
    printf("{\"id\":%lu,\"name\":",id);quote(characters?hg_character_name((unsigned)id):hg_field_map_name((unsigned)id));
    printf(",\"rendered\":%s,\"triangles\":%zu,\"error\":",rendered?"true":"false",triangles);quote(error);puts("}");
    hg_field_view_free(view);return 0;
}
