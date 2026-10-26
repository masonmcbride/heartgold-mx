"""Original whole loader and buffer-owner units; unresolved imports remain traps."""
import re
from pathlib import Path
import sys,json,hashlib
from guest_assembly import assemble
from generated_files import write_text_if_changed
root,out,cc=map(Path,sys.argv[1:4])
paths=[root/'asm'/s for s in ('overlay_01_021EAF00.s','overlay_01_021F6830.s','overlay_01_021F4704.s','overlay_01_021FAC44.s','overlay_01_021FB04C.s','overlay_01_021FB878.s','overlay_01_021EA6C4.s','unk_02054648.s','overlay_01_021FAD1C.s','overlay_01_021FB368.s','unk_02020B8C.s','unk_02023694.s','unk_0200B150.s','overlay_01_021F1348.s','unk_0202068C.s','unk_02026DE0.s','unk_02069660.s','unk_0206979C.s','unk_020689C8.s','overlay_01_021FC4C4.s','overlay_01_data_02208BFC.s','overlay_01_021FD1B8.s','overlay_01_02203E40.s','overlay_01_021F8D80.s','overlay_01_021F944C.s','overlay_01_022053EC.s','overlay_01_021E90C0.s','model_attributes.s','overlay_01_021EABA8.s','overlay_01_021EB1E8.s','unk_02025C44.s','overlay_01_021EAFD4.s','unk_02004A44.s','unk_02005D10.s','overlay_01_021FB4C0.s','overlay_01_021FB5D4.s','unk_02014A08.s')]
# Original default renderer descriptor constructors identify coherent owner units.
# Include their complete code and referenced data tables in one batch; callbacks
# outside this resource island remain named, rejecting imports.
effect_table=(root/'asm/overlay_01_data_02208BFC.s').read_text()
for name in re.findall(r'^\s*\.word 0x[0-9A-Fa-f]+, (ov01_\w+),',effect_table,re.M):
    path=root/'asm'/('overlay_01_'+name.removeprefix('ov01_')+'.s')
    if not path.exists():
        if (root/'src/field'/('overlay_01_'+name.removeprefix('ov01_')+'.c')).exists():continue # native pointer-bearing owner remains a named import
        raise ValueError(('missing effect constructor unit',name))
    if path not in paths:paths.append(path)
# Resolve the complete original graphics-descriptor family together. Native
# projections retain all five callback slots; ARM work/implementation stays here.
sprite_source=(root/'asm/overlay_01_sprite_data.s').read_text()
graphics_table=re.search(r'^ov01_02209A38:\n((?:\t.word \w+\n){20})',sprite_source,re.M)
if not graphics_table:raise ValueError('graphics callback table drift')
graphics_names=re.findall(r'\t.word (\w+)',graphics_table[1])+['ov01_0220724C']
graphics_rows={}
for name in graphics_names:
    block=re.search(r'^'+name+r':[^\n]*\n((?:\t.word \w+\n){5})',sprite_source,re.M)
    if not block:raise ValueError(('graphics descriptor drift',name))
    graphics_rows[name]=re.findall(r'\t.word (\w+)',block[1])
callback_targets={n for row in graphics_rows.values() for n in row}
assembly_definitions={}
for path in sorted((root/'asm').glob('overlay_01_*.s')):
    for name in re.findall(r'\bthumb_func_start (\w+)',path.read_text()):
        if name in callback_targets:
            if name in assembly_definitions:raise ValueError(('ambiguous graphics callback',name))
            assembly_definitions[name]=path
for path in sorted(set(assembly_definitions.values())):
    if path not in paths:paths.append(path)
if root/'asm/unk_0205FD20.s' not in paths:paths.append(root/'asm/unk_0205FD20.s')
raw='\n'.join(p.read_text() for p in paths)
data_units={p:p.read_text() for p in (root/'asm').glob('overlay_01*data*.s')}
while True:
    names=set(re.findall(r'\b(?:ov01_\w+|defaultFieldEffectRenderers)\b',raw));added=[]
    for path,text in data_units.items():
        if path not in paths and names.intersection(re.findall(r'^(\w+):',text,re.M)):
            paths.append(path);added.append(text)
    if not added:break
    raw+='\n'+'\n'.join(added)
