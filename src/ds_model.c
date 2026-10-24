#include "ds_model.h"
#include "ds_transform.h"
#include <string.h>
#include <math.h>
static unsigned u16(const uint8_t *p) { return p[0]|(unsigned)p[1]<<8; }
static uint32_t u32(const uint8_t *p) { return u16(p)|(uint32_t)u16(p+2)<<16; }
static int span(size_t n,size_t off,size_t len) { return off<=n && len<=n-off; }
/* Dictionary search trees are not executed; indexed entries and names are bounded. */
static int entry(const uint8_t *p,size_t n,unsigned i,uint32_t *off,char *name) {
    if(n<8 || i>=p[1])return 0;
    size_t size=u16(p+2),table=u16(p+6),count=p[1];
    if(size>n || !span(size,table,4) || u16(p+table)!=4)return 0;
    size_t names=table+u16(p+table+2);
    if(names<table+4+4*count || !span(size,table+4,4*count) || !span(size,names,16*count))return 0;
    *off=u32(p+table+4+4*i);
    if(name) { memcpy(name,p+names+16*i,16);name[16]=0; }
    return 1;
}
int ds_model_open(const uint8_t *p,size_t n,unsigned index,DSModel *out) {
    if(!p || !out || n<20 || memcmp(p,"BMD0",4) || u16(p+4)!=0xfeff ||
       u16(p+6)!=2 || u32(p+8)!=n || u16(p+12)!=16 || !u16(p+14) || u16(p+14)>2)return 0;
    unsigned blocks=u16(p+14);if(n<16+4*blocks)return 0;
    size_t block=u32(p+16);if(block<16+4*blocks || !span(n,block,8))return 0;
    size_t len=u32(p+block+4);if(len<8 || !span(n,block,len) || memcmp(p+block,"MDL0",4))return 0;
    if(blocks==1) { if(block+len!=n)return 0; }
    else { size_t tex=u32(p+20);if(tex!=block+len || !span(n,tex,8) || memcmp(p+tex,"TEX0",4) || u32(p+tex+4)!=n-tex)return 0; }
    const uint8_t *mdl=p+block;uint32_t off;char name[17];
    if(!entry(mdl+8,len-8,index,&off,name) || off<8+u16(mdl+10) || !span(len,off,64))return 0;
    const uint8_t *m=mdl+off;size_t size=u32(m);
    if(!span(len,off,size) || size<64)return 0;
    size_t sbc=u32(m+4),mat=u32(m+8),shp=u32(m+12),end=u32(m+16);
    if(sbc<64 || mat<=sbc || shp<=mat || end<shp || end>size || !span(size,shp,8))return 0;
    DSModel v={m,size,shp,m[23],m[24],m[25],u16(m+36),u16(m+40),u16(m+42),{0}};
    memcpy(v.name,name,17);
    if(m[shp+1]!=v.shapes)return 0;
    for(unsigned i=0;i<v.shapes;i++) { const uint8_t *unused;size_t bytes;if(!ds_model_shape(&v,i,&unused,&bytes))return 0; }
    *out=v;return 1;
}
int ds_model_shape(const DSModel *m,unsigned i,const uint8_t **out,size_t *length) {
    if(!m || !out || !length || i>=m->shapes || !span(m->size,m->shapes_offset,8))return 0;
    const uint8_t *p=m->bytes+m->shapes_offset;size_t n=m->size-m->shapes_offset;uint32_t off;
    if(!entry(p,n,i,&off,NULL) || off<u16(p+2) || !span(n,off,16) || u16(p+off+2)!=16)return 0;
    size_t data=u32(p+off+8),size=u32(p+off+12);
    if(data<16 || !span(n-off,data,size) || size%4)return 0;
    *out=p+off+data;*length=size;return 1;
}
static int32_t signed_bits(uint32_t v,unsigned bits) {
    uint32_t sign=1u<<(bits-1),mask=(1u<<bits)-1;v&=mask;
    return (int32_t)(v&~sign)-(int32_t)(v&sign);
}
static int geometry(const uint8_t *p,size_t n,DSModelTriangle emit,void *user,DSModelStats *out,int raw) {
    if(!p || !out || n%4)return 0;
    DSModelStats stats={0};DSModelVertex current={0},ring[4];unsigned mode=0;size_t count=0;int active=0;int32_t tex_u=0,tex_v=0;uint32_t color=0x7fff;
    size_t pos=0;
    while(pos<n) {
        uint32_t ops=u32(p+pos);pos+=4;
        for(unsigned k=0;k<4;k++) {
            unsigned op=(ops>>(8*k))&255,words;
            switch(op) {
            case 0:case 0x41:words=0;break;
            case 0x23:words=2;break;
            case 0x1b:words=3;break;
            case 0x14:case 0x20:case 0x21:case 0x22:case 0x24:case 0x25:case 0x26:case 0x27:case 0x28:case 0x40:words=1;break;
            default:return 0;
            }
            if(!raw && (op==0x14 || op==0x1b))return 0;
            if(!span(n,pos,4*words))return 0;
            uint32_t a=words?u32(p+pos):0,b=words>1?u32(p+pos+4):0;pos+=4*words;stats.commands++;
            if(op==0x40) { if(active || a>3)return 0;active=1;mode=a;count=0; }
            if(op==0x41) { if(!active)return 0;active=0; }
            if(op==0x22) { tex_u=signed_bits(a,16);tex_v=signed_bits(a>>16,16); }
            if(op==0x20)color=a&0x7fff;
            if(op<0x23 || op>0x28)continue;
            if(!active)return 0;
            switch(op) {
            case 0x23:current=(DSModelVertex){signed_bits(a,16),signed_bits(a>>16,16),signed_bits(b,16)};break;
            case 0x24:current=(DSModelVertex){signed_bits(a,10)*64,signed_bits(a>>10,10)*64,signed_bits(a>>20,10)*64};break;
            case 0x25:current.x=signed_bits(a,16);current.y=signed_bits(a>>16,16);break;
            case 0x26:current.x=signed_bits(a,16);current.z=signed_bits(a>>16,16);break;
            case 0x27:current.y=signed_bits(a,16);current.z=signed_bits(a>>16,16);break;
            case 0x28: {
                current.x=signed_bits((uint32_t)current.x+(uint32_t)signed_bits(a,10),16);
                current.y=signed_bits((uint32_t)current.y+(uint32_t)signed_bits(a>>10,10),16);
                current.z=signed_bits((uint32_t)current.z+(uint32_t)signed_bits(a>>20,10),16);break;
            }
            }
            current.u=tex_u;current.v=tex_v;current.color=color;
            ring[count%4]=current;count++;stats.vertices++;
            unsigned indices[6],triangles=0;
            if(mode==0 && count%3==0) { indices[0]=(unsigned)((count-3)%4);indices[1]=(unsigned)((count-2)%4);indices[2]=(unsigned)((count-1)%4);triangles=1; }
            if(mode==1 && count%4==0) { unsigned v[6]={0,1,2,0,2,3};memcpy(indices,v,sizeof(v));triangles=2; }
            if(mode==2 && count>=3) { indices[0]=(unsigned)((count-3)%4);indices[1]=(unsigned)((count-2)%4);indices[2]=(unsigned)((count-1)%4);if(count%2==0) { unsigned t=indices[0];indices[0]=indices[1];indices[1]=t; }triangles=1; }
            if(mode==3 && count>=4 && count%2==0) { unsigned v[6]={(unsigned)((count-4)%4),(unsigned)((count-3)%4),(unsigned)((count-1)%4),(unsigned)((count-4)%4),(unsigned)((count-1)%4),(unsigned)((count-2)%4)};memcpy(indices,v,sizeof(v));triangles=2; }
            for(unsigned t=0;t<triangles;t++) { DSModelVertex tri[3]={ring[indices[3*t]],ring[indices[3*t+1]],ring[indices[3*t+2]]};if(emit)emit(user,tri);stats.triangles++; }
        }
    }
    if(active)return 0;*out=stats;return 1;
}

