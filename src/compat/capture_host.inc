/* Native animation handles only; Task_GetPokemon and catch calculations remain
 * original. The view reads these phases while the original task waits. */
typedef struct NativeCaptureBall {UnkBallData data;unsigned animation,frames;} NativeCaptureBall;
UnkBallData *ov07_02233DB8(UnkStruct_134 *args){GF_ASSERT(args&&args->battleSystem==&running_battle->system);return Heap_Alloc(HEAP_ID_BATTLE,sizeof(NativeCaptureBall));}
void UnkBallData_SetBallAnimation(UnkBallData *data,s32 animation){GF_ASSERT(data&&animation>=0);((NativeCaptureBall *)data)->animation=(unsigned)animation;((NativeCaptureBall *)data)->frames=20;running_battle->capture_phase=(unsigned)animation+1;running_battle->capture_ticks++;}
BOOL ov07_02232F60(UnkBallData *data,s32 unused){(void)unused;GF_ASSERT(data);return ((NativeCaptureBall *)data)->frames&&--((NativeCaptureBall *)data)->frames;}
void ov07_02233ECC(UnkBallData *data){GF_ASSERT(data);Heap_Free(data);}
void sub_0200602C(u16 sound,s32 volume){(void)volume;PlaySE(sound);}
void sub_0201649C(void *icon,s32 hidden){GF_ASSERT(!icon&&(hidden==0||hidden==1));}
u8 PaletteData_BeginPaletteFade(PaletteData *palette,u16 mask,u16 opaque,s8 wait,u8 current,u8 end,u16 color){GF_ASSERT(palette==&running_battle->palette&&(mask==15||mask==5)&&opaque==65535&&wait==1&&((current==0&&end==16)||(current==16&&end==0))&&color==0);running_battle->capture_fade=16;return TRUE;}
u16 PaletteData_GetSelectedBuffersBitmask(PaletteData *palette){GF_ASSERT(palette==&running_battle->palette);return running_battle->capture_fade&&--running_battle->capture_fade?15:0;}
void Pokepic_StartPaletteFadeAll(PokepicManager *manager,int start,int end,int frames,int color){GF_ASSERT(!manager&&((start==0&&end==16)||(start==16&&end==0))&&frames==0&&color==0);}
void PokepicManager_DeleteAllPics(PokepicManager *manager){GF_ASSERT(!manager);running_battle->capture_phase=100;}

/* Native Pokedex entry application: render the caught species through the
 * shared battle message view and wait for explicit acknowledgement. Original
 * capture state machine owns registration, disposal and contest publication. */
void ov12_02237CC4(BattleSystem *system){GF_ASSERT(system==&running_battle->system);running_battle->hp_bars_hidden=1;}
void ov12_02237D00(BattleSystem *system){GF_ASSERT(system==&running_battle->system);running_battle->hp_bars_hidden=0;}
void ov12_02265FC4(UnkBattleSystemSub17C *bar,s32 mode){
    GF_ASSERT((bar==&running_battle->system.unk17C[0]||bar==&running_battle->system.unk17C[1])&&mode==0);
    /* Background transfers belong to the native battle renderer; the capture
     * controller's surrounding hide/show requests own visible HP bars. */
}
ObjCharTransferTasksManager *ObjCharTransfer_PopTaskManager(void){return NULL;}
void ObjCharTransfer_PushTaskManager(ObjCharTransferTasksManager *manager){GF_ASSERT(!manager);}
OverlayManager *ov18_021F8974(UnkStruct_50C *args){
    GF_ASSERT(args&&args->mon);
    unsigned species=GetMonData(args->mon,MON_DATA_SPECIES,NULL);
    snprintf(running_battle->text,sizeof(running_battle->text),"Pokedex entry: %s",hg_pokemon_species_name(species));
    running_battle->waiting=running_battle->direct_message=1;running_battle->input_pending=0;
    return (OverlayManager *)running_battle;
}
s32 ov18_021F89C8(OverlayManager *manager){GF_ASSERT(manager==(OverlayManager *)running_battle);return 1;}
void ov18_021F89D0(OverlayManager *manager){GF_ASSERT(manager==(OverlayManager *)running_battle);}

/* Native capture view owns sprite placement while original task timing runs. */
Pokepic *ov18_021F95F8(OverlayManager *manager){GF_ASSERT(manager==(OverlayManager *)running_battle);return (Pokepic *)running_battle;}
void ov18_021F95AC(OverlayManager *manager){GF_ASSERT(manager==(OverlayManager *)running_battle);}
void Pokepic_AddAttr(Pokepic *pic,int attr,int value){GF_ASSERT(pic==(Pokepic *)running_battle&&attr==POKEPIC_X);running_battle->capture_pic_x+=value;}
int Pokepic_GetAttr(Pokepic *pic,int attr){GF_ASSERT(pic==(Pokepic *)running_battle&&attr==POKEPIC_X);return running_battle->capture_pic_x;}
void Pokepic_SetAttr(Pokepic *pic,int attr,int value){GF_ASSERT(pic==(Pokepic *)running_battle&&attr==POKEPIC_X);running_battle->capture_pic_x=value;}
void PaletteData_SetAutoTransparent(PaletteData *palette,BOOL enabled){GF_ASSERT(palette==&running_battle->palette&&enabled);}
void GetPokemonSpriteCharAndPlttNarcIds(PokepicTemplate *out,Pokemon *mon,u8 facing){GF_ASSERT(out&&mon&&facing==2);memset(out,0,sizeof(*out));}
Pokepic *PokepicManager_CreatePokepic(PokepicManager *manager,PokepicTemplate *template,int x,int y,int z,int polygon,PokepicAnimScript *animation,PokepicCallback callback){
    GF_ASSERT(!manager&&template&&x==128&&y==72&&!z&&!polygon&&!animation&&!callback);running_battle->capture_pic_x=x;return (Pokepic *)running_battle;
}
void BufferPCBoxName(MessageFormat *format,u32 slot,PCStorage *storage,u32 box){GF_ASSERT(storage==&running_battle->storage&&box<NUM_BOXES);buffer_encoded_name(format,slot,storage->box_names[box],BOX_NAME_LENGTH);}