loader_dependencies='set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS\n'+''.join('  "'+str(p.resolve())+'"\n' for p in paths+[root/'asm/unk_02056D7C.s',root/'src/field/overlay_01_021FFECC.c',root/'include/overlay_01_021F1348.h',root/'include/unk_020689C8.h',root/'src/overlay_01_021EA824.c',root/'include/overlay_01_021EA824.h',root/'src/field/fieldmap.c',root/'src/camera.c',root/'include/camera.h',root/'src/ascii_util.c',root/'src/gf_rtc.c'])+')\n'

aux_path=root/'asm/overlay_02_02248728.s';aux_text=aux_path.read_text();aux_records={}
for name in ('ov02_0224F864','ov02_0224F8F4'):
    matches=list(re.finditer(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,aux_text,re.S))
    if len(matches)!=1:raise ValueError(('auxiliary owner drift',name))
    body=matches[0][0];raw+='\n.text\n'+body+'\n';aux_records[name]=hashlib.sha256(body.encode()).hexdigest()

comm_source=root/'asm/unk_02056D7C.s'
comm_text=comm_source.read_text()
comm_matches=list(re.finditer(
    r'\tthumb_func_start CommPlayerManager_ForcePosition\n.*?'
    r'\tthumb_func_end CommPlayerManager_ForcePosition',
    comm_text,re.S))
if len(comm_matches)!=1:
    raise ValueError('communication function drift')
comm_body=comm_matches[0][0]

# Original global communication-manager pointer, initially NULL.
raw+='\n.bss\n.balign 4\n_021D41C4:\n.space 4\n.text\n'+comm_body+'\n'
aux_records['CommPlayerManager_ForcePosition']=hashlib.sha256(comm_body.encode()).hexdigest()

# Communication mode / overlay status queries are original software, not sound
# services. Keep their real globals and conditional overlay read in guest memory.
for filename, names in {
    'unk_02035900.s': ('sub_020378CC',),
    'unk_02033AE0.s': ('sub_02034044',),
    'overlay_00_thumb.s': ('ov00_021E7080',),
}.items():
    path=root/'asm'/filename
    text=path.read_text()
    loader_dependencies+='set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "'+str(path.resolve())+'")\n'
    for name in names:
        matches=list(re.finditer(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,text,re.S))
        if len(matches)!=1:raise ValueError(('communication query drift',name))
        body=matches[0][0];raw+='\n.text\n'+body+'\n'
        aux_records[name]=hashlib.sha256(body.encode()).hexdigest()
# Preserve the original overlay's zero-initialized four-byte pointer owner.
if not re.search(r'^_0221A680:\n\s*\.space 0x4\b', (root/'asm/overlay_00_thumb.s').read_text(),re.M):
    raise ValueError('communication overlay owner drift')
raw+='\n.bss\n.balign 4\n_0221A680:\n.space 4\n.text\n'


# Renderer 14 is original C with embedded ARM records. Compile its unchanged
# bodies for the existing guest island, avoiding widened host pointers entirely.
# Its SDK render record is opaque storage here; only the established SDK bridge
# consumes fields, and compile-time checks pin the original ARM ABI.
import subprocess
renderer_path=root/'src/field/overlay_01_021FFECC.c'
renderer_text=renderer_path.read_text()
def anonymous_type(text,name):
    matches=re.findall(r'typedef struct \{(?:(?!typedef struct).)*?\} '+re.escape(name)+r';',text,re.S)
    if len(matches)!=1:raise ValueError(('renderer declaration drift',name,len(matches)))
    return matches[0]
