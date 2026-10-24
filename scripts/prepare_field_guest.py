"""Assemble whole original movement units and relocate a bounded guest image.
Unprovided imports receive named failure traps, never successful stubs.
"""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
root,out,cc=map(pathlib.Path,sys.argv[1:])
sources=[root/'asm'/name for name in ('unk_0205CB48.s','unk_0205FD20.s','unk_02054648.s','unk_02062108.s','unk_0203DB6C.s','unk_data_020FCBD8.s','unk_data_020FDB44.s','unk_data_020FD978.s','overlay_01_022053EC.s','unk_0203BA5C.s','unk_02017FAC.s','unk_020658D4.s')]
raw='\n'.join(p.read_text() for p in sources)
# One exact effect update slice; do not claim compilation of its containing unit.
effect_path=root/'asm/overlay_01_022001E4.s'
effect_source=effect_path.read_text()
effect_matches=list(re.finditer(r'\tthumb_func_start ov01_02200480\n.*?\tthumb_func_end ov01_02200480',effect_source,re.S))
if len(effect_matches)!=1:raise ValueError('Original emote update definition changed')
effect_slice=effect_matches[0][0]
raw+='\n.text\n'+effect_slice+'\n'
# Shared field interaction uses the original surf eligibility instructions.
surf_path=root/'asm/overlay_01_021F1AFC.s'
surf_matches=list(re.finditer(r'\tthumb_func_start Field_PlayerCanSurfOnTile\n.*?\tthumb_func_end Field_PlayerCanSurfOnTile',surf_path.read_text(),re.S))
if len(surf_matches)!=1:raise ValueError('Original surf predicate changed')
raw+='\n.text\n'+surf_matches[0][0]+'\n'
rock_matches=list(re.finditer(r'\tthumb_func_start MetatileBehavior_IsRockClimbInDirection\n.*?\tthumb_func_end MetatileBehavior_IsRockClimbInDirection',surf_path.read_text(),re.S))
if len(rock_matches)!=1:raise ValueError('Original rock-climb predicate changed')
raw+='\n.text\n'+rock_matches[0][0]+'\n'

# Original avatar transition dispatcher and selected pose callbacks. Other bits
# keep named failure traps; the public boundary rejects them before execution.
avatar_slices=[]
for name in ('Field_PlayerAvatar_ApplyTransitionFlags','ov01_021F1B38','ov01_021F1CEC','ov01_021F1C30','ov01_021F1D24','ov01_021F3084'):
 matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,surf_path.read_text(),re.S)
 if len(matches)!=1:raise ValueError(('Original avatar transition changed',name))
 avatar_slices.append((name,matches[0]));raw+='\n.text\n'+matches[0]+'\n'
avatar_table=re.findall(r'sPlayerAvatarBitUpdateFuncs:[^\n]*\n((?:\t.word \w+\n){15})',surf_path.read_text())
if len(avatar_table)!=1:raise ValueError('Original avatar transition table changed')
raw+='\n.rodata\nsPlayerAvatarBitUpdateFuncs:\n'+avatar_table[0]

# Pure original door-sound selector, with a scoped native animation-list reader.
door_path=root/'asm/overlay_01_021E90C0.s'
door_matches=re.findall(r'\tthumb_func_start GetDoorSE\n.*?\tthumb_func_end GetDoorSE',door_path.read_text(),re.S)
if len(door_matches)!=1:raise ValueError('Original door sound selector changed')
raw+='\n.text\n'+door_matches[0]+'\n'

# Original Apricorn availability/type → sprite selector and its original LUT.
apricorn_path=root/'asm/unk_020632B0.s'
apricorn_matches=list(re.finditer(r'\tthumb_func_start sub_020640A4\n.*?\tthumb_func_end sub_020640A4',apricorn_path.read_text(),re.S))
if len(apricorn_matches)!=1:raise ValueError('Original Apricorn sprite selector changed')
sprite_source=(root/'asm/overlay_01_sprite_data.s').read_text()
tables=re.findall(r'ov01_02207274:\n((?:\t.word SPRITE_\w+\n)+)',sprite_source)
if len(tables)!=1:raise ValueError('Original Apricorn sprite table changed')
raw+='\n#include "constants/sprites.h"\n.text\n'+apricorn_matches[0][0]+'\n.rodata\n.public ov01_02207274\nov01_02207274:\n'+tables[0]+'\n'


# Static trainer facing reset: unchanged four initializers, their shared
# state initializer/tick, and the original ordinary-object type initializer.
actor_slices=[]
for relative,names in {'asm/unk_02061284.s':('sub_02061648','sub_0206166C','sub_02061690','sub_0206169C','sub_020616A8','sub_020616B4'),'asm/unk_020632B0.s':('sub_02063AFC',)}.items():
 source=(root/relative).read_text()
 for name in names:
  matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,source,re.S)
  if len(matches)!=1:raise ValueError(('Original static actor reset changed',name))
  actor_slices.append(dict(source=relative,function=name,slice_sha256=hashlib.sha256(matches[0].encode()).hexdigest()))
  raw+='\n.text\n'+matches[0]+'\n'
