#include "compat/pokemon.h"
#include "compat/mart.h"
#include "compat/stats.h"
#include "bag.h"
#include "inventory.h"
#include "sys_flags.h"
#include "ds_audio.h"
#include "ds_thumb.h"
#include "mart_types.inc"
#include "mart_sale_guest.inc"
#include <stdarg.h>
/* Native view of the fields consumed by the unchanged transaction functions.
 * It is never passed to an original DS task or treated as the DS MartData ABI. */
typedef struct { int visible; } ShopWidget;
typedef struct {
    PlayerProfile *playerProfile;void *inventory,*pokeathlonSave,*apricornBox;GameStats *gameStats;SaveVarsFlags *varsFlags;
    unsigned unk264,unk290,unk298;u16 *unk268;const struct MartItem *priceOverrides;u8 unk270,unk271,martType,printerID;
    u16 item;s16 quantity;int cost;ShopWidget windows[6],*sprites[2];u16 spriteDrawn[2];
} MartData;
struct HGShop {FieldSystem *field;MartData data;u16 items[ITEMS_COUNT];unsigned count,selected,maximum;int open,selling,stage;ShopWidget widgets[2];char message[256];};
static void unavailable(void){fprintf(stderr,"Unsupported specialized shop service\n");abort();}
static unsigned PokeathlonSave_GetAthletePoints(void *p){(void)p;unavailable();return 0;}
static void PokeathlonSave_SubAthletePoints(void *p,int n){(void)p;(void)n;unavailable();}
static void PokeathlonSave_SetUnkB7C_AtIndex(void *p,int i){(void)p;(void)i;unavailable();}
static void PokeathlonSave_SetUnkB78_AtIndex(void *p,int i){(void)p;(void)i;unavailable();}
static void ApricornBox_GiveApricorn(void *p,int i,int n){(void)p;(void)i;(void)n;unavailable();}
#define GiveOrTakeSeal native_mart_seal
static void native_mart_seal(void *p,int i,int n){(void)p;(void)i;(void)n;unavailable();}
/* Native text is presented immediately; widget callbacks update the retained
 * modal view rather than issuing DS window/sprite hardware commands. */