int ds_model_geometry(const uint8_t *p,size_t n,DSModelTriangle emit,void *user,DSModelStats *out) {
    return geometry(p,n,emit,user,out,1);
}
typedef DSTransform LandTransform;
typedef struct { LandTransform transform;DSSceneTriangle emit;void *user;int valid;unsigned material; } SceneEmit;
static void scene_triangle(void *user,const DSModelVertex tri[3]) {
    SceneEmit *e=user;DSSceneVertex result[3];
    for(unsigned i=0;i<3;i++) {
        result[i]=(DSSceneVertex){0};ds_transform_point(&e->transform,tri[i].x/4096.0,tri[i].y/4096.0,tri[i].z/4096.0,&result[i].x,&result[i].y,&result[i].z);
        result[i].u=tri[i].u/16.0;result[i].v=tri[i].v/16.0;result[i].color=tri[i].color;result[i].material=e->material;
        if(!isfinite(result[i].x) || !isfinite(result[i].y) || !isfinite(result[i].z)) { e->valid=0;return; }
    }
    if(e->emit)e->emit(e->user,result);
}
static int32_t s32(const uint8_t *p) { uint32_t v=u32(p);return (int32_t)((int64_t)v-((v&0x80000000u)?INT64_C(4294967296):0)); }
static int node_transform(const uint8_t *dict,size_t n,unsigned index,DSTransform *out) {
    uint32_t off;if(!entry(dict,n,index,&off,NULL) || off<u16(dict+2) || !span(n,off,4))return 0;
    size_t limit=n;for(unsigned i=0;i<dict[1];i++) { uint32_t next;if(!entry(dict,n,i,&next,NULL))return 0;if(next>off && next<limit)limit=next; }
    const uint8_t *p=dict+off;size_t at=4;unsigned flags=u16(p);if((flags&10)==10)return 0;DSTransform t=ds_transform_identity();
    if(!(flags&1)) { if(!span(limit-off,at,12))return 0;t.m[3]=s32(p+at)/4096.0;t.m[7]=s32(p+at+4)/4096.0;t.m[11]=s32(p+at+8)/4096.0;at+=12; }
    if(flags&8) {
        if(!span(limit-off,at,4))return 0;unsigned form=(flags>>4)&15;if(form>8)return 0;
        double a=signed_bits(u16(p+at),16)/4096.0,b=signed_bits(u16(p+at+2),16)/4096.0;
        double c=(flags&512)?-b:b,d=(flags&1024)?-a:a;
        unsigned row=form%3,col=form/3;for(unsigned r=0;r<3;r++)for(unsigned k=0;k<3;k++)t.m[4*r+k]=0;
        t.m[4*row+col]=(flags&256)?-1:1;unsigned rows[2],cols[2],ri=0,ci=0;
        for(unsigned k=0;k<3;k++) { if(k!=row)rows[ri++]=k;if(k!=col)cols[ci++]=k; }
        t.m[4*rows[0]+cols[0]]=a;t.m[4*rows[0]+cols[1]]=c;t.m[4*rows[1]+cols[0]]=b;t.m[4*rows[1]+cols[1]]=d;at+=4;
    } else if(!(flags&2)) {
        if(!span(limit-off,at,16))return 0;double v[9];v[0]=signed_bits(u16(p+2),16)/4096.0;
        for(unsigned i=1;i<9;i++)v[i]=signed_bits(u16(p+at+2*(i-1)),16)/4096.0;
        for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++)t.m[row*4+col]=v[col*3+row];at+=16;
    }
    if(!(flags&4)) {
        if(!span(limit-off,at,24))return 0;double scale[3];int nonunit=0;
        for(unsigned i=0;i<3;i++){
            scale[i]=s32(p+at+4*i)/4096.0;double inverse=s32(p+at+12+4*i)/4096.0;
            if(!scale[i]||!inverse||fabs(scale[i]*inverse-1)>1.0/128)return 0;
            nonunit|=scale[i]!=1;
        }
        /* Root-only scale has no parent compensation. Keep articulated model
         * scale explicit until its inverse-scale propagation is implemented. */
        if(nonunit&&dict[1]!=1)return 0;
        ds_transform_scale(&t,scale[0],scale[1],scale[2]);
    }
    *out=t;return 1;
}
int ds_model_node(const DSModel *model,unsigned node,DSTransform *out) {
    if(!model || !model->bytes || model->size<72 || !out || node>=model->nodes)return 0;
    size_t end=u32(model->bytes+4);if(end<72 || end>model->size || model->bytes[65]!=model->nodes)return 0;
    return node_transform(model->bytes+64,end-64,node,out);
}
static int scene(const DSModel *model,int selected_shape,int selected_material,DSSceneTriangle emit,void *user,DSModelStats *out,int inspection,const DSJointPose *pose) {
    if(!model || !model->bytes || !out || model->size<72 || !model->nodes || model->nodes>32)return 0;
    const uint8_t *p=model->bytes;size_t sbc=u32(p+4),end=u32(p+8);
    if(p[65]!=model->nodes || sbc<72 || end<=sbc || end>model->size)return 0;
    LandTransform nodes[32],stack[32];unsigned valid[32]={0};
    for(unsigned i=0;i<model->nodes;i++)if(!node_transform(p+64,sbc-64,i,nodes+i))return 0;
    if(pose&&pose->scale_mask&&model->nodes!=1)return 0;
    if(pose)for(unsigned i=0;i<32;i++)if(pose->mask&(1u<<i)){if(i>=model->nodes)return 0;nodes[i]=pose->nodes[i];}
    SceneEmit state={ds_transform_identity(),emit,user,1,0};
    DSModelStats total={0};int visible=1;size_t pos=sbc;
    while(pos<end) {
        unsigned op=p[pos++],kind=op&31,params;
        switch(op) {
        case 0:case 1:case 11:case 43:params=0;break;
        case 7:if(!inspection)return 0;params=1;break;
        case 2:params=2;break;
        case 3:case 4:case 36:case 68:case 5:params=1;break;
        case 6:case 38:case 70:case 102:params=3+!!(op&32)+!!(op&64);break;
        default:return 0;
        }
        if(!span(end,pos,params))return 0;const uint8_t *a=p+pos;pos+=params;total.commands++;
        if(op==7 && a[0]>=model->nodes)return 0;
        if(op==1) { if(!state.valid)return 0;*out=total;return 1; }
        if(op==2) { if(a[0]>=model->nodes || a[1]>1)return 0;visible=a[1]; }
        if(op==3) { if(a[0]>=32 || !valid[a[0]])return 0;state.transform=stack[a[0]]; }
        if(kind==4) { if(a[0]>=model->materials)return 0;state.material=a[0]; }
        if(kind==6) {
            if(a[0]>=model->nodes || a[1]>=model->nodes || a[2]!=0)return 0;
            unsigned destination=op&32?a[3]:32,source=op&64?a[3+!!(op&32)]:32;
            if(op&64) { if(source>=32 || !valid[source])return 0;state.transform=stack[source]; }
            state.transform=ds_transform_multiply(state.transform,nodes[a[0]]);
            if(op&32) { if(destination>=32)return 0;stack[destination]=state.transform;valid[destination]=1; }
        }
        if(op==11 || op==43) {
            double factor=s32(p+(op==11?28:32))/4096.0;
            if(factor<=0)return 0;ds_transform_scale(&state.transform,factor,factor,factor);
            for(unsigned k=0;k<12;k++)if(!isfinite(state.transform.m[k]))return 0;
        }
        if(op==5) {
            const uint8_t *dl;size_t n;DSModelStats stats;
            if(!ds_model_shape(model,a[0],&dl,&n))return 0;
            int draw=visible && (selected_shape<0 || a[0]==selected_shape);if(selected_material>=0)state.material=(unsigned)selected_material;
            if(!geometry(dl,n,draw?scene_triangle:NULL,&state,&stats,0))return 0;
            if(draw) { total.vertices+=stats.vertices;total.triangles+=stats.triangles; }
            total.commands+=stats.commands;
        }
    }
    return 0;
}

