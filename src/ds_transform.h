#pragma once
/* Host graphics affine matrices: row-major 3x4, column-vector points. */
typedef struct { double m[12]; } DSTransform;
DSTransform ds_transform_identity(void);
DSTransform ds_transform_multiply(DSTransform,DSTransform);
void ds_transform_point(const DSTransform *,double,double,double,double *,double *,double *);
void ds_transform_scale(DSTransform *,double,double,double);