static BOOL DialogBox_IsPrintFinished(unsigned id){GF_ASSERT(!id);return TRUE;}
static void ClearFrameAndWindow2(ShopWidget *w,int unused){(void)unused;w->visible=0;}
static void Sprite_SetDrawFlag(ShopWidget *w,int value){w->visible=value;}
static void ov03_02258560(MartData *d,BOOL v){d->windows[0].visible=v;}
static void ov03_022586BC(MartData *d,int v){d->windows[1].visible=v;}
static void ov03_022582C0(MartData *d,int v){d->windows[2].visible=v;}
static void sub_02066D80(SaveVarsFlags *p){(void)p;unavailable();} /* Decoration tracking only. */
static struct {u32 newKeys;int touchNew;} native_mart_input;
#define gSystem native_mart_input
#define PAD_BUTTON_A 1
#define PAD_BUTTON_B 2
#define GF_AssertFail() GF_ASSERT(FALSE)
SaveVarsFlags *hg_story_save_vars(SaveData *);
#define Save_PlayerData_GetProfile hg_story_save_profile
#define Mart_Init hg_mart_start
#include "mart_decls.inc"
#include "mart_kernel.inc"
#undef Mart_Init
static void write32(u8 *ram,unsigned off,u32 value){memcpy(ram+off,&value,4);}
typedef struct {HGShop *shop;ItemSlot *slots;unsigned capacity;} SaleBoundary;
static int sale_bridge(DSThumb *cpu,void *opaque){
    SaleBoundary *b=opaque;HGShop *s=b->shop;const char *name=NULL;
    for(unsigned i=0;i<NELEMS(hg_mart_sale_imports);i++)if(cpu->r[15]==hg_mart_sale_imports[i].address){name=hg_mart_sale_imports[i].name;break;}
    if(!name)return -1;
    if(!strcmp(name,"TextPrinterCheckActive")&&!cpu->r[0]){cpu->r[0]=0;return 1;}
    if(!strcmp(name,"PlaySE")&&cpu->r[0]==0x643){return !s->field->native_audio||ds_audio_play(s->field->native_audio,DS_AUDIO_EFFECT,cpu->r[0])?1:-1;}
    if(!strcmp(name,"PlayerProfile_AddMoney")&&cpu->r[0]==0x03000000u){cpu->r[0]=PlayerProfile_AddMoney(s->data.playerProfile,cpu->r[1]);return 1;}
    if(!strcmp(name,"Pocket_TakeItem")&&cpu->r[0]==0x03000004u&&cpu->r[1]==b->capacity&&cpu->r[2]==s->data.item&&cpu->r[3]==(unsigned)s->data.quantity){cpu->r[0]=Pocket_TakeItem(b->slots,b->capacity,cpu->r[2],cpu->r[3]);return cpu->r[0]?1:-1;}
    if(!strcmp(name,"ov15_021F9F08")&&cpu->r[0]==0x02008000u){s->data.windows[0].visible=1;return 1;} /* Native money panel reads the real profile. */
    if(!strcmp(name,"ScheduleWindowCopyToVram")&&cpu->r[0]==0x02008034u){s->data.windows[1].visible=1;return 1;}
    return -1;
}
static int sell(HGShop *s){
    unsigned pocket=GetItemAttr(s->data.item,ITEMATTR_FIELD_POCKET,HEAP_ID_FIELD2),capacity=0;if(pocket>7)return 0;
    ItemSlot *slots=Bag_GetPocketSlotN(s->data.inventory,pocket,0);if(!slots)return 0;
    while(capacity<255&&Bag_GetPocketSlotN(s->data.inventory,pocket,capacity))capacity++;
    u8 ram[65536]={0};memcpy(ram,hg_mart_sale_image,sizeof(hg_mart_sale_image));
    write32(ram,0x8234,0x02009000);write32(ram,0x823c,0x03000000);write32(ram,0x8680,(u16)s->data.quantity);write32(ram,0x8684,s->data.cost);
    write32(ram,0x9004,0x03000004);ram[0x900d]=(u8)capacity;memcpy(ram+0x9066,&s->data.item,2);
    DSThumb cpu={0};cpu.memory=(DSMainRAM){ram,sizeof(ram)};cpu.code_start=0x02000000;cpu.code_size=HG_MART_SALE_CODE_SIZE;cpu.r[0]=0x02008000;cpu.r[15]=HG_MART_SALE_ov15_021FD24C;cpu.r[13]=0x0200f000;cpu.return_address=0x02200001;cpu.r[14]=cpu.return_address;
    SaleBoundary b={s,slots,capacity};cpu.boundary=sale_bridge;cpu.boundary_user=&b;
    DSThumbResult result=ds_thumb_run(&cpu,10000);return result==DS_THUMB_RETURN&&cpu.r[0]==24&&cpu.r[13]==0x0200f000;
}
static void refresh_sell(HGShop *s){s->count=0;for(unsigned i=1;i<ITEMS_COUNT;i++)if(Bag_GetQuantity(s->data.inventory,i,HEAP_ID_FIELD2)&&GetItemAttr(i,ITEMATTR_PRICE,HEAP_ID_FIELD2)&&!GetItemAttr(i,ITEMATTR_PREVENT_TOSS,HEAP_ID_FIELD2))s->items[s->count++]=(u16)i;if(s->selected>=s->count)s->selected=0;}
HGShop *hg_shop_new(FieldSystem *field,const u16 *stock,int selling){
    if(!field||!field->saveData||!stock||selling<0||selling>1)return NULL;
    HGShop *s=calloc(1,sizeof(*s));if(!s)return NULL;s->field=field;s->selling=selling;s->open=1;
    s->data=(MartData){.playerProfile=hg_story_save_profile(field->saveData),.inventory=hg_story_save_bag(field->saveData),.gameStats=hg_story_save_stats(field->saveData),.varsFlags=hg_story_save_vars(field->saveData),.unk268=s->items,.sprites={s->widgets,s->widgets+1}};
    if(selling)refresh_sell(s);else {unsigned n=0;while(n<64&&stock[n]!=0xffff){if(!stock[n]||stock[n]>=ITEMS_COUNT){free(s);return NULL;}n++;}if(n==64){free(s);return NULL;}s->data.unk270=ov03_02256BEC(stock,NULL,MART_TYPE_NORMAL);ov03_02256C2C(&s->data,stock,CheckFlag09A(s->data.varsFlags));s->count=s->data.unk270;}
    return s;
}
void hg_shop_free(HGShop *s){free(s);}
int hg_shop_open(const HGShop *s){return s&&s->open;}
unsigned hg_shop_count(const HGShop *s){return s?s->count:0;}
unsigned hg_shop_item(const HGShop *s,unsigned row){return s&&row<s->count?s->items[row]:0;}
int hg_shop_select(HGShop *s,unsigned row){if(!s||!s->open||s->stage||row>=s->count)return 0;s->selected=row;return 1;}
int hg_shop_stage(const HGShop *s){return s?s->stage:0;}
unsigned hg_shop_quantity(const HGShop *s){return s?(unsigned)s->data.quantity:0;}
void hg_shop_input(HGShop *s,uint16_t keys,int confirm,int cancel){
    if(!s||!s->open)return;
    if(cancel&&s->stage==HG_SHOP_RECEIPT){cancel=0;confirm=1;}
    if(cancel){if(!s->stage)s->open=0;else s->stage=HG_SHOP_LIST;return;}
    if(s->stage==HG_SHOP_LIST){
        if(s->count){if(keys&0x40)s->selected=(s->selected+s->count-1)%s->count;if(keys&0x80)s->selected=(s->selected+1)%s->count;}
        if(confirm){if(!s->count){snprintf(s->message,sizeof(s->message),"You have nothing you can sell.");s->stage=HG_SHOP_RECEIPT;return;}
            s->data.item=s->items[s->selected];s->data.cost=ov03_02258120(&s->data,s->data.item)/(s->selling?2:1);s->data.quantity=1;
            s->maximum=s->selling?Bag_GetQuantity(s->data.inventory,s->data.item,HEAP_ID_FIELD2):s->data.cost?PlayerProfile_GetMoney(s->data.playerProfile)/(unsigned)s->data.cost:0;if(!s->selling&&s->maximum>99)s->maximum=99;
            if(!s->maximum){snprintf(s->message,sizeof(s->message),"You don't have enough money.");s->stage=HG_SHOP_RECEIPT;}else s->stage=HG_SHOP_QUANTITY;
        }
    }else if(s->stage==HG_SHOP_QUANTITY){int delta=keys&0x40?1:keys&0x80?-1:keys&0x10?10:keys&0x20?-10:0;if(delta)s->data.quantity=ov03_022587E8(s->data.quantity,s->maximum,delta);if(confirm)s->stage=HG_SHOP_CONFIRM;
    }else if(s->stage==HG_SHOP_CONFIRM){if(!confirm)return;
        Bag saved_bag=*(Bag *)s->data.inventory;PlayerProfile saved_profile=*s->data.playerProfile;GameStats saved_stats=*s->data.gameStats;
        unsigned total=(unsigned)s->data.cost*(unsigned)s->data.quantity;int ok=1;
        if(s->selling){if(Bag_GetQuantity(s->data.inventory,s->data.item,HEAP_ID_FIELD2)<(unsigned)s->data.quantity)ok=0;else ok=sell(s);}
        else if(PlayerProfile_GetMoney(s->data.playerProfile)<total||!Bag_HasSpaceForItem(s->data.inventory,s->data.item,s->data.quantity,HEAP_ID_FIELD2))ok=0;
        else ok=MartData_PerformTransaction(&s->data)==TASK_MART_13;
        if(!ok){*(Bag *)s->data.inventory=saved_bag;*s->data.playerProfile=saved_profile;*s->data.gameStats=saved_stats;}
        snprintf(s->message,sizeof(s->message),"%s",ok?(s->selling?"Thank you! Your items have been sold.":"Thank you! Here are your items."):"The transaction could not complete. Check your money and Bag space.");
        s->stage=HG_SHOP_RECEIPT;s->data.windows[5].visible=ok;
    }else if(confirm){if(!s->selling&&s->data.windows[5].visible){native_mart_input.newKeys=1;unsigned result=ov03_02257F24(&s->data);native_mart_input.newKeys=0;s->data.windows[5].visible=0;if(result==TASK_MART_15){snprintf(s->message,sizeof(s->message),"You also received a Premier Ball!");return;}}
        s->stage=HG_SHOP_LIST;s->message[0]=0;if(s->selling)refresh_sell(s);
    }
}
static void append(char *out,size_t size,const char *fmt,...){size_t n=strlen(out);if(n>=size)return;va_list a;va_start(a,fmt);vsnprintf(out+n,size-n,fmt,a);va_end(a);}
void hg_shop_text(HGShop *s,char *out,size_t size){
    if(!out||!size)return;out[0]=0;if(!s)return;
    append(out,size,"POKE MART / %s\nMoney: %u\n\n",s->selling?"SELL":"BUY",PlayerProfile_GetMoney(s->data.playerProfile));
    if(s->stage==HG_SHOP_LIST){unsigned first=s->selected/7*7;for(unsigned i=first;i<s->count&&i<first+7;i++){HGItem item;if(hg_item_details(s->items[i],&item))append(out,size,"%c %s  %u\n",i==s->selected?'>':' ',item.name,GetItemAttr(item.id,ITEMATTR_PRICE,HEAP_ID_FIELD2)/(s->selling?2:1));}append(out,size,"\nW/S select  Enter choose\nEsc return to clerk");}
    else if(s->stage==HG_SHOP_RECEIPT)append(out,size,"%s\n\nEnter continue",s->message);
    else {HGItem item;if(!hg_item_details(s->data.item,&item))return;append(out,size,"%s\nOwned: %u\nQuantity: %d\nTotal: %u\n\n%s",item.name,Bag_GetQuantity(s->data.inventory,s->data.item,HEAP_ID_FIELD2),s->data.quantity,s->data.cost*s->data.quantity,s->stage==HG_SHOP_QUANTITY?"W/S +/-1  A/D +/-10\nEnter continue  Esc cancel":"Enter confirm  Esc cancel");}
}