renderer_header=(root/'include/overlay_01_021F1348.h').read_text()
common_header=(root/'include/unk_020689C8.h').read_text()
renderer_types="""
typedef unsigned char u8; typedef unsigned int u32; typedef int BOOL;
#define TRUE 1
typedef struct LocalMapObject LocalMapObject;
typedef struct FieldSystem FieldSystem;
typedef struct TaskManager TaskManager;
typedef struct NNSG3dResMdl NNSG3dResMdl;
typedef struct {u32 opaque[21];} NNSG3dRenderObj;
typedef struct {int x,y,z;} VecFx32;
"""
renderer_types+='\n'.join(anonymous_type(common_header,n) for n in ('UnkOv01_021FFFCC_common','UnkOv01_021FFFCC'))
renderer_types+='\n'+'\n'.join(anonymous_type(renderer_header,n) for n in ('UnkOv01_021FFECC_sub','UnkOv01_021FFECC','UnkOv01_021FFFCD'))
renderer_types+="""
typedef BOOL (*UnkOv01_02209280_Cb1)(void *,UnkOv01_021FFFCD *);
typedef void (*UnkOv01_02209280_Cb2)(void *,UnkOv01_021FFFCD *);
"""
renderer_types+='\n'.join(anonymous_type(renderer_header,n) for n in ('UnkOv01_02209280','UnkOv01_021FFF5C'))
renderer_types+="""
_Static_assert(sizeof(void *)==4 && sizeof(UnkOv01_021FFECC_sub)==20,"ARM model holder");
_Static_assert(__builtin_offsetof(UnkOv01_021FFECC,unk54)==0x54 && sizeof(UnkOv01_021FFECC)==0x1a4,"ARM embedded render objects");
UnkOv01_021FFECC *ov01_021F1430(void *,int,int,int);
void ov01_021F1448(void *);
void ov01_021F19F4(void *,UnkOv01_021FFECC_sub *,int,int,int);
void sub_02069978(NNSG3dRenderObj *,UnkOv01_021FFECC_sub *);
void sub_02069784(UnkOv01_021FFECC_sub *);
FieldSystem *ov01_021F146C(LocalMapObject *);
TaskManager *ov01_021F1468(FieldSystem *);
int MapObject_GetXCoord(LocalMapObject *);int MapObject_GetZCoord(LocalMapObject *);
void sub_020611C8(int,int,VecFx32 *);void sub_0206121C(TaskManager *,VecFx32 *);
UnkOv01_021FFECC *ov01_021F1450(FieldSystem *,int);
int MapObject_GetPriorityPlusValue(LocalMapObject *,int);
void ov01_021F1620(FieldSystem *,const UnkOv01_02209280 *,VecFx32 *,int,UnkOv01_021FFF5C *,int);
UnkOv01_021FFFCC *sub_02068D98(void *);
u32 MapObject_GetID(LocalMapObject *);u32 MapObject_GetMapID(LocalMapObject *);
BOOL sub_0205F0A8(LocalMapObject *,u32,u32);BOOL sub_02066420(LocalMapObject *,int);
void sub_02068DB8(void *,VecFx32 *);void sub_020699BC(int,VecFx32 *);
"""
renderer_c=out/'original_renderer14.c';renderer_s=out/'original_renderer14.s'
write_text_if_changed(renderer_c,renderer_types+'\n'+re.sub(r'^#include[^\n]*','',renderer_text,flags=re.M))
subprocess.run([str(cc),'-target','arm-none-eabi','-mcpu=arm946e-s','-mthumb','-O1','-ffreestanding','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-S',str(renderer_c),'-o',str(renderer_s)],check=True)
raw+='\n.set hg_compiler_syntax, 1\n'+renderer_s.read_text()+'\n.set hg_compiler_syntax, 0\n'
write_text_if_changed(out/'original_renderer14_provenance.json',json.dumps(dict(source=str(renderer_path),source_sha256=hashlib.sha256(renderer_path.read_bytes()).hexdigest(),types_sha256=hashlib.sha256(renderer_types.encode()).hexdigest(),compiler_target='arm-none-eabi arm946e-s Thumb',adaptation='original declarations on ARM32; SDK render record opaque 84-byte storage, fields consumed by existing typed bridge'),indent=2)+'\n')

