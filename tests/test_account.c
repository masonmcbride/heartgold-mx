#include "ds_account.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"account:%d: %s\n",__LINE__,#x);exit(1);} } while(0)
int main(void) {
    DSAccountData account;
    ds_account_create(&account,0x4144414a,UINT64_C(0x123456789ab),0xdeadbeef);
    CHECK(!ds_account_valid(&account));
    ds_account_clear_dirty(&account);
    /* Fixture computed independently with Python zlib.crc32 over the documented
     * little-endian record. Covers the 43-bit ID/type packing and checksum. */
    const unsigned char prefix[]={64,0,0,0,0x23,0x09,0,0,0xab,0x89,0x67,0x45,0xef,0xbe,0xad,0xde};
    CHECK(!memcmp(account.bytes,prefix,sizeof(prefix)));
    const unsigned char checksum[]={0x36,0xac,0xcd,0xf4};
    CHECK(!memcmp(account.bytes+60,checksum,4)&&ds_account_valid(&account));
    DSAccountData before=account;
    ds_account_clear_dirty(&account);CHECK(!memcmp(&account,&before,sizeof(account)));
    for(unsigned byte=0;byte<64;byte++) {
        account=before;account.bytes[byte]^=1;CHECK(!ds_account_valid(&account));
    }
    puts("Local cartridge account layout, dirty/checksum lifecycle and corruption checks pass");
    return 0;
}