int ds_model_scene(const DSModel *model,DSSceneTriangle emit,void *user,DSModelStats *out) { return scene(model,-1,-1,emit,user,out,0,NULL); }
int ds_model_scene_shape(const DSModel *model,unsigned shape,unsigned material,DSSceneTriangle emit,void *user,DSModelStats *out) {
    if(!model || shape>=model->shapes || material>=model->materials)return 0;return scene(model,(int)shape,(int)material,emit,user,out,0,NULL);
}

/* Texture/palette bindings index material lists by names, not archive order. */
static int binding(const uint8_t *mat,size_t n,size_t dict,unsigned material,char name[17]) {
    if(!span(n,dict,8))return 0;const uint8_t *p=mat+dict;size_t size=u16(p+2),table=u16(p+6),count=p[1];
    if(size>n-dict || !span(size,table,4) || u16(p+table)!=4)return 0;
    size_t names=table+u16(p+table+2);
    if(names<table+4+4*count || !span(size,table+4,4*count) || !span(size,names,16*count))return 0;
    int found=0;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *e=p+table+4+4*i;size_t start=u16(e);unsigned len=e[2];
        if(!span(n,start,len))return 0;
        for(unsigned j=0;j<len;j++)if(mat[start+j]==material) {
            if(found)return 0;memcpy(name,p+names+16*i,16);name[16]=0;found=1;
        }
    }
    return 1;
}
int ds_model_material(const DSModel *model,unsigned index,DSModelMaterial *out) {
    if(!model || !out || !model->bytes || model->size<64 || index>=model->materials)return 0;
    size_t start=u32(model->bytes+8),end=u32(model->bytes+12);
    if(end>model->size || start>=end || end-start<12)return 0;
    const uint8_t *p=model->bytes+start;size_t n=end-start;uint32_t off;
    if(!entry(p+4,n-4,index,&off,NULL) || off<4+u16(p+6) || !span(n,off,44) || u16(p+off+2)<44 || !span(n,off,u16(p+off+2)))return 0;
    DSModelMaterial v={0};if(!entry(p+4,n-4,index,&off,v.name))return 0;v.diffuse=u32(p+off+4);v.polygon_attrs=u32(p+off+12);v.texture_params=u32(p+off+20);v.width=u16(p+off+32);v.height=u16(p+off+34);v.identity_uv=u16(p+off+2)==44 && u32(p+off+36)==4096 && u32(p+off+40)==4096;
    unsigned uv_flags=u16(p+off+30);
    if(model->bytes[22]==0 && u16(p+off+2)==52 && (uv_flags&7)==7 && !(uv_flags&8) && u32(p+off+36)==4096 && u32(p+off+40)==4096) {
        v.translation_uv=1;v.uv_u=-s32(p+off+44)/4096.0*v.width;v.uv_v=s32(p+off+48)/4096.0*v.height;
    }
    if(!binding(p,n,u16(p),index,v.texture) || !binding(p,n,u16(p+2),index,v.palette))return 0;
    *out=v;return 1;
}

