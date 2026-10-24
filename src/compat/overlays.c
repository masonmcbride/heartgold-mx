#include "compat/overlays.h"
static struct { int registered,loaded,busy; HGOverlayHook load,unload;void *context; } modules[256];
int hg_overlay_register_lifecycle(FSOverlayID id,HGOverlayHook load,HGOverlayHook unload,void *context) {
    if(id<0 || id>=256 || modules[id].registered)return 0;
    modules[id].registered=1;modules[id].load=load;modules[id].unload=unload;modules[id].context=context;return 1;
}
int hg_overlay_register(FSOverlayID id) { return hg_overlay_register_lifecycle(id,NULL,NULL,NULL); }
int hg_overlay_unregister(FSOverlayID id) {
    if(id<0 || id>=256 || !modules[id].registered || modules[id].loaded || modules[id].busy)return 0;
    memset(&modules[id],0,sizeof(modules[id]));return 1;
}
unsigned hg_overlay_live(void) { unsigned n=0;for(unsigned i=0;i<256;i++)n+=modules[i].loaded;return n; }
static void require_module(FSOverlayID id,int loaded) {
    if(id<0 || id>=256 || !modules[id].registered || modules[id].loaded!=loaded) {
        fprintf(stderr,"Unsupported or invalid native overlay lifecycle: id=%d expected_loaded=%d\n",id,loaded);abort();
    }
}
BOOL HandleLoadOverlay(FSOverlayID id,PMOverlayLoadType type) {
    require_module(id,0);
    if(type!=OVY_LOAD_ASYNC && type!=OVY_LOAD_NORMAL) { fprintf(stderr,"Unsupported native overlay load mode %d\n",type);abort(); }
    /* Original async helper waits for the image read before starting it too.
     * Static native code is already resident; load hooks implement its lifetime. */
    BOOL result=FS_LoadOverlay(HG_PROCESSOR_ARM9,id);GF_ASSERT(result);return result;
}
void UnloadOverlayByID(FSOverlayID id) { require_module(id,1);GF_ASSERT(FS_UnloadOverlay(HG_PROCESSOR_ARM9,id)); }
BOOL FS_LoadOverlay(int processor,FSOverlayID id) {
    if(processor!=HG_PROCESSOR_ARM9 || id<0 || id>=256 || !modules[id].registered || modules[id].loaded || modules[id].busy)return FALSE;
    modules[id].busy=1;
    BOOL ready=!modules[id].load || modules[id].load(modules[id].context);
    modules[id].busy=0;if(!ready)return FALSE;
    modules[id].loaded=1;return TRUE;
}
BOOL FS_UnloadOverlay(int processor,FSOverlayID id) {
    if(processor!=HG_PROCESSOR_ARM9 || id<0 || id>=256 || !modules[id].registered || !modules[id].loaded || modules[id].busy)return FALSE;
    modules[id].busy=1;
    BOOL ready=!modules[id].unload || modules[id].unload(modules[id].context);
    modules[id].busy=0;if(!ready)return FALSE;
    modules[id].loaded=0;return TRUE;
}
