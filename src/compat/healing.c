#include "compat/motion.h"
#include "compat/healing.h"
#include "compat/pokemon.h"
#include "field_data.h"
#include "prop_runtime.h"
#include "ds_thumb.h"
#include "ds_audio.h"
#include "healing_guest.inc"
#define BASE 0x02000000u
#define FIELD (BASE+0x8000)
#define ENV (BASE+0x8200)
#define TASK 0x03000000u
#define PROPS 0x03000004u
#define SAVE 0x03001004u
#define PARTY 0x03001000u
#define MONS 0x03002000u
#define MODEL 0x03000100u
#define SURFACE 0x03000200u
_Static_assert(sizeof(hg_healing_guest_image)<=0x8000,"Healing guest overlaps native marshaling");
struct HGHealing { u8 state[24];unsigned map,busy,changed,matrix_width,cell;double machine[3],lights[3];HGFieldProps *balls;HGFieldView *view; };
typedef struct { HGHealing *healing;FieldSystem *field;int initialized; } Boundary;
static int write_word(DSThumb *cpu,u32 address,u32 value) {
    if(address<BASE+0x8000||(address&3))return 0;u8 *p=ds_translate(&cpu->memory,address,4);if(!p)return 0;
    for(unsigned i=0;i<4;i++)p[i]=(u8)(value>>(8*i));return 1;
}
static int read_word(DSThumb *cpu,u32 address,u32 *value) {
    if(address&3)return 0;const u8 *p=ds_translate(&cpu->memory,address,4);if(!p)return 0;
    *value=p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24;return 1;
}
static int vector(DSThumb *cpu,u32 address,VecFx32 *v) {
    u32 a,b,c;if(!read_word(cpu,address,&a)||!read_word(cpu,address+4,&b)||!read_word(cpu,address+8,&c))return 0;
    *v=(VecFx32){(s32)a,(s32)b,(s32)c};return 1;
}
static int bridge(DSThumb *cpu,void *opaque) {
    Boundary *b=opaque;HGHealing *h=b->healing;const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_healing_guest_imports);i++)if(cpu->r[15]==hg_healing_guest_imports[i].address){name=hg_healing_guest_imports[i].name;break;}
    if(!name)return -1;
