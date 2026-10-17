#include "ds_transform.h"
DSTransform ds_transform_identity(void) { return (DSTransform){{1,0,0,0,0,1,0,0,0,0,1,0}}; }
DSTransform ds_transform_multiply(DSTransform a,DSTransform b) {
    DSTransform c={0};for(unsigned row=0;row<3;row++) {
        for(unsigned col=0;col<3;col++)for(unsigned k=0;k<3;k++)c.m[4*row+col]+=a.m[4*row+k]*b.m[4*k+col];
        c.m[4*row+3]=a.m[4*row+3];for(unsigned k=0;k<3;k++)c.m[4*row+3]+=a.m[4*row+k]*b.m[4*k+3];
    }return c;
}
void ds_transform_point(const DSTransform *t,double x,double y,double z,double *xx,double *yy,double *zz) {
    *xx=t->m[0]*x+t->m[1]*y+t->m[2]*z+t->m[3];*yy=t->m[4]*x+t->m[5]*y+t->m[6]*z+t->m[7];*zz=t->m[8]*x+t->m[9]*y+t->m[10]*z+t->m[11];
}
void ds_transform_scale(DSTransform *t,double x,double y,double z) { for(unsigned i=0;i<3;i++) { t->m[4*i]*=x;t->m[4*i+1]*=y;t->m[4*i+2]*=z; } }
