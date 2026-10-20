#include "compat/pokemon.h"
#include "battle_runtime.h"
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
static void timeout_probe(int sig){(void)sig;void *frames[32];int n=backtrace(frames,32);backtrace_symbols_fd(frames,n,2);_exit(2);}
int main(void){
    signal(SIGALRM,timeout_probe);alarm(5);
    Party party;Party_InitWithMaxSize(&party,6);Pokemon *mon=AllocMonZeroed(HEAP_ID_BATTLE);CreateMon(mon,155,5,31,FALSE,0,0,0);Party_AddMon(&party,mon);Heap_Free(mon);
    char error[256]={0};HGBattle *battle=hg_battle_new(&party,19,2,0,NULL,error,sizeof(error));if(!battle){fprintf(stderr,"Battle init: %s\n",error);return 1;}
    int status;unsigned ticks=0;
    do{status=hg_battle_tick(battle);if(status==HG_BATTLE_MESSAGE){puts(hg_battle_message(battle));hg_battle_input(battle,1);}else if(status==HG_BATTLE_COMMAND||status==HG_BATTLE_MOVES)hg_battle_input(battle,1);}while(status!=HG_BATTLE_FAILED&&status!=HG_BATTLE_FINISHED&&++ticks<10000);
    if(status==HG_BATTLE_FAILED)fprintf(stderr,"Battle step %u: %s\n",ticks,hg_battle_error(battle));else printf("Battle status %d result %u at %u frames\n",status,hg_battle_result(battle),ticks);
    hg_battle_free(battle);return status!=HG_BATTLE_FINISHED;
}