#define IS(n) (!strcmp(name,n))
    if(IS("SaveArray_Party_Get")){if(cpu->r[0]!=SAVE||!b->field->saveData)return -1;cpu->r[0]=PARTY;return 1;}
    if(IS("Party_GetCount")){if(cpu->r[0]!=PARTY)return -1;cpu->r[0]=Party_GetCount(hg_story_save_party(b->field->saveData));return 1;}
    if(IS("Party_GetMonByIndex")){if(cpu->r[0]!=PARTY||cpu->r[1]>=(unsigned)Party_GetCount(hg_story_save_party(b->field->saveData)))return -1;cpu->r[0]=MONS+4*cpu->r[1];return 1;}
    if(IS("GetMonData")){unsigned slot=(cpu->r[0]-MONS)/4;if(cpu->r[0]<MONS||(cpu->r[0]&3)||slot>=(unsigned)Party_GetCount(hg_story_save_party(b->field->saveData))||cpu->r[2])return -1;cpu->r[0]=GetMonData(Party_GetMonByIndex(hg_story_save_party(b->field->saveData),slot),cpu->r[1],NULL);return 1;}
    if(IS("TaskManager_GetFieldSystem")){if(cpu->r[0]!=TASK)return -1;cpu->r[0]=FIELD;return 1;}
    if(IS("TaskManager_GetEnvironment")){if(cpu->r[0]!=TASK)return -1;cpu->r[0]=ENV;return 1;}
    if(IS("Heap_AllocAtEnd")){if(cpu->r[1]!=24)return -1;cpu->r[0]=ENV;return 1;}
    if(IS("Heap_Free"))return cpu->r[0]==ENV?1:-1;
    if(IS("TaskManager_Call")){
        if(cpu->r[0]!=TASK||cpu->r[1]!=(HG_HEALING_GUEST_PokecenterAnimRun|1)||cpu->r[2]!=ENV)return -1;
        b->initialized=1;return 1;
    }
    if(IS("sub_02054C20")){
        unsigned id=cpu->r[1];if(cpu->r[0]!=FIELD||(id!=0x24&&id!=0x25)||!write_word(cpu,cpu->r[2],SURFACE+id))return -1;
        if(cpu->r[3]&&!write_word(cpu,cpu->r[3],h->cell))return -1;cpu->r[0]=1;return 1;
    }
    if(IS("MapMatrix_GetWidth")){if(cpu->r[0]!=0x03000008u)return -1;cpu->r[0]=h->matrix_width;return 1;}
    if(IS("_s32_div_f")){if(!cpu->r[1]||cpu->r[0]>65535||cpu->r[1]>65535)return -1;unsigned n=cpu->r[0],d=cpu->r[1];cpu->r[0]=n/d;cpu->r[1]=n%d;return 1;}
    if(IS("MapProp_GetTranslation")){
        if(cpu->r[1]!=SURFACE+0x24)return -1;
        double local[3]={h->machine[0]-256-512*(h->cell%h->matrix_width),h->machine[1],h->machine[2]-256-512*(h->cell/h->matrix_width)};
        for(unsigned i=0;i<3;i++)if(!write_word(cpu,cpu->r[0]+4*i,(u32)(s32)(local[i]*4096)))return -1;return 1;
    }
    if(IS("ov01_021FB90C")){
        unsigned id=cpu->r[0];if(id!=0x6b&&id!=0x25)return -1;
        u32 addr=BASE+0x8400+(id==0x6b?0:64);
        if(!write_word(cpu,addr,MODEL+id))return -1;cpu->r[0]=addr;return 1;
    }
    if(IS("NNS_G3dGetMdlSet")){
        unsigned id=cpu->r[0]-MODEL;if(id!=0x6b&&id!=0x25)return -1;
        /* Only original dictionary lookup walks this proxy; model parsing and
         * animation operate on validated native DSModel resources. */
        u32 addr=BASE+0x8500+(id==0x6b?0:64);u8 *p=ds_translate(&cpu->memory,addr,32);if(!p)return -1;
        memset(p,0,32);p[9]=1;p[14]=8;p[20]=24;cpu->r[0]=addr;return 1;
    }
    if(IS("ov01_021FB9E0")){cpu->r[0]=0;return 1;}
    if(IS("MapProp_GetRenderSurface")){cpu->r[0]=SURFACE;return 1;}
    if(IS("MapPropOneShotAnimationManager_LoadPropAnimations")){
        unsigned tag=cpu->r[2],model=cpu->r[3];
        if((tag==0x10&&model!=0x6b)||(tag==0x20&&model!=0x25)||(tag!=0x10&&tag!=0x20))return -1;
        return hg_prop_animation_load(b->field->native_prop_animations,tag,model,tag==0x20?h->lights:h->machine)?1:-1;
    }
    if(IS("MapPropManager_LoadOne")){
        VecFx32 pos,rot;if(cpu->r[1]!=0x6b||!vector(cpu,cpu->r[2],&pos)||!vector(cpu,cpu->r[3],&rot))return -1;
        cpu->r[0]=MapPropManager_LoadOne(hg_field_props_manager(h->balls),0x6b,&pos,&rot,NULL);h->changed=1;return 1;
    }
    if(IS("MapPropManager_GetMapPropByIndex_Checked_RequireActive")){
        unsigned slot=cpu->r[1];if(slot>=MAP_PROP_MAX||!MapProp_IsActive(MapPropManager_GetMapPropByIndex(hg_field_props_manager(h->balls),slot)))return -1;
        cpu->r[0]=SURFACE+slot;return 1;
    }
    /* Native render tracks bind by model and each live ball's position instead
     * of retaining a DS render-object pointer. Validate the original first-ball
     * attachment; hg_healing_view applies that track to its actual instances. */
    if(IS("MapPropOneShotAnimationManager_SetAnimationRenderObj"))return cpu->r[1]==0x10&&cpu->r[2]==0?1:-1;
    if(IS("MapPropOneShotAnimationManager_PlayAnimation"))return hg_prop_animation_play(b->field->native_prop_animations,cpu->r[1],cpu->r[2])?1:-1;
    if(IS("MapPropOneShotAnimationManager_IsAnimationLoopFinished")){
        int done=hg_prop_animation_finished(b->field->native_prop_animations,cpu->r[1]);if(done<0)return -1;cpu->r[0]=done;return 1;
    }
    if(IS("MapPropOneShotAnimationManager_UnloadAnimation"))return hg_prop_animation_unload(b->field->native_prop_animations,cpu->r[2])?1:-1;
    if(IS("MapPropManager_RemoveMapPropByIndex")){
        if(cpu->r[0]>=MAP_PROP_MAX)return -1;MapPropManager_RemoveMapPropByIndex(cpu->r[0],hg_field_props_manager(h->balls));h->changed=1;return 1;
    }
    if(IS("PlaySE")||IS("PlayFanfare"))return b->field->native_audio&&ds_audio_play(b->field->native_audio,IS("PlaySE")?DS_AUDIO_EFFECT:DS_AUDIO_FANFARE,cpu->r[0])?1:-1;
    if(IS("IsFanfarePlaying")){cpu->r[0]=ds_audio_playing(b->field->native_audio,DS_AUDIO_FANFARE,0);return 1;}
    fprintf(stderr,"Healing guest unsupported import: %s\n",name);return -1;
