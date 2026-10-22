#pragma once
#include <stddef.h>
#include <stdint.h>
#include "ds_transform.h"
/* Bounded original model-resource views; all offsets remain file-relative. */
typedef struct {
    const uint8_t *bytes;
    size_t size, shapes_offset;
    unsigned nodes, materials, shapes, declared_vertices, declared_triangles, declared_quads;
    char name[17];
} DSModel;
typedef struct { int32_t x,y,z,u,v;uint32_t color; } DSModelVertex; /* signed 20.12, mesh-local */
typedef void (*DSModelTriangle)(void *, const DSModelVertex[3]);
typedef struct { size_t commands, vertices, triangles; } DSModelStats;
int ds_model_open(const uint8_t *, size_t, unsigned index, DSModel *);
int ds_model_shape(const DSModel *, unsigned index, const uint8_t **, size_t *);
/* Supported mesh GX slice only; no material, node/SBC or matrix evaluation.
 * On failure callbacks may already have run; callers must discard partial output. */
int ds_model_geometry(const uint8_t *, size_t, DSModelTriangle, void *, DSModelStats *);

/* Host graphics coordinates after the supported model SBC slice. */
typedef struct { double x,y,z,u,v;uint32_t color;unsigned material; } DSSceneVertex;
typedef void (*DSSceneTriangle)(void *,const DSSceneVertex[3]);
int ds_model_scene(const DSModel *,DSSceneTriangle,void *,DSModelStats *);

typedef struct { char name[17],texture[17],palette[17];uint32_t texture_params,polygon_attrs,diffuse;unsigned width,height;int identity_uv,translation_uv;double uv_u,uv_v; } DSModelMaterial;
int ds_model_material(const DSModel *,unsigned,DSModelMaterial *);

int ds_model_scene_shape(const DSModel *,unsigned shape,unsigned material,DSSceneTriangle,void *,DSModelStats *);

int ds_model_node(const DSModel *,unsigned,DSTransform *);

/* Inspection only: freeze simple SBC billboard orientation in model space.
 * Not camera-facing billboard emulation or support in normal game scenes. */
int ds_model_inspection_scene(const DSModel *,DSSceneTriangle,void *,DSModelStats *);

#include "ds_joint_animation.h"
int ds_model_scene_pose(const DSModel *,int shape,int material,const DSJointPose *,DSSceneTriangle,void *,DSModelStats *);