# Pointer-free environment records retain their original ARM32 allocation and
# pointer-to-pointer destruction, including the original field configuration.
from prepare_original_field import declaration, functions
environment_code, environment_records = functions(root, {
    'src/field/fieldmap.c': ('FieldMap_InitModelAttributes',),
    'src/unk_02037C94.c': ('sub_0203993C',),
    'src/overlay_01_021EA824.c': ('Fog_New', 'Fog_Free', 'ov01_021EA854',
        'ov01_021EA858', 'ov01_021EA85C', 'ov01_021EA860', 'ov01_021EA864'),
})
environment_types = """
typedef unsigned int u32; typedef unsigned short u16; typedef int BOOL; typedef int s32;
typedef int GXFogBlend, GXFogSlope;
typedef struct ModelAttributes ModelAttributes;
#define FALSE 0
#define TRUE 1
#define HEAP_ID_FIELD1 4
#define NULL ((void *)0)
#define GX_POLYGONMODE_MODULATE 0
#define GX_CULL_BACK 2
void *Heap_Alloc(int, u32); void Heap_FreeExplicit(int, void *);
void MI_CpuClear32(void *, u32);
void ModelAttributes_SetPolygonMode(ModelAttributes *, int, BOOL);
void ModelAttributes_SetCullMode(ModelAttributes *, int, BOOL);
void ModelAttributes_SetAlpha(ModelAttributes *, int, BOOL);
void ModelAttributes_SetMiscAttrEnabled(ModelAttributes *, int, BOOL, BOOL);
void ModelAttributes_ApplyGlobal(ModelAttributes *, int);
void G3X_SetFog(int,int,int,int);
static void FieldMap_InitModelAttributes(ModelAttributes *) __attribute__((used));
"""
environment_types += declaration((root/'include/overlay_01_021EA824.h').read_text(), 'FogData')
environment_types += '\n_Static_assert(sizeof(FogData)==0x38, "ARM fog record");\n'
# Original communication owner declaration, compiled for ARM32. Only its mode
# getter is admitted; construction/destruction remain named boundaries.
environment_types += "\ntypedef unsigned char u8; typedef signed char s8; typedef unsigned long long u64;\n"
environment_types += "typedef struct UnkStruct_ov44_0223197C UnkStruct_ov44_0223197C; typedef struct SaveData SaveData; typedef struct PlayerProfile PlayerProfile;\n"
sdk=Path(sys.argv[4]) if len(sys.argv)>4 else root.parent.parent/'build/evidence/weather/NitroSDK'
# The original SDK ABI aligns 64-bit struct members to four bytes; AAPCS
# defaults to eight. Preserve the documented original offsets explicitly.
environment_types += anonymous_type((sdk/'include/nitro/math/rand.h').read_text(),'MATHRandContext32').replace('} MATHRandContext32;', '} __attribute__((packed, aligned(4))) MATHRandContext32;')
environment_types += declaration((root/'src/unk_02037C94.c').read_text(),'UnkStruct_sub_02039C6C')
if not re.search(r'^UnkStruct_sub_02039C6C \*_021D4150 = NULL;', (root/'src/unk_02037C94.c').read_text(),re.M):
    raise ValueError('communication owner initializer drift')
environment_types += "\nUnkStruct_sub_02039C6C *_021D4150 = NULL;\n_Static_assert(__builtin_offsetof(UnkStruct_sub_02039C6C,unk4E)==0x4e, \"ARM communication mode\");\n"
loader_dependencies+='set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "'+str((root/'src/unk_02037C94.c').resolve())+'" "'+str((sdk/'include/nitro/math/rand.h').resolve())+'")\n'
environment_c=out/'original_environment.c'; environment_s=out/'original_environment.s'
write_text_if_changed(environment_c,environment_types+'\n'+environment_code)
subprocess.run([str(cc),'-target','arm-none-eabi','-mcpu=arm946e-s','-mthumb','-O1',
    '-ffreestanding','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-S',
    '-femit-all-decls',str(environment_c),'-o',str(environment_s)],check=True)
raw+='\n.set hg_compiler_syntax, 1\n'+re.sub(r'\.L(\w+)',r'.Lenv_\1',environment_s.read_text())+'\n.set hg_compiler_syntax, 0\n'
write_text_if_changed(out/'original_environment_provenance.json',json.dumps(dict(
    functions=environment_records, compiler_target='arm-none-eabi arm946e-s Thumb',
    boundary='original environment owners and read-only ARM32 communication queries; active communication lifecycle and device services remain distinct imports'),indent=2)+'\n')