# Resolve admitted callbacks from the original movement and type tables.
movement_source=(root/'asm/unk_data_020FCBD8.s').read_text()
movement_table=re.search(r'_020FD1F4:\n((?:\t.word \w+\n){57})',movement_source)
if not movement_table:raise ValueError('Original movement table changed')
movement_rows=re.findall(r'\t.word (\w+)',movement_table[1])
actor_rows=[]
for movement in range(14,18):
 descriptor=re.search(r'^'+movement_rows[movement]+r':\n((?:\t.word \w+\n){5})',movement_source,re.M)
 if not descriptor:raise ValueError(('Original facing movement descriptor changed',movement))
 values=re.findall(r'\t.word (\w+)',descriptor[1])
 if values[2]!='sub_0206166C' or values[3]!='sub_0205FCBC':raise ValueError(('Unexpected static actor callbacks',movement,values))
 actor_rows.append('ACTOR_FACING('+str(movement)+',HG_FIELD_GUEST_'+values[1]+')\n')
write_text_if_changed(out/'field_guest_actor.inc',''.join(actor_rows))
type_source=(root/'asm/unk_020632B0.s').read_text()
type_table=re.search(r'_020FE104:\n((?:\t.word \w+\n){12})',type_source)
if not type_table:raise ValueError('Original actor type table changed')
write_text_if_changed(out/'field_guest_actor_types.inc',''.join('case '+str(i)+':return 1;\n' for i,name in enumerate(re.findall(r'\t.word (\w+)',type_table[1])) if name=='sub_02063AFC'))

# Bounded original brightness controller closure. Other wipe/window fades remain
# unprovided, and hardware brightness is captured by the native display boundary.
fade_path=root/'asm/unk_0201010C.s'
fade_source=fade_path.read_text();fade_slices=[]
for name in ('FadeFunc_00','FadeFunc_01','sub_02010B14','sub_02010BB4','sub_02010BF4','sub_02010A6C'):
 matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,fade_source,re.S)
 if len(matches)!=1:raise ValueError(('Original brightness controller changed',name))
 fade_slices.append((name,matches[0]));raw+='\n.text\n'+matches[0]+'\n'

message_sources=[root/'files/msgdata/msg'/name for name in ('msg_0096_D31R0201.json','msg_0066_D23R0102.json')]
for p in message_sources:
 header=out/'msgdata/msg'/p.with_suffix('.h').name
 definitions=dict(re.findall(r'^#define (msg_\w+) (\d+)$',header.read_text(),re.M))
 if definitions!={m['id']:str(i) for i,m in enumerate(json.loads(p.read_text())['messages'])}:raise ValueError(('Shared message indices changed',p))
from guest_assembly import assemble
image,code_size,imports=assemble(raw,root,out,cc,'field_guest','HG_FIELD_GUEST')
# Same relocation identities as the guest image. Validate dense slots before
# emitting O(1) membership/name indexing; handlers compare numeric addresses.
base=min(imports.values())
if sorted(imports.values())!=list(range(base,base+4*len(imports),4)):
 raise ValueError('Motion import relocation slots are not dense/aligned')
write_text_if_changed(out/'field_guest_dispatch.inc',
 'enum HGMotionImport {\n'+''.join(' HG_MOTION_'+name+' = '+str(address)+'u,\n' for name,address in imports.items())+'};\n'+
 '#define HG_MOTION_IMPORT_BASE '+str(base)+'u\n'+
 '#define HG_MOTION_IMPORT_COUNT '+str(len(imports))+'u\n')