int ds_model_modify_material_flags(const DSModel *model,uint8_t *writable,uint16_t mask,int enabled){
    if(!model||model->bytes!=writable||model->size<64)return 0;
    size_t start=u32(writable+8),end=u32(writable+12);
    if(end>model->size||start>=end||end-start<12)return 0;
    uint8_t *material=writable+start;size_t size=end-start;
    /* Validate the full set before publishing any edits. */
    for(unsigned i=0;i<model->materials;i++){
        uint32_t offset;
        if(!entry(material+4,size-4,i,&offset,NULL)||offset<4+u16(material+6)||!span(size,offset,44)||u16(material+offset+2)<44||!span(size,offset,u16(material+offset+2)))return 0;
    }
    for(unsigned i=0;i<model->materials;i++){
        uint32_t offset;entry(material+4,size-4,i,&offset,NULL);
        unsigned flags=u16(material+offset+30);flags=enabled?(flags|mask):(flags&~mask);
        material[offset+30]=(uint8_t)flags;material[offset+31]=(uint8_t)(flags>>8);
    }return 1;
}

int ds_model_inspection_scene(const DSModel *m,DSSceneTriangle emit,void *user,DSModelStats *out) { return scene(m,-1,-1,emit,user,out,1,NULL); }

int ds_model_scene_pose(const DSModel *model,int shape,int material,const DSJointPose *pose,DSSceneTriangle emit,void *user,DSModelStats *out) { return scene(model,shape,material,emit,user,out,0,pose); }
