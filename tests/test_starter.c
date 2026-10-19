#include "compat/motion.h"
#include "compat/pokemon.h"
#include "compat/starter.h"
#include "choose_starter.h"
#include "game_stats.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
static HGStarter *start(HGStoryState *story,FieldSystem *field,Location *location) {
    memset(field,0,sizeof(*field));hg_story_bind_field(story,field);field->location=location;
    HGStarter *starter=hg_starter_begin(field,0);CHECK(starter);LaunchStarterChoiceScene(field);
    unsigned ticks=0;while(!hg_starter_pending(starter)&&ticks++<64)CHECK(hg_starter_tick(starter));CHECK(ticks<64);
    return starter;
}
int main(void) {
    const unsigned species[]={152,155,158};Location location={.mapId=61,.warpId=-1};
    for(unsigned choice=0;choice<3;choice++) {
        HGStoryState *story=hg_story_new();CHECK(story);FieldSystem field;
        HGStarter *starter=start(story,&field,&location);Party *party=hg_story_save_party(field.saveData);
        CHECK(!party->core.curCount&&field.taskman);
        for(unsigned i=0;i<3;i++){HGStarterOption option;CHECK(hg_starter_option(starter,i,&option)&&option.species==species[i]&&option.level==5&&option.hp&&option.name[0]);}
        CHECK(!hg_starter_choose(starter,3)&&hg_starter_pending(starter));
        u32 before=GameStats_GetScore(hg_story_save_stats(field.saveData));
        CHECK(hg_starter_choose(starter,choice)&&!hg_starter_pending(starter));
        unsigned ticks=0;while(hg_starter_busy(starter)&&ticks++<64)CHECK(hg_starter_tick(starter));
        CHECK(ticks<64&&!field.taskman&&party->core.curCount==1);
        Pokemon *mon=Party_GetMonByIndex(party,0);CHECK(GetMonData(mon,MON_DATA_SPECIES,NULL)==species[choice]);
        CHECK(GetMonData(mon,MON_DATA_MET_LEVEL,NULL)==5&&GetMonData(mon,MON_DATA_MET_TERRAIN,NULL)==12);
        CHECK(GetMonData(mon,MON_DATA_OT_ID,NULL)==PlayerProfile_GetTrainerID(hg_story_save_profile(field.saveData)));
        CHECK(Pokedex_CheckMonCaughtFlag(hg_story_save_dex(field.saveData),species[choice]));
        CHECK(GameStats_GetScore(hg_story_save_stats(field.saveData))>before);
        hg_starter_free(starter);CHECK(!field.native_starter);hg_story_free(story);
    }
    HGStoryState *owner=hg_story_new(),*private=hg_story_clone(owner);CHECK(owner&&private);FieldSystem field;
    HGStarter *starter=start(private,&field,&location);u32 seed=GetLCRNGSeed();hg_starter_free(starter);
    CHECK(!field.taskman&&!field.native_starter&&GetLCRNGSeed()==seed);hg_story_free(private);
    hg_story_bind_field(owner,&field);CHECK(!hg_story_save_party(field.saveData)->core.curCount);hg_story_free(owner);
    puts("Original starter task: all three choices, trainer/encounter data, party/Pokedex/score publication and private cancellation passed");return 0;
}