#undef IS
}
static int run(HGHealing *h,FieldSystem *field,int initialize,unsigned count) {
    u8 ram[65536]={0};memcpy(ram,hg_healing_guest_image,sizeof(hg_healing_guest_image));memcpy(ram+ENV-BASE,h->state,24);
    Boundary b={h,field,0};DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,sizeof(ram)};cpu.code_start=BASE;cpu.code_size=HG_HEALING_GUEST_CODE_SIZE;
    cpu.return_address=0x02200001;cpu.r[14]=cpu.return_address;cpu.r[13]=BASE+0xf000;
    cpu.r[15]=initialize==2?HG_HEALING_GUEST_Save_GetPartyLeadAlive:initialize?HG_HEALING_GUEST_PokecenterAnimCreate:HG_HEALING_GUEST_PokecenterAnimRun;cpu.r[0]=initialize==2?SAVE:initialize?FIELD:TASK;cpu.r[1]=count;
    write_word(&cpu,FIELD+0x30,0x03000008u);write_word(&cpu,FIELD+0x10,TASK);write_word(&cpu,FIELD+0x9c,PROPS);
    cpu.boundary=bridge;cpu.boundary_user=&b;
    DSThumbResult status=ds_thumb_run(&cpu,10000);
    if(status!=DS_THUMB_RETURN||cpu.r[13]!=BASE+0xf000||(initialize==1&&!b.initialized)){
        fprintf(stderr,"Healing guest failed status=%d pc=%08x address=%08x\n",status,cpu.r[15],cpu.fault_address);return -1;
    }
    memcpy(h->state,ram+ENV-BASE,24);return initialize==1?0:(int)cpu.r[0];
}
static int find_prop(FieldSystem *field,unsigned id,double world[3]) {
    HGFieldTile tile;if(!hg_field_resources_tile(field->native_scene_resources,PlayerAvatar_GetXCoord(field->playerAvatar),PlayerAvatar_GetZCoord(field->playerAvatar),&tile))return 0;
    unsigned found=0;
    for(size_t i=0;i+48<=tile.land.props_size;i+=48){const u8 *p=tile.land.props+i;u32 words[4];for(unsigned j=0;j<4;j++)words[j]=p[j*4]|(u32)p[j*4+1]<<8|(u32)p[j*4+2]<<16|(u32)p[j*4+3]<<24;
        if(words[0]!=id)continue;if(found++)return 0;
        world[0]=tile.matrix_x*512.0+256+(s32)words[1]/4096.0;world[1]=tile.altitude*8.0+(s32)words[2]/4096.0;world[2]=tile.matrix_z*512.0+256+(s32)words[3]/4096.0;
    }return found==1;
}
HGHealing *hg_healing_new(FieldSystem *field,unsigned count) {
    if(!field||!field->location||!field->native_prop_animations||!count||count>6)return NULL;
    HGFieldTile tile;if(!hg_field_resources_tile(field->native_scene_resources,PlayerAvatar_GetXCoord(field->playerAvatar),PlayerAvatar_GetZCoord(field->playerAvatar),&tile))return NULL;
    HGHealing *h=calloc(1,sizeof(*h));if(!h)return NULL;h->matrix_width=hg_field_resources_matrix_width(field->native_scene_resources);if(!h->matrix_width){free(h);return NULL;}h->cell=tile.matrix_z*h->matrix_width+tile.matrix_x;h->map=field->location->mapId;h->balls=hg_field_props_new(h->map);
    if(!h->balls||!hg_field_props_prepare(h->balls,0x6b,count)||!find_prop(field,0x24,h->machine)||!find_prop(field,0x25,h->lights)||run(h,field,1,count)<0){hg_healing_free(h);return NULL;}
    h->busy=1;return h;
}
HGHealing *hg_healing_clone(const HGHealing *source) {
    if(!source)return NULL;HGHealing *h=malloc(sizeof(*h));if(!h)return NULL;*h=*source;h->view=NULL;h->balls=hg_field_props_clone(source->balls);h->changed=1;if(!h->balls){free(h);return NULL;}return h;
}
void hg_healing_free(HGHealing *h){if(h){hg_field_view_free(h->view);hg_field_props_free(h->balls);free(h);}}
int hg_healing_tick(HGHealing *h,FieldSystem *field){if(!h||!h->busy)return 1;int result=run(h,field,0,0);if(result<0)return 0;if(result==1)h->busy=0;return result<=1;}
int hg_healing_busy(const HGHealing *h){return h&&h->busy;}
int hg_healing_present(HGHealing *h,HGPropAnimations *animations,HGFieldView *view){
    if(h&&hg_field_props_count(h->balls)&&!hg_healing_view(h,animations))return 0;
    HGPropPose poses[16],lights[16];unsigned n=hg_prop_animations_poses(animations,poses),count=0;
    for(unsigned i=0;i<n;i++)if(!h||poses[i].model!=0x6b)lights[count++]=poses[i];return hg_field_view_prop_poses(view,lights,count);
}
const HGFieldView *hg_healing_view(HGHealing *h,HGPropAnimations *animations){
    if(!h)return NULL;if(h->changed){hg_field_view_free(h->view);h->view=NULL;if(hg_field_props_count(h->balls))h->view=hg_field_props_view(h->balls);h->changed=0;}
    if(h->view){HGPropPose all[16],balls[16];unsigned n=hg_prop_animations_poses(animations,all),count=0;
        for(unsigned i=0;i<n;i++)if(all[i].model==0x6b)for(unsigned slot=0;slot<MAP_PROP_MAX;slot++){
            MapProp *prop=MapPropManager_GetMapPropByIndex(hg_field_props_manager(h->balls),slot);if(!MapProp_IsActive(prop))continue;
            balls[count]=all[i];VecFx32 v;MapProp_GetTranslation(&v,prop);balls[count].world[0]=v.x/4096.0;balls[count].world[1]=v.y/4096.0;balls[count].world[2]=v.z/4096.0;count++;
        }
        if(!hg_field_view_prop_poses(h->view,balls,count))return NULL;
    }return h->view;
}

u16 hg_healing_party_lead(SaveData *save){ HGHealing h={0};FieldSystem field={.saveData=save};int slot=run(&h,&field,2,0);GF_ASSERT(slot>=0&&slot<6);return (u16)slot; }
