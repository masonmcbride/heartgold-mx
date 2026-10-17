#!/usr/bin/env python3
"""Generate private original-resource indices and a matrix archive from manifests."""
import json,pathlib,re,struct,sys
root,out=map(pathlib.Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
def listed(relative,variable):
    p=root/relative;text=re.sub(r'#.*','',p.read_text());m=re.search(r'\b'+variable+r'\s*=\s*(?:copy_gen\.process\()?files\((.*?)\)',text,re.S)
    if not m:raise ValueError(variable)
    return [p.parent/name for name in re.findall(r"'([^']+)'",m[1])]
def header(relative,entries):
    p=out/relative;p.parent.mkdir(parents=True,exist_ok=True)
    if len(entries)!=len(set(name for name,value in entries)):raise ValueError('Duplicate indices')
    p.write_text('/* Private indices from original manifests. */\n'+''.join(f'#define {name} {index}\n' for name,index in entries))
matrices=listed('files/fielddata/mapmatrix/map_matrix/meson.build','map_matrix_bin')
for i,p in enumerate(matrices):
    if int(p.stem.split('_')[2])!=i:raise ValueError('Matrix order changed')
header('files/fielddata/mapmatrix/map_matrix.naix',[('NARC_map_matrix_'+p.name.replace('.','_'),i) for i,p in enumerate(matrices)])
events=listed('files/fielddata/eventdata/zone_event/meson.build','zone_event_json')
header('files/fielddata/eventdata/zone_event.naix',[('NARC_zone_event_'+p.stem+'_bin',i) for i,p in enumerate(events)])
scripts=listed('files/fielddata/script/scr_seq/meson.build','scr_seq_asm')
for i,p in enumerate(scripts):
    if int(p.stem.split('_')[2])!=i:raise ValueError('Script order changed')
header('files/fielddata/script/scr_seq.naix',[(p.stem+'_bin',i) for i,p in enumerate(scripts)])
messages=(root/'files/msgdata/NARC_msg.txt').read_text().splitlines()
header('files/msgdata/msg.naix',[(name.removeprefix('NARC_msg_').removesuffix('_bin'),i) for i,name in enumerate(messages)])
encounters=json.loads((root/'files/fielddata/encountdata/gs_enc_data.json').read_text())['encounters']
header('files/fielddata/encountdata/g_enc_data.naix',[(f'NARC_g_enc_data_g_enc_data_{i:08}',i) for i,_ in enumerate(encounters)])
# Pack unchanged matrix files with the same NARC container layout as other local resources.
image=bytearray();fat=bytearray()
for p in matrices:
    start=len(image);image.extend(p.read_bytes());fat.extend(struct.pack('<II',start,len(image)));image.extend(b'\xff'*((-len(image))%4))
def block(tag,data):return tag+struct.pack('<I',8+len(data))+data
body=block(b'BTAF',struct.pack('<HH',len(matrices),0)+fat)+block(b'BTNF',struct.pack('<IHH',4,0,1))+block(b'GMIF',image)
p=out/'resources/map_matrix.narc';p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(struct.pack('<4sHHIHH',b'NARC',0xfffe,0x100,16+len(body),16,3)+body)
(out/'field_manifest.json').write_text(json.dumps({'matrices':[p.name for p in matrices],'events':len(events),'scripts':len(scripts),'messages':len(messages),'encounters':len(encounters)},indent=2)+'\n')
constants=(root/'include/constants/maps.h').read_text()
names=[(int(number),symbol) for symbol,number in re.findall(r'^#define\s+(MAP_\w+)\s+(\d+)',constants,re.M) if symbol!='MAP_ID_MAX']
(out/'field_names.inc').write_text('static const char *const map_names[] = {\n'+''.join(f'[{i}]="{name}",\n' for i,name in names)+'};\n')

# Original land archive, unchanged and build-local.
import shutil
shutil.copyfile(root/"files/a/0/6/5",out/"resources/land_data.narc")

shutil.copyfile(root/"files/a/0/4/2",out/"resources/area_data.narc")
shutil.copyfile(root/"files/a/0/4/4",out/"resources/map_texture.narc")

shutil.copyfile(root/"files/a/0/4/0",out/"resources/prop_models.narc")
shutil.copyfile(root/"files/a/0/4/3",out/"resources/area_models.narc")

shutil.copyfile(root/"files/fielddata/build_model/bm_field_matshp.dat",out/"resources/bm_field_matshp.dat")
