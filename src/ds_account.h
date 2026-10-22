#pragma once
#include <stdint.h>
/* Cartridge account storage compatibility. This creates a local temporary
 * identity, not a registered network account or an online service. */
typedef struct { uint8_t bytes[64]; } DSAccountData;
void ds_account_create(DSAccountData *,uint32_t game_code,uint64_t console_id,uint32_t player_id);
int ds_account_valid(const DSAccountData *);
void ds_account_clear_dirty(DSAccountData *);
