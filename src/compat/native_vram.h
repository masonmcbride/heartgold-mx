#pragma once
#include <stdint.h>
/* Native storage only: no NNS task is shared with ARM. Original GF manager
 * owns this borrowed task arena and its lifetime. Destination adapters remain
 * open; this bounded transport currently admits an empty transfer queue. */
typedef struct { uint32_t type,destination,size;const void *source; } NNSGfdVramTransferTask;
void NNS_GfdInitVramTransferManager(NNSGfdVramTransferTask *,uint32_t);
void NNS_GfdDoVramTransfer(void);
void hg_native_vram_detach(NNSGfdVramTransferTask *);
/* Native opaque allocation identities, not packed DS VRAM addresses. The work
 * arena is owned by original GF3DVramMan; resource views retain existing bytes. */
uint32_t hg_native_texture_work_size(uint32_t);
void hg_native_texture_pool_init(uint32_t,uint32_t,void *,uint32_t,int);
void hg_native_palette_pool_init(uint32_t,void *,uint32_t,int);
void hg_native_texture_pool_detach(void *);
uint64_t hg_native_texture_reserve(unsigned,uint32_t);
int hg_native_texture_valid(unsigned,uint64_t);
int hg_native_texture_release(unsigned,uint64_t);
void hg_native_texture_reset(unsigned);
int hg_native_texture_pool_active(unsigned);
uint32_t hg_native_texture_pool_used(unsigned);
/* Native location views, never SDK binary layouts or ARM records. Initial
 * empty proxies carry no location; image uploads remain an open contract. */
typedef struct {uint32_t locations[3];} HGNativeImageProxy;
void hg_native_image_proxy_init(HGNativeImageProxy *);
uint32_t hg_native_image_proxy_location(const HGNativeImageProxy *,int);

typedef struct NNSG2dPaletteData {void *pRawData;uint32_t szByte,fmt,bExtendedPlt;} NNSG2dPaletteData;
int hg_native_palette_unpack(void *,NNSG2dPaletteData *);
