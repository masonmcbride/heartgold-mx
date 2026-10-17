#include "compat/overlays.h"
static struct { int registered,loaded; } modules[256];
int hg_overlay_register(FSOverlayID id) {
    if(id<0 || id>=256 || modules[id].registered)return 0;
    modules[id].registered=1;return 1;
}
int hg_overlay_unregister(FSOverlayID id) {
    if(id<0 || id>=256 || !modules[id].registered || modules[id].loaded)return 0;
    modules[id].registered=0;return 1;
}
unsigned hg_overlay_live(void) { unsigned n=0;for(unsigned i=0;i<256;i++)n+=modules[i].loaded;return n; }
static void require_module(FSOverlayID id,int loaded) {
    if(id<0 || id>=256 || !modules[id].registered || modules[id].loaded!=loaded) {
        fprintf(stderr,"Unsupported or invalid native overlay lifecycle: id=%d expected_loaded=%d\n",id,loaded);abort();
    }
}
BOOL HandleLoadOverlay(FSOverlayID id,PMOverlayLoadType type) {
    require_module(id,0);
    if(type!=OVY_LOAD_ASYNC) { fprintf(stderr,"Unsupported native overlay load mode %d\n",type);abort(); }
    modules[id].loaded=1;return TRUE;
}
void UnloadOverlayByID(FSOverlayID id) { require_module(id,1);modules[id].loaded=0; }