# One ARM32 copy of the original camera owner/history/algorithms. No native
# Camera layout is exposed to these instructions.
# Mathematically generated Q12 indexed trig values, never copied SDK material.
# Full equality to the original binary's lookup table remains a recorded audit
# obligation; generation itself is deterministic at 70 decimal digits.
from decimal import Decimal, localcontext, ROUND_HALF_UP
with localcontext() as ctx:
    ctx.prec=70
    def atan_inverse(n):
        x=Decimal(1)/n;term=x;total=x;k=1
        while abs(term)>Decimal('1e-74'):
            term*=-x*x;total+=term/(2*k+1);k+=1
        return total
    pi=16*atan_inverse(5)-4*atan_inverse(239)
    def sine(x):
        term=x;total=x;k=1
        while abs(term)>Decimal('1e-72'):
            term*=-x*x/((2*k)*(2*k+1));total+=term;k+=1
        return int((total*4096).to_integral_value(rounding=ROUND_HALF_UP))
    values=[sine((Decimal(i)/2048-(2 if i>2048 else 0))*pi) for i in range(4096)]
write_text_if_changed(out/'original_camera_math.inc','static const short camera_sine[4096]={'+','.join(map(str,values))+'};\n')
camera_header=(root/'include/camera.h').read_text()
camera_types="""
typedef unsigned char u8; typedef unsigned short u16; typedef int s32,fx32,BOOL;
typedef short fx16; typedef long long fx64;
typedef struct {fx32 x,y,z;} VecFx32;
typedef int GXBufferMode;
#define NULL ((void *)0)
#define TRUE 1
#define FALSE 0
#define GX_BUFFERMODE_W 1
#define GX_BUFFERMODE_Z 0
#define FX32_ONE 4096
#define FX32_CONST(x) ((fx32)((x)*4096+0.5))
#define GF_ASSERT(x) ((x)?(void)0:GF_AssertFail())
void GF_AssertFail(void); void *Heap_Alloc(int,unsigned); void Heap_Free(void *);
fx32 FX_Mul(fx32,fx32); fx32 FX_Div(fx32,fx32);
fx16 FX_SinIdx(u16); fx16 FX_CosIdx(u16);
void VEC_Add(const VecFx32 *,const VecFx32 *,VecFx32 *);
void VEC_Subtract(const VecFx32 *,const VecFx32 *,VecFx32 *);
fx32 VEC_Mag(const VecFx32 *);
void NNS_G3dGlbLookAt(const VecFx32 *,const VecFx32 *,const VecFx32 *);
void NNS_G3dGlbPerspective(fx32,fx32,fx32,fx32,fx32);
void NNS_G3dGlbOrtho(fx32,fx32,fx32,fx32,fx32,fx32);
"""
camera_header=re.sub(r'^#include[^\n]*','',camera_header,flags=re.M).replace('enum HeapID','int')
camera_source=re.sub(r'^#include[^\n]*','',(root/'src/camera.c').read_text(),flags=re.M).replace('enum HeapID','int')
camera_types+='typedef signed char s8; typedef unsigned int u32; typedef struct {u32 opaque[18];} FSFile;\nvoid FS_InitFile(FSFile *); BOOL FS_OpenFile(FSFile *,const char *); u32 FS_GetLength(FSFile *); s32 FS_ReadFile(FSFile *,void *,s32); BOOL FS_CloseFile(FSFile *); void Heap_FreeExplicit(int,void *);\n'
camera_types+=camera_header+'\n_Static_assert(sizeof(Camera)==0x68 && sizeof(CameraHistory)==0x28,"ARM camera ABI");\n'
camera_c=out/'original_camera.c';camera_s=out/'original_camera.s'
light_code,light_records=functions(root,{
    'src/ascii_util.c':('Ascii_StrLen','Ascii_GetDelim','Ascii_StrToL','sub_02020B3C'),
    'src/system.c':('Sys_AllocAndReadFile',),
    'src/gf_rtc.c':('GF_RTC_TimeToSec',),
})
light_types='\ntypedef union {u32 val[4];s8 name[16];} NNSG3dResName;\n#define NNS_G3D_RESNAME_VALSIZE 4\ntypedef struct {u32 hour,minute,second;} RTCTime;\nRTCTime *hg_field_rtc_time(void);\n#define getTime(work) hg_field_rtc_time()\n'
write_text_if_changed(camera_c,camera_types+camera_source+light_types+light_code.replace('enum HeapID','int'))
write_text_if_changed(out/'original_light_transport_provenance.json',json.dumps(dict(functions=light_records,
    boundary='original ASCII/file-allocation/RTC scalar algorithms; opaque ARM FSFile identity only, no native SDK layout cast'),indent=2)+'\n')
