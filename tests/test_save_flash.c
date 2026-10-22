#include "ds_save_flash.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr,"flash check failed at line %d: %s\n",__LINE__,ds_save_flash_last_error()); \
        exit(1); \
    } \
} while (0)

int main(void) {
    char path[]="/tmp/heartgold-flash-XXXXXX";
    int fd=mkstemp(path);
    CHECK(fd>=0);
    close(fd);
    CHECK(unlink(path)==0);
    CHECK(ds_save_flash_open(path));

    unsigned char data[4096], got[4096];
    CHECK(ds_save_flash_read(DS_SAVE_FLASH_SIZE-sizeof(got),got,sizeof(got)));
    for (unsigned i=0;i<sizeof(got);i++) CHECK(got[i]==255);
    memset(data,0x67,sizeof(data));
    CHECK(ds_save_flash_write(123,data,sizeof(data)));
    CHECK(!ds_save_flash_write(DS_SAVE_FLASH_SIZE-1,data,2));
    CHECK(!ds_save_flash_read(DS_SAVE_FLASH_SIZE-1,got,2));
    CHECK(!ds_save_flash_open(path)); /* A live cartridge cannot be replaced. */

    ds_save_flash_fault_after(0);
    CHECK(!ds_save_flash_write(123,"bad",3));
    ds_save_flash_close();
    CHECK(ds_save_flash_open(path));
    CHECK(ds_save_flash_read(123,got,sizeof(got)));
    CHECK(!memcmp(data,got,sizeof(data)));
    ds_save_flash_close();
    CHECK(!ds_save_flash_read(0,got,1));

    /* Existing malformed files are rejected, never truncated into a new save. */
    fd=open(path,O_WRONLY|O_TRUNC);
    CHECK(fd>=0);
    CHECK(write(fd,"keep",4)==4);
    close(fd);
    CHECK(!ds_save_flash_open(path));
    fd=open(path,O_RDONLY);
    CHECK(fd>=0);
    CHECK(read(fd,got,sizeof(got))==4);
    CHECK(!memcmp(got,"keep",4));
    close(fd);
    CHECK(unlink(path)==0);
    puts("flash durability, range, failure, reopen and malformed-file checks pass");
    return 0;
}
