#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct { unsigned width,height,has_headers,has_altitudes,name_length;size_t cells; } HGMatrixLayout;
/* Validate complete lengths before the original pointer-based parser executes. */
int hg_matrix_validate(const uint8_t *,size_t,HGMatrixLayout *);

/* Borrowed, validated land sections. No serialized address becomes a pointer. */
typedef struct {
    const uint8_t *attributes, *props, *model, *collision;
    size_t props_size, model_size, collision_size;
    unsigned extra_size;
} HGLandChunk;
int hg_land_decode(const uint8_t *, size_t, HGLandChunk *);
int hg_land_tile(const HGLandChunk *, unsigned x, unsigned z, uint16_t *);
/* Matrix-global tile coordinates; returns failure for out-of-bounds/missing land.
 * Owns no returned resource pointers. Uses original map/matrix getters. */
int hg_field_attribute(unsigned map, unsigned x, unsigned z, uint16_t *);

#include <stdio.h>
/* First matching chunk; binary PPM diagnostic with the supported land scene transforms. */
int hg_field_wireframe(unsigned map, FILE *);

/* BDHC collision tables, borrowed from a validated resource. See the original
 * Thumb height-query API below for execution. */
typedef struct {
    const uint8_t *points,*normals,*constants,*planes,*strips,*indices;
    unsigned point_count,normal_count,constant_count,plane_count,strip_count,index_count;
} HGCollision;
int hg_collision_decode(const uint8_t *,size_t,HGCollision *);

/* Opaque untextured triangle diagnostic with a host depth buffer. */
int hg_field_solid(unsigned map,FILE *);

/* Host-owned resource context; not the original FieldSystem ABI. Owns its
 * original MapMatrix and decoded chunk metadata; resource bytes are borrowed
 * from the process-lifetime cache. Single-threaded, explicit lifetime. */
typedef struct HGFieldResources HGFieldResources;
typedef struct {
    unsigned map_header, land_member, matrix_x, matrix_z, altitude;
    uint16_t attribute;
    HGLandChunk land;
    HGCollision collision;
} HGFieldTile;
HGFieldResources *hg_field_resources_new(unsigned map);
void hg_field_resources_free(HGFieldResources *);
int hg_field_resources_tile(HGFieldResources *, int x, int z, HGFieldTile *);
/* Number of successfully decoded distinct matrix cells, for diagnostics. */
size_t hg_field_resources_loaded(const HGFieldResources *);

/* Execute original Thumb BDHC height selection in isolated guest RAM.
 * x/z/reference/output are signed Q12 in chunk-local model coordinates.
 * mode 0=nearest reference, 1=highest, 2=lowest. Returns 1 hit, 0 miss,
 * -1 invalid input/unsupported guest execution/arithmetic failure. Output is
 * unchanged on miss/error. Native math boundaries are not SDK execution. */
int hg_collision_height(const uint8_t *,size_t,unsigned mode,int32_t reference,
                        int32_t x,int32_t z,int32_t *height);
/* Static BDHC height at a matrix-global tile center. Height remains in the
 * chunk collision coordinate system; dynamic terrain/objects are not queried. */
int hg_field_resources_height(HGFieldResources *,int x,int z,unsigned mode,
                              int32_t reference,int32_t *height);
/* Same static provider at arbitrary nonnegative matrix-global Q12 position. */
int hg_field_resources_height_at(HGFieldResources *,int32_t x,int32_t z,unsigned mode,
                                 int32_t reference,int32_t *height);