subprocess.run([str(cc),'-target','arm-none-eabi','-mcpu=arm946e-s','-mthumb','-O1',
    '-ffreestanding','-fwrapv','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
    '-S',str(camera_c),'-o',str(camera_s)],check=True)
raw+='\n.set hg_compiler_syntax, 1\n'+re.sub(r'\.L(\w+)',r'.Lcamera_\1',camera_s.read_text())+'\n.set hg_compiler_syntax, 0\n'
write_text_if_changed(out/'original_camera_provenance.json',json.dumps(dict(
    functions=[dict(function=n,source='src/camera.c',sha256=hashlib.sha256(body.encode()).hexdigest())
        for n,body in [(m[1],m[0]) for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',(root/'src/camera.c').read_text(),re.M|re.S)]],
    original_sha256=hashlib.sha256((root/'src/camera.c').read_bytes()).hexdigest(),
    adaptation='ARM32 declarations; original camera algorithms unchanged; SDK math/state imports explicit'),indent=2)+'\n')

# Original ARM32 2D ownership and SDK software-state code use upstream-pinned
# source inputs; every unprovided active drawing operation remains an import.
from prepare_original_2d import generate as generate_2d
from prepare_original_2d_sdk import generate as generate_2d_sdk
from prepare_original_sound import generate as generate_sound
sdk_roots=tuple(map(Path,sys.argv[4:6])) or (None,None)
for generator in (generate_2d,generate_2d_sdk,generate_sound):
    code2d,provenance2d,dependencies2d=generator(root,out,cc,*sdk_roots)
    raw+='\n.set hg_compiler_syntax, 1\n'+code2d+'\n.set hg_compiler_syntax, 0\n'
    if generator==generate_sound:write_text_if_changed(out/'original_sound_provenance.json',json.dumps(provenance2d,indent=2)+'\n')
    if generator==generate_2d:write_text_if_changed(out/'original_2d_provenance.json',json.dumps(provenance2d,indent=2)+'\n')
    loader_dependencies+='set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS\n'+''.join('  "'+str(p.resolve())+'"\n' for p in dependencies2d)+')\n'


# HG_ORIGINAL_SCREEN_ENVIRONMENT
# Compile Nintendo's unchanged task/environment functions for the ARM32 ABI.
screen_env_code, screen_env_records = functions(root, {
    'src/systask_environment.c': (
        'CreateSysTaskAndEnvironment',
        'DestroySysTaskAndEnvironment',
    )
})
screen_env_types = """
typedef unsigned int u32;
typedef struct SysTask SysTask;
typedef void (*SysTaskFunc)(SysTask *, void *);
enum HeapID { HG_SCREEN_ENV_ABI_PLACEHOLDER = 0 };
#define NULL ((void *)0)
void *Heap_Alloc(enum HeapID, u32);
void Heap_Free(void *);
void *memset(void *, int, unsigned long);
SysTask *SysTask_CreateOnMainQueue(SysTaskFunc, void *, u32);
void *SysTask_GetData(SysTask *);
void SysTask_Destroy(SysTask *);
"""
screen_env_c = out/'original_screen_environment.c'
screen_env_s = out/'original_screen_environment.s'
write_text_if_changed(screen_env_c, screen_env_types + screen_env_code)
subprocess.run([
    str(cc), '-target', 'arm-none-eabi', '-mcpu=arm946e-s',
    '-mthumb', '-O1', '-ffreestanding', '-fno-unwind-tables',
    '-fno-asynchronous-unwind-tables', '-S',
    str(screen_env_c), '-o', str(screen_env_s)
], check=True)
raw += '\n.set hg_compiler_syntax, 1\n' + re.sub(
    r'\.L(\w+)', r'.Lscreenenv_\1', screen_env_s.read_text()
) + '\n.set hg_compiler_syntax, 0\n'
loader_dependencies += (
    'set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "'
    + str((root/'src/systask_environment.c').resolve()) + '")\n'
)
write_text_if_changed(
    out/'original_screen_environment_provenance.json',
    json.dumps(screen_env_records, indent=2)+'\n'
)

write_text_if_changed(out/'original_loader_dependencies.cmake',loader_dependencies)
image,code,imports=assemble(raw,root,out,cc,'original_loader_guest','HG_LOADER',include_dirs=(out,),data_imports=('NNS_GfdDefaultFuncFreeTexVram','NNS_GfdDefaultFuncFreePlttVram','FX_SinCosTable_','sSoundWork'))
# Trusted original callback PCs become typed native trampolines, never native
# pointers embedded in ARM records. Unimplemented original targets remain traps.
exports={n:int(v) for n,v in re.findall(r'#define HG_LOADER_(\w+) (\d+)u',(out/'original_loader_guest.inc').read_text())}
write_text_if_changed(out/'original_loader_test_exports.inc',''.join('{\"'+n+'\",'+str(v)+'u},\n' for n,v in sorted(exports.items())))
projection=['static void canonical_graphics_callback(LocalMapObject *,u32);']
for target in sorted(callback_targets):
    address=exports.get(target,imports.get(target))
    if address is None:raise ValueError(('unresolved graphics callback symbol',target))
    projection.append(f'static void canonical_graphics_{target}(LocalMapObject *object){{canonical_graphics_callback(object,{address}u);}}')
for name,row in graphics_rows.items():
    projection.append(('' if name=='ov01_0220724C' else 'static ')+f'UnkLMOCallbackStruct2 {name} = '+'{'+','.join('canonical_graphics_'+target for target in row)+'};')
projection.append('UnkLMOCallbackStruct2 *ov01_02209A38[20] = {'+','.join('&'+name for name in graphics_names[:-1])+'};')
write_text_if_changed(out/'original_loader_callbacks.inc','\n'.join(projection)+'\n')
write_text_if_changed(out/'original_loader_callbacks_provenance.json',json.dumps(dict(source='asm/overlay_01_sprite_data.s',source_sha256=hashlib.sha256(sprite_source.encode()).hexdigest(),descriptors=graphics_rows,unimplemented_targets=sorted(callback_targets-assembly_definitions.keys()),boundary='original five-slot descriptor order retained with typed native trampolines into original ARM implementations'),indent=2)+'\n')

write_text_if_changed(out/'original_loader_provenance.json',json.dumps(dict(
    sources={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
    auxiliary_functions=aux_records,sdk_data_imports=['NNS_GfdDefaultFuncFreeTexVram','NNS_GfdDefaultFuncFreePlttVram'],code_bytes=code,image_bytes=len(image),imports=imports,
    boundary='original loader guest island; unprovided imports fail explicitly'),indent=2)+'\n')

from prepare_original_field import declaration,functions
source=(root/'src/field/map_prop_manager.c').read_text()
code,records=functions(root,{'src/field/map_prop_manager.c':('MapPropManager_New','MapPropManager_Init','MapProp_Init','MapPropManager_Free')})
write_text_if_changed(out/'original_loader_props.inc',declaration(source,'MapProp')+'\n'+declaration(source,'MapPropManager')+'\n'+code)
write_text_if_changed(out/'original_loader_props_provenance.json',json.dumps(records,indent=2)+'\n')
# The original file-cache owner is distinct from the immutable host archive cache.
cache_code,cache_records=functions(root,{'src/system.c':('sub_0201A430',)})
cache_header=(root/'include/system.h').read_text()
write_text_if_changed(out/'original_loader_system.inc','static struct FSCacheEntry gFileCache[128];\n'+cache_code)
write_text_if_changed(out/'original_loader_system_provenance.json',json.dumps(cache_records,indent=2)+'\n')

# Original raw-resource manager native graph. Pointer-bearing storage stays native;
# ARM retains only an opaque identity. Traverse all intra-unit outgoing calls.
raw_source=(root/'src/unk_02025534.c').read_text()
raw_defs={m[1]:m[0] for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',raw_source,re.M|re.S)}
raw_names=set();pending=sorted(n for n in raw_defs if n.startswith('GF3dGfxRawRes'))
while pending:
    name=pending.pop()
    if name in raw_names:continue
    raw_names.add(name)
    pending.extend(n for n in re.findall(r'\b(\w+)\s*\(',raw_defs[name]) if n in raw_defs and n not in raw_names)
raw_code,raw_records=functions(root,{'src/unk_02025534.c':tuple(sorted(raw_names))})
raw_header=(root/'include/gf_3d_loader.h').read_text()
raw_types='typedef u32 NNSG3dTexKey,NNSG3dPlttKey;\n'+'\n'.join(declaration(raw_header,n) for n in ('GF_2DGfxRawResObj','GF_2DGfxRawResMan','GF_3DGfxRawResObj','GF_3DGfxRawResMan'))
raw_open=r"""
/* Typed native SDK view, not a serialized TEX0 declaration. The bounded bridge
 * admits unused compressed-info records with zero size/flags/key only. */
typedef struct { struct {u32 size;} header;struct {u16 flag;} tex4x4Info;const u8 *bytes;void *owner;u32 keys[2];unsigned texture;u32 source_guest; } HGRawTex;
#define NNSG3dResTex HGRawTex
#define NNS_G3D_RESPLTT_USEPLTT4 1
static HGRawTex *raw_tex(const void *);
static void *raw_tex_data(HGRawTex *);
static u32 raw_size(HGRawTex *,unsigned);
static u32 raw_alloc_key(u32,BOOL,u32,unsigned);
static int raw_free_key(u32,unsigned);
static void raw_set_keys(HGRawTex *,u32,u32);
static void raw_set_palette(HGRawTex *,u32);
static void raw_release_keys(HGRawTex *,u32 *,u32 *);
static u32 raw_release_palette(HGRawTex *);
static void raw_upload(HGRawTex *,BOOL,unsigned);
static void raw_flush(const void *,u32);
static void raw_heap_free(void *);
#define NNS_G3dGetTex raw_tex
#define NNS_G3dGetTexData raw_tex_data
#define NNS_G3dTexGetRequiredSize(t) raw_size(t,0)
#define NNS_G3dTex4x4GetRequiredSize(t) raw_size(t,2)
#define NNS_G3dPlttGetRequiredSize(t) raw_size(t,1)
#define NNS_GfdAllocTexVram(n,c,o) raw_alloc_key(n,c,o,0)
#define NNS_GfdAllocPlttVram(n,c,o) raw_alloc_key(n,c,o,1)
#define NNS_GfdFreeTexVram(k) raw_free_key(k,0)
#define NNS_GfdFreePlttVram(k) raw_free_key(k,1)
#define NNS_G3dTexSetTexKey raw_set_keys
#define NNS_G3dPlttSetPlttKey raw_set_palette
#define NNS_G3dTexReleaseTexKey raw_release_keys
#define NNS_G3dPlttReleasePlttKey raw_release_palette
#define GF3dRender_LoadTexture(t,b) raw_upload(t,b,0)
#define GF3dRender_LoadPalette(t,b) raw_upload(t,b,1)
#define DC_FlushRange raw_flush
#define Heap_Free raw_heap_free
"""
raw_close='\n'+''.join('#undef '+n+'\n' for n in ('NNSG3dResTex NNS_G3dGetTex NNS_G3dGetTexData NNS_G3dTexGetRequiredSize NNS_G3dTex4x4GetRequiredSize NNS_G3dPlttGetRequiredSize NNS_GfdAllocTexVram NNS_GfdAllocPlttVram NNS_GfdFreeTexVram NNS_GfdFreePlttVram NNS_G3dTexSetTexKey NNS_G3dPlttSetPlttKey NNS_G3dTexReleaseTexKey NNS_G3dPlttReleasePlttKey GF3dRender_LoadTexture GF3dRender_LoadPalette DC_FlushRange Heap_Free').split())
write_text_if_changed(out/'original_loader_raw.inc',raw_types+'\n'+raw_open+raw_code+raw_close)
write_text_if_changed(out/'original_loader_raw_provenance.json',json.dumps(dict(functions=raw_records,boundary='original native resource owner; typed SDK/cache/key/heap transport; unsupported modes reject'),indent=2)+'\n')
