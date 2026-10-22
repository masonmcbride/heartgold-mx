#pragma once
#include <stddef.h>
#include <stdint.h>
/* One DS cartridge backup chip per process. No file is opened implicitly. */
#define DS_SAVE_FLASH_SIZE (512u * 1024u)
int ds_save_flash_open(const char *path);
/* Continue never creates a file when its path is missing. */
int ds_save_flash_open_existing(const char *path);
void ds_save_flash_close(void);
const char *ds_save_flash_last_error(void);
/* Negative disables injection; zero fails the next write before changing bytes. */
void ds_save_flash_fault_after(int successful_writes);
/* Test transport interruption: durably write a prefix of the failing command. */
void ds_save_flash_fault_partial_after(int successful_writes, uint32_t prefix_bytes);
/* Report a sync failure after the selected command's bytes reach the host. */
void ds_save_flash_fault_sync_after(int successful_writes);
unsigned ds_save_flash_write_count(void);
uint32_t ds_save_flash_last_write_size(void);
int ds_save_flash_read(uint32_t offset, void *dst, uint32_t size);
int ds_save_flash_write(uint32_t offset, const void *src, uint32_t size);
