#pragma once
#include "compat/field.h"
/* Exact Q12 scalars/vectors; native graphics handles below are deliberately not
 * SDK binary layouts. They own decoded resources at the compatibility boundary. */
typedef s32 fx32;
typedef struct { fx32 x,y,z; } VecFx32;
typedef struct { fx32 m[9]; } MtxFx33;
#define FX32_ONE 4096
#define FX32_CONST(x) ((fx32)((x)*4096))
typedef struct HGPropModel NNSG3dResMdl;
typedef struct { NNSG3dResMdl *model;void *anmMat,*anmJnt,*anmVis; } NNSG3dRenderObj;
typedef struct Field3dRenderObjManager Field3dRenderObjManager;
typedef struct MapPropAnimationManager MapPropAnimationManager;
typedef struct AreaDataManager_Sub8AC AreaDataManager_Sub8AC;
typedef struct AreaDataManager AreaDataManager;
typedef struct ModelAttributes ModelAttributes;
#define POKEHEARTGOLD_FIELD_MAP_LOAD_MANAGER_H
#define POKEHEARTGOLD_FIELD_AREA_DATA_H
#define POKEHEARTGOLD_FIELD_MODEL_ATTRIBUTES_H
#define GUARD_POKEHEARTGOLD_FIELD_OVERLAY_01_021EA6C4_H
#define POKEHEARTGOLD_GF_3D_RENDER_H
#define GUARD_POKEHEARTGOLD_UNK_0201F990_H
#define POKEHEARTGOLD_UNK_02020B8C_H
#define POKEHEARTGOLD_UNK_02097268_H
#include "prop_types.inc"
#include "field/map_prop.h"
void *Heap_AllocAtEnd(enum HeapID,u32);
void MI_CpuClearFast(void *,u32);
void NARC_ReadFile(NARC *,u32,void *);
BOOL Field3dRenderObjManager_IsResFileHeaderLoadedByIndex(Field3dRenderObjManager *,int);
Field3dRenderObj *Field3dRenderObjManager_GetOrAllocRenderObjByID(Field3dRenderObjManager *,int);
BOOL Field3dRenderObjManager_FreeRenderObj(Field3dRenderObjManager *,Field3dRenderObj *);
void GetSafariObjectConfig(SafariObjectConfig *,int,int);
BOOL sub_0201F990(NNSG3dResMdl *,const VecFx32 *,const MtxFx33 *,VecFx32 *);
AreaDataManager_Sub8AC *ov01_021FB9F4(AreaDataManager *);
void ov01_021EA9B0(ModelAttributes *,NNSG3dResMdl *,u32);
void ov01_021EA7F8(int,AreaDataManager_Sub8AC *,u16 *);
void ov01_021EA804(int,AreaDataManager_Sub8AC *,u16 *,u16 *);
u16 (*ov01_021EA81C(int,AreaDataManager_Sub8AC *))[2];
void GF3dRender_DrawModel(NNSG3dRenderObj *,const VecFx32 *,const MtxFx33 *,const VecFx32 *);
void NNS_G3dGlbSetBaseTrans(const VecFx32 *);
void NNS_G3dGlbSetBaseRot(const MtxFx33 *);
void NNS_G3dGlbSetBaseScale(const VecFx32 *);
void NNS_G3dGlbFlush(void);
void NNS_G3dDraw1Mat1Shp(const NNSG3dResMdl *,int,int,BOOL);
void sub_02020D2C(MtxFx33 *,VecFx32 *);
