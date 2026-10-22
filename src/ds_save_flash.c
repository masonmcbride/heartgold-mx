#include "ds_save_flash.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int flash_fd = -1;
static int fail_after = -1;
static uint32_t partial_bytes;
static unsigned write_count;
static uint32_t last_write_size;
static int sync_fault;
static char flash_error[256];
static int fail(const char *operation) {
    snprintf(flash_error, sizeof(flash_error), "%s: %s", operation, strerror(errno));
    return 0;
}
static int transfer(int writing, uint32_t offset, void *data, uint32_t size) {
    if (flash_fd < 0 || offset > DS_SAVE_FLASH_SIZE || size > DS_SAVE_FLASH_SIZE-offset || (!data && size)) {
        errno = EINVAL; return fail("flash range/device");
    }
    uint32_t done = 0;
    while (done < size) {
        ssize_t n = writing ? pwrite(flash_fd, (char *)data+done, size-done, offset+done)
                            : pread(flash_fd, (char *)data+done, size-done, offset+done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { if (n == 0) errno = EIO; return fail(writing ? "write flash" : "read flash"); }
        done += (uint32_t)n;
    }
    return 1;
}
static int open_flash(const char *path,int create) {
    if (flash_fd >= 0 || !path || !*path) { errno = EINVAL; return fail("open flash"); }
    flash_error[0] = 0;
    int created = 0;
    if(create) {
        flash_fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (flash_fd >= 0) created = 1;
        else if (errno == EEXIST) flash_fd = open(path, O_RDWR);
    } else flash_fd=open(path,O_RDWR);
    if (flash_fd < 0) return fail("open flash");
    struct stat st;
    if (fstat(flash_fd, &st) < 0) { fail("stat flash"); goto error; }
    if (!created && st.st_size != DS_SAVE_FLASH_SIZE) { errno = EINVAL; fail("flash size (expected 512 KiB)"); goto error; }
    if (created) {
        unsigned char erased[4096]; memset(erased, 0xff, sizeof(erased));
        for (uint32_t off = 0; off < DS_SAVE_FLASH_SIZE; off += sizeof(erased))
            if (!transfer(1, off, erased, sizeof(erased))) goto error;
        if (fsync(flash_fd) < 0) { fail("sync new flash"); goto error; }
        /* Persist the directory entry too: a successful first save must survive restart. */
        char parent[4096]; size_t len = strlen(path);
        if (len >= sizeof(parent)) { errno = ENAMETOOLONG; fail("flash directory"); goto error; }
        memcpy(parent, path, len+1);
        char *slash = strrchr(parent, '/');
        if (slash) { if (slash == parent) slash[1] = 0; else *slash = 0; }
        else strcpy(parent, ".");
        int dirfd = open(parent, O_RDONLY);
        if (dirfd < 0) { fail("open flash directory"); goto error; }
        int result = fsync(dirfd); int saved_errno = errno; close(dirfd); errno = saved_errno;
        if (result < 0) { fail("sync flash directory"); goto error; }
    }
    fail_after = -1;
    partial_bytes = 0; sync_fault=0; write_count = 0; last_write_size = 0;
    return 1;
error:
    close(flash_fd); flash_fd = -1; return 0;
}
int ds_save_flash_open(const char *path) { return open_flash(path,1); }
int ds_save_flash_open_existing(const char *path) { return open_flash(path,0); }
void ds_save_flash_close(void) { if (flash_fd >= 0) close(flash_fd); flash_fd = -1; fail_after = -1; }
const char *ds_save_flash_last_error(void) { return flash_error; }
void ds_save_flash_fault_after(int count) { fail_after = count; partial_bytes = 0;sync_fault=0; }
void ds_save_flash_fault_partial_after(int count,uint32_t prefix) { fail_after=count;partial_bytes=prefix;sync_fault=0; }
void ds_save_flash_fault_sync_after(int count) { fail_after=count;sync_fault=1;partial_bytes=0; }
unsigned ds_save_flash_write_count(void) { return write_count; }
uint32_t ds_save_flash_last_write_size(void) { return last_write_size; }
int ds_save_flash_read(uint32_t off, void *dst, uint32_t size) { return transfer(0,off,dst,size); }
int ds_save_flash_write(uint32_t off, const void *src, uint32_t size) {
    write_count++; last_write_size=size;
    if (fail_after == 0 && !sync_fault) {
        uint32_t prefix=partial_bytes < size ? partial_bytes : size;
        if(prefix && (!transfer(1,off,(void *)src,prefix) || fsync(flash_fd)<0)) return fail("partial flash write");
        errno = EIO; return fail("injected flash write");
    }
    if (!transfer(1,off,(void *)src,size)) return 0;
    if(fail_after==0 && sync_fault){errno=EIO;return fail("injected flash sync");}
    if (fsync(flash_fd) < 0) return fail("sync flash write");
    unsigned char verify[4096];
    for (uint32_t done = 0; done < size;) {
        uint32_t n = size-done; if (n > sizeof(verify)) n = sizeof(verify);
        if (!transfer(0,off+done,verify,n)) return 0;
        if (memcmp(verify,(const char *)src+done,n)) { errno = EIO; return fail("verify flash write"); }
        done += n;
    }
    if (fail_after > 0) fail_after--;
    return 1;
}