# Typed terrain predicates are already compiled from the complete original C
# unit. Derive the guest bridge in one batch rather than per movement/species.
terrain_source=root/'src/metatile_behavior.c'
terrain_predicates=re.findall(r'^BOOL (\w+)\((?:u8|u32) \w+\)',terrain_source.read_text(),re.M)
if len(terrain_predicates)!=len(set(terrain_predicates)):raise ValueError('Ambiguous terrain predicates')
terrain_imports=sorted(set(terrain_predicates)&set(imports))
write_text_if_changed(out/'field_guest_terrain.inc',''.join('TILE_PREDICATE('+name+')\n' for name in terrain_imports))
write_text_if_changed(out/'field_guest_provenance.json', json.dumps({'sources':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources+message_sources+[terrain_source]},'function_slices':[{'source':str(effect_path),'function':'ov01_02200480','source_sha256':hashlib.sha256(effect_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(effect_slice.encode()).hexdigest()}],'interaction_slices':[{'source':str(surf_path),'function':name,'source_sha256':hashlib.sha256(surf_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()} for name,body in [('Field_PlayerCanSurfOnTile',surf_matches[0][0]),('MetatileBehavior_IsRockClimbInDirection',rock_matches[0][0])]],'terrain_predicates':terrain_imports,'image_sha256':hashlib.sha256(image).hexdigest(),'image_size':len(image),'code_size':code_size,'imports':imports},indent=2)+'\n')
print(f'Original field guest: {len(image)} bytes, {len(imports)} named import traps')

# Record the selector and LUT independently of whole-assembly coverage.
provenance_path=out/'field_guest_provenance.json'
provenance=json.loads(provenance_path.read_text())
provenance['apricorn_selector']={'source':str(apricorn_path),'function':'sub_020640A4','source_sha256':hashlib.sha256(apricorn_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(apricorn_matches[0][0].encode()).hexdigest(),'table_source':'asm/overlay_01_sprite_data.s','table_sha256':hashlib.sha256(tables[0].encode()).hexdigest()}
write_text_if_changed(provenance_path, json.dumps(provenance,indent=2)+'\n')

provenance=json.loads(provenance_path.read_text())
provenance['brightness_controller']=[{'source':str(fade_path),'function':name,'source_sha256':hashlib.sha256(fade_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()} for name,body in fade_slices]
write_text_if_changed(provenance_path, json.dumps(provenance,indent=2)+'\n')

provenance['actor_reset']={'functions':actor_slices,'movement_table_sha256':hashlib.sha256(movement_table[1].encode()).hexdigest(),'type_table_sha256':hashlib.sha256(type_table[1].encode()).hexdigest()}
provenance['avatar_transitions']={'functions':[{'function':name,'slice_sha256':hashlib.sha256(body.encode()).hexdigest()} for name,body in avatar_slices],'source':str(surf_path),'source_sha256':hashlib.sha256(surf_path.read_bytes()).hexdigest(),'table_sha256':hashlib.sha256(avatar_table[0].encode()).hexdigest()}
write_text_if_changed(provenance_path, json.dumps(provenance,indent=2)+'\n')

# The healing task has its own guest arena: expanding movement into marshaled
# field records would corrupt state. Both images use the same relocation path.
healing_path=root/'asm/overlay_02_02248728.s'
healing_source=healing_path.read_text();healing_slices=[]
for name in ('PokecenterAnimCreate','PokecenterAnimRun'):
 matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,healing_source,re.S)
 if len(matches)!=1:raise ValueError(('Original healing controller changed',name))
 healing_slices.append(matches[0])
start=healing_source.index('ov02_02253D90:')
end=healing_source.index('ov02_02253DD8:',start)
position=healing_source[start:end]
values=re.findall(r'0x[0-9A-Fa-f]+',re.sub(r';[^\n]*','',position))
if len(values)!=72:raise ValueError('Original six healing ball offsets changed')
healing_raw='#include "constants/sndseq.h"\n.text\n'+'\n'.join(healing_slices)+'\n.rodata\n'+position+'\n'
lead_path=root/'asm/unk_0205BB1C.s'
lead_matches=re.findall(r'\tthumb_func_start Save_GetPartyLeadAlive\n.*?\tthumb_func_end Save_GetPartyLeadAlive',lead_path.read_text(),re.S)
if len(lead_matches)!=1:raise ValueError('Original party lead selector changed')
healing_raw+='\n.text\n'+lead_matches[0]+'\n'
offset_path=root/'asm/unk_02054648.s'
offset_matches=re.findall(r'\tthumb_func_start sub_02054DC8\n.*?\tthumb_func_end sub_02054DC8',offset_path.read_text(),re.S)
if len(offset_matches)!=1:raise ValueError('Original matrix prop offset changed')
healing_raw+='\n.text\n'+offset_matches[0]+'\n'
hi,hcode,himports=assemble(healing_raw,root,out,cc,'healing_guest','HG_HEALING_GUEST')
write_text_if_changed(out/'healing_provenance.json', json.dumps({'source':str(healing_path),'source_sha256':hashlib.sha256(healing_path.read_bytes()).hexdigest(),'functions':['PokecenterAnimCreate','PokecenterAnimRun'],'party_lead_source':str(lead_path),'party_lead_sha256':hashlib.sha256(lead_path.read_bytes()).hexdigest(),'ball_offsets_sha256':hashlib.sha256(position.encode()).hexdigest(),'image_sha256':hashlib.sha256(hi).hexdigest(),'image_size':len(hi),'imports':himports},indent=2)+'\n')

provenance['door_sound']={'source':str(door_path),'source_sha256':hashlib.sha256(door_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(door_matches[0].encode()).hexdigest()}
write_text_if_changed(provenance_path, json.dumps(provenance,indent=2)+'\n')

# Save-dependent field queries retain their original decisions in a separate arena.
query_raw='.text\n'
query_sources={}
for relative,names in {'asm/unk_02068FC8.s':('sub_020691E8',),'asm/unk_02030A98.s':('FrontierSave_GetStat',)}.items():
 source=(root/relative).read_text();query_sources[relative]=hashlib.sha256(source.encode()).hexdigest()
 for name in names:
  matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,source,re.S)
  if len(matches)!=1:raise ValueError(('Original field query changed',name))
  query_raw+=matches[0]+'\n'
qi,qcode,qimports=assemble(query_raw,root,out,cc,'field_queries_guest','HG_QUERY_GUEST')
write_text_if_changed(out/'field_queries_provenance.json', json.dumps({'sources':query_sources,'image_size':len(qi),'imports':qimports},indent=2)+'\n')
