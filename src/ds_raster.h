#pragma once
#include "ds_model.h"
#include "ds_texture.h"
typedef struct { DSSceneVertex v[3]; } DSRasterTriangle;
typedef struct { DSTexture texture;uint32_t params,polygon;int textured;double uv_u,uv_v;int uv_transform;double uv_matrix[6]; } DSRasterMaterial;
typedef struct { double yaw,pitch,scale,x,y,z,pan_x,pan_y;
    /* Original SDK view/projection transport; zero preserves viewer camera. */
    unsigned projection_kind; DSTransform view; double projection[16]; } DSCamera;
/* Orthographic host renderer. RGBA8 output, opaque depth + sorted alpha pass.
 * No DS lighting, culling or hardware raster precision; supports native affine UV matrices. */
int ds_raster_draw(const DSRasterTriangle *,size_t,const DSRasterMaterial *,size_t,const DSCamera *,uint8_t *,unsigned,unsigned);

int ds_raster_project(const DSCamera *,const double world[3],unsigned,unsigned,double screen[3]);

int ds_camera_look_at(DSCamera *,const int32_t [3],const int32_t [3],const int32_t [3]);
int ds_camera_perspective(DSCamera *,const int32_t [5]);
int ds_camera_ortho(DSCamera *,const int32_t [6]);
