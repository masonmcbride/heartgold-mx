#include "ds_account.h"
#include <string.h>
static uint32_t word(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(8*i));
}
static uint32_t crc(const uint8_t *bytes) {
    uint32_t value=~0u;
    for(unsigned i=0;i<60;i++) {
        value^=bytes[i];
        for(unsigned bit=0;bit<8;bit++)value=(value>>1)^(0xedb88320u&(0u-(value&1)));
    }
    return ~value;
}
void ds_account_create(DSAccountData *account,uint32_t game,uint64_t console,uint32_t player) {
    memset(account,0,sizeof(*account));
    put(account->bytes,64);
    /* Temporary login: eleven high identity bits share a word with type 1;
     * the authenticated login/profile remains absent until network registration. */
    put(account->bytes+4,((uint32_t)(console>>32)&0x7ffu)|(1u<<11));
    put(account->bytes+8,(uint32_t)console);
    put(account->bytes+12,player);
    put(account->bytes+36,game);
    put(account->bytes+60,crc(account->bytes));
    put(account->bytes+32,1); /* Clearing dirty recalculates the stored CRC. */
}
int ds_account_valid(const DSAccountData *account) {
    return account&&word(account->bytes+60)==crc(account->bytes);
}
void ds_account_clear_dirty(DSAccountData *account) {
    put(account->bytes+32,word(account->bytes+32)&~1u);
    put(account->bytes+60,crc(account->bytes));
}
