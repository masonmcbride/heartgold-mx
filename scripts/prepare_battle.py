"""Prepare unchanged original battle logic and explicit host boundaries.

Generated code and game data stay build-local. Ambiguous definitions reject.
"""
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:3]);out.mkdir(parents=True,exist_ok=True)
paths=['src/battle/battle_system.c','src/battle/battle_controller.c','src/battle/battle_controller_player.c','src/battle/overlay_12_0224E4FC.c','src/battle/battle_command.c','src/dex_mon_measures.c','src/unk_0208805C.c','src/trainer_data.c','src/field/encounter_check.c']
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}'
functions={};sources={}
for path in paths:
 s=(root/path).read_text();sources[path]=s
 for m in re.finditer(pattern,s,re.M|re.S):
  if m[1] in functions:raise ValueError(('ambiguous battle function',m[1]))
  functions[m[1]]=(path,m)
# Native UI replaces SDK graphics and application tasks; all rule functions stay original.
exclude={'BattleController_SendLocalMessage','BattleController_RecvMessage','BattleSystem_TryRecvMessage','BattleSystem_LoadLevelUpNameplate','BattleSystem_UnloadLevelUpNameplate'}
selected={n for n,(p,_) in functions.items() if p not in {'src/battle/battle_system.c','src/unk_0208805C.c','src/trainer_data.c','src/field/encounter_check.c'}}-exclude
selected|={'FieldSystem_EncounterRateRoll','FieldSystem_UpdateTurnFrameCounter','EncSlotArray_Init_Land','ApplyAbilityEffectToEncounterRate','ApplyLeadMonHeldItemEffectToEncounterRate','FieldSystem_GenerateRegularEncounter','TrainerMessageWithIdPairExists','CreateNPCTrainerParty','TrainerData_ReadTrData','TrainerData_ReadTrPoke','TrainerClass_GetGenderOrTrainerCount','TrMon_OverridePidGender','TrMon_FrustrationCheckAndSetFriendship','BattleSystem_BufferMessage','BattleSystem_AdjustMessageForSide','BattleSystem_GameStatIncrement','CalculateHpBarPixelsLength','CalculateHpBarColor','HpBar_GetColorIdx'}
# Original data/system accessors required by the logic closure.
while True:
 needed=set(re.findall(r'\b(\w+)\b',''.join(functions[n][1][0] for n in selected)))
 add={n for n in needed-selected if n in functions and functions[n][0] in {'src/battle/battle_system.c','src/field/encounter_check.c'}}
 # Formatting and graphics are native presentation boundaries.
 add-={'EncounterGen_ChooseUnownForm','BattleSystem_PrintBattleMessage','BattleSystem_HpBar_Delete','ov12_0223C224','ov12_0223C4E8'}
 if not add:break
 selected|=add
# Emit original non-function data declarations in their original order.
records=[];lines=[]
for path,s in sources.items():
 if path=='src/field/encounter_check.c':
  prefix=s[:min(m.start() for p,m in functions.values() if p==path)];prefix=re.sub(r'^#include.*\n|^FS_EXTERN_OVERLAY.*\n','',prefix,flags=re.M);prefix=re.sub(r'^(?:static )?[^\n;{}]+\([^;{}]*\);\n','',prefix,flags=re.M);lines.append(prefix);continue
 if path=='src/trainer_data.c':
  lines.append(re.search(r'^static const u8 sTrainerGenders\[\] = \{.*?^\};',s,re.M|re.S)[0]);continue
 if path in {'src/battle/battle_system.c','src/unk_0208805C.c','src/trainer_data.c','src/field/encounter_check.c'}:continue
 at=0;parts=[]
 for m in re.finditer(pattern,s,re.M|re.S):
  parts.append(s[at:m.start()]);at=m.end()
 parts.append(s[at:]);prefix=''.join(parts)
 prefix=re.sub(r'^#include.*\n','',prefix,flags=re.M)
 if path=='src/dex_mon_measures.c':prefix=prefix.replace('msg_0812','812').replace('msg_0814','814')
 # Forward declarations are emitted uniformly after shared record definitions.
 prefix=re.sub(r'^(?:static )?[^\n;{}]+\([^;{}]*\);\n','',prefix,flags=re.M)
 prefix=re.sub(r'^extern ManagedSpriteTemplate .*;\n','',prefix,flags=re.M)
 lines.append(prefix)
for name in sorted(selected):
 path,m=functions[name];line=sources[path][:m.start()].count('\n')+1
 lines.extend([f'#line {line} "{root/path}"',m[0]])
 records.append({'source':path,'function':name,'sha256':hashlib.sha256(m[0].encode()).hexdigest()})
# All functions, including explicit native task boundaries, have original signatures.
encounter_source=sources['src/field/encounter_check.c']
encounter_records=re.search(r'typedef enum RTC_TimeOfDay \{.*?\} TIMEOFDAY;', (root/'include/gf_rtc.h').read_text(),re.S)[0]+'\n'+re.search(r'enum RadioMusicPlayingSeq \{.*?\};',(root/'include/overlay_2/overlay_02_gear_phone.h').read_text(),re.S)[0]+'\n'+re.search(r'static inline u16 LCRandRange\(.*?^\}',(root/'include/math_util.h').read_text(),re.M|re.S)[0]+'\n'+'\n'.join(re.findall(r'typedef struct (?:UnlockedUnown|EncounterGenState) \{.*?\} \w+;',encounter_source,re.S))+'\n'
encounter_records+='\n'.join(re.findall(r'^#define (?:ROD_TYPE_\w+|NUM_ENCOUNTERS_HEADBUTT)\s+[^\n]+', (root/'include/overlay_01.h').read_text()+'\n'+(root/'include/field/headbutt.h').read_text(),re.M))+'\n'
(out/'encounter_records.inc').write_text(encounter_records)
lines=[re.sub(r'typedef struct (?:UnlockedUnown|EncounterGenState) \{.*?\} \w+;','',line,flags=re.S) for line in lines]
(out/'battle_decls.inc').write_text('\n'.join(m[0].split(' {\n')[0]+';' for n,(_,m) in functions.items() if n in selected or n in exclude)+'\n')
(out/'battle_kernel.inc').write_text('\n'.join(lines).replace('msg_0813','813').replace('msg_0815','815').replace('msg_0812','812').replace('msg_0814','814')+'\n')
h=(root/'include/battle/battle.h').read_text()
plain=h[h.index('typedef struct BattleMessage {'):h.index('typedef struct BattleSystem BattleSystem;')]
plain+='\ntypedef struct BattleSystem BattleSystem;\n'
plain+=h[h.index('typedef struct UnkBattleSystemSub17C {'):h.index('struct GetterWork {')]
plain+=h[h.index('struct GetterWork {'):h.index('typedef struct {',h.index('struct GetterWork {'))]
plain+=h[h.index('typedef struct {',h.index('struct GetterWork {')):h.index('\n#endif',h.index('struct GetterWork {'))]
plain+='\ntypedef void (*ControllerFunction)(BattleSystem *,BattleContext *);\n'
extra=''
for path,tag in [('include/obj_char_transfer.h','UnkStruct_02021AC8'),('include/naming_screen.h','NamingScreenArgs')]:
 text=(root/path).read_text();extra+=re.search(r'typedef struct '+tag+r' \{.*?\} '+tag+r';',text,re.S)[0]+'\n'
enum=(root/'include/launch_application.h').read_text();extra=re.search(r'typedef enum NameScreenType \{.*?\} NameScreenType;',enum,re.S)[0]+'\n'+extra
message_records=(root/'include/battle/battle_message_structs.h').read_text()
message_records=re.sub(r'^#.*\n','',message_records,flags=re.M)
dex_header=(root/'include/dex_mon_measures.h').read_text()
extra+=re.search(r'typedef struct PokedexData \{.*?\} PokedexData;',dex_header,re.S)[0]+'\n'
(out/'battle_records.inc').write_text(extra+plain+message_records)
# Original function signatures define the host imports; no guessed implicit prototypes.
calls=set(re.findall(r'\b([A-Za-z_]\w*)\s*\(',''.join(functions[n][1][0] for n in selected)))
external=calls-selected-exclude-{'if','for','while','switch','return','sizeof','GF_ASSERT','NELEMS'}
prototypes={}
for p in (root/'include').rglob('*.h'):
 text=p.read_text(errors='replace')
 for m in re.finditer(r'^([A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\));',text,re.M):
  if m[2] in external:
   signature=m[1]
   if m[2] in prototypes and prototypes[m[2]]!=signature:continue
   prototypes[m[2]]=signature
for name in external:
 if name in functions:prototypes[name]=functions[name][1][0].split(' {\n')[0]
(out/'battle_imports.inc').write_text('\n'.join(prototypes[n]+';' for n in sorted(prototypes))+'\n')
constants=sorted(set(re.findall(r'\bmsg_0197_(\d+)\b',''.join(sources.values()))))
(out/'battle_message_ids.inc').write_text('\n'.join(f'#define msg_0197_{n} {int(n)}' for n in constants)+'\n')
# Original constant tables live in assembler; preserve values in build-local C.
externs={}
for path,source in sources.items():
 if path in {'src/unk_0208805C.c','src/trainer_data.c','src/field/encounter_check.c'}:continue
 for m in re.finditer(r'^extern ((?:const )?(?:u8|u16|u32|TempStatsStruct)) (\w+)(\[[^;]*\])?;',source,re.M):externs[m[2]]=(m[1],m[3] or '')
asm_text='\n'.join(p.read_text(errors='replace') for p in (root/'asm').glob('*.s'))
tables=[]
for name,(typ,dims) in externs.items():
 match=re.search(r'^'+re.escape(name)+r':[^\n]*\n(.*?)(?=^[A-Za-z_]\w*:|\Z)',asm_text,re.M|re.S)
 if not match:raise ValueError(('missing original table',name))
 values=[]
 for line in match[1].splitlines():
  d=re.match(r'\s*\.(?:byte|short|hword|word)\s+([^;]+)',line)
  if d:values.extend(x.strip() for x in d[1].split(','))
 if not values:raise ValueError(('empty table',name))
 tables.append(f'{typ} {name}{dims}={{'+','.join(values)+'};')
(out/'battle_data.inc').write_text('\n'.join(tables)+'\n')
# Original VM opcode table, extracted without guessing numeric assignments.
table=(root/'asm/overlay_12_battle_command.s').read_text()
names=re.findall(r'^\s*\.word (BtlCmd_\w+)',table,re.M)
if not names or any(n not in functions for n in names):raise ValueError('Invalid original battle command table')
(out/'battle_table.inc').write_text('\n'.join(n+',' for n in names)+'\n')
(out/'battle_provenance.json').write_text(json.dumps({'functions':records,'imports':sorted(external),'unresolved_prototypes':sorted(external-set(prototypes)),'opcodes':names,'sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}},indent=2)+'\n')

# Original battle script macros assemble to words through the shared ELF reader.
if len(sys.argv)>3:
 import concurrent.futures
 from script_assembly import assemble,sections
 from narc_tools import pack
 cc=pathlib.Path(sys.argv[3]);folder=out/'battle_scripts';folder.mkdir(exist_ok=True)
 def compile_script(source):
  target=folder/(source.stem+'.o');data=assemble(root,out,cc,source,target)
  table,content,name=sections(data)
  chunks=[content(section) for section in table if section[1]==1 and section[5] and name(section[0]) in ('.text','.rodata','.data')]
  if len(chunks)!=1:raise ValueError(('ambiguous battle script section',source))
  if len(chunks[0])%4 or len(chunks[0])>1600:raise ValueError(('invalid battle script',source))
  return chunks[0]
 for kind in ('effect_script','move_script','subscript'):
  source_folder=root/'files/battledata/script'/kind
  paths=[source_folder/name for name in re.findall(r"'([^']+\.s)'",(source_folder/'meson.build').read_text())]
  if [int(p.stem.split('_')[2] if kind!='subscript' else p.stem.split('_')[1]) for p in paths]!=list(range(len(paths))):raise ValueError(('battle script numbering',kind))
  with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:members=list(pool.map(compile_script,paths))
  pack(out/'resources'/('battle_'+kind+'.narc'),members)

# Assemble original measurement templates and preserve their object order.
if len(sys.argv)>3:
 import struct,subprocess
 template=root/'files/application/zukanlist/zkn_data/zukan_data.json.txt'
 for suffix,gira in [('',False),('_gira',True)]:
  source=out/('dex_measures'+suffix+'.S')
  subprocess.run([str((out/'jsonproc').resolve()),str(template.with_suffix('')),str(template),str(source)],check=True)
  source.write_text(('#define GIRA 1\n' if gira else '')+source.read_text().replace('.rodata','.section .rodata').replace('@object','%object'))
  data=assemble(root,out,cc,source,source.with_suffix('.o'))
  table,content,name=sections(data);symtab=next(s for s in table if s[1]==2)
  strings=content(table[symtab[6]]);objects=[]
  for offset in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
   n,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,offset)
   label=strings[n:strings.index(b'\0',n)].decode()
   if info&15==1 and size:
    member=content(table[index])[value:value+size]
    if len(member)!=size:raise ValueError(('truncated measurement',label))
    objects.append((index,value,label,member))
  objects.sort()
  if not objects or len({x[2] for x in objects})!=len(objects):raise ValueError('Invalid dex objects')
  pack(out/'resources'/('dex_measures'+suffix+'.narc'),[x[3] for x in objects])
  if not gira:(out/'dex_measure_ids.inc').write_text(''.join(f'#define NARC_zukan_data_{x[2]} {i}\n' for i,x in enumerate(objects)))

metadata=json.loads((out/'battle_provenance.json').read_text())
metadata['measurement_inputs']={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in [
 'include/dex_mon_measures.h','files/application/zukanlist/zkn_data/zukan_data.json',
 'files/application/zukanlist/zkn_data/zukan_data.json.txt']}
(out/'battle_provenance.json').write_text(json.dumps(metadata,indent=2)+'\n')

# Original trainer records and party objects; no handwritten species/team tables.
if len(sys.argv)>3:
 folder=root/'files/poketool/trainer';trainers=folder/'trainers.json'
 generated=out/'trainer_data.c'
 subprocess.run([str((out/'jsonproc').resolve()),str(trainers),str(folder/'trdata.json.txt'),str(generated)],check=True)
 generated.write_text('#include <stdio.h>\n'+generated.read_text()+'\nint main(void){return fwrite(__data,sizeof(__data),1,stdout)!=1;}\n')
 executable=out/'trainer_data'
 subprocess.run([str(cc),'-I'+str(pathlib.Path(__file__).resolve().parent.parent/'src/compat/include'),'-I'+str(root/'include'),str(generated),'-o',str(executable)],check=True)
 data=subprocess.check_output([str(executable.resolve())]);count=len(json.loads(trainers.read_text())['trainers'])
 if len(data)!=count*20:raise ValueError('Original trainer record size changed')
 pack(out/'resources/trainer_data.narc',[data[i*20:(i+1)*20] for i in range(count)])
 source=out/'trainer_parties.S'
 subprocess.run([str((out/'jsonproc').resolve()),str(trainers),str(folder/'trpoke.json.txt'),str(source)],check=True)
 source.write_text(source.read_text().replace('.rodata','.section .rodata').replace('@object','%object'))
 obj=assemble(root,out,cc,source,source.with_suffix('.o'));table,content,name=sections(obj)
 symtab=next(s for s in table if s[1]==2);strings=content(table[symtab[6]]);members={}
 for offset in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
  n,value,size,info,other,index=struct.unpack_from('<IIIBBH',obj,offset)
  label=strings[n:strings.index(b'\0',n)].decode()
  if label.startswith('gTrainerPoke_'):
   number=int(label[len('gTrainerPoke_'):]);member=content(table[index])[value:value+size]
   if number in members or len(member)!=size or size>120:raise ValueError(('Invalid original trainer party',label))
   members[number]=member
 if sorted(members)!=list(range(count)):raise ValueError('Original trainer party indices changed')
 pack(out/'resources/trainer_parties.narc',[members[i] for i in range(count)])
 capsule=root/'files/application/custom_ball/edit/gs_cb_data.narc'
 (out/'resources/trainer_capsules.narc').write_bytes(capsule.read_bytes())
 metadata=json.loads((out/'battle_provenance.json').read_text())
 metadata['trainer_inputs']={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [trainers,folder/'trdata.json.txt',folder/'trpoke.json.txt',capsule]}
 metadata['trainer_count']=count
 (out/'battle_provenance.json').write_text(json.dumps(metadata,indent=2)+'\n')

if len(sys.argv)>3:
 original_trainers=json.loads(trainers.read_text())['trainers']
 trainer_names=[t['name'].removeprefix('{TRNAME}') for t in original_trainers]
 (out/'trainer_names.inc').write_text('static const char *const trainer_names[]={'+','.join(json.dumps(n,ensure_ascii=True) for n in trainer_names)+'};\n')

if len(sys.argv)>3:
 for source,target in [('trtbl.narc','trainer_message_table.narc'),('trtblofs.narc','trainer_message_offsets.narc')]:
  path=root/'files/poketool/trmsg'/source
  (out/'resources'/target).write_bytes(path.read_bytes())

if len(sys.argv)>3:
 original=root/'files/fielddata/encountdata/gs_enc_data.json'
 generated=out/'wild_encounters.c'
 subprocess.run([str((out/'jsonproc').resolve()),str(original),str(original.with_suffix('.json.txt')),str(generated)],check=True)
 generated.write_text('#include <stdio.h>\n'+generated.read_text()+'\nint main(void){return fwrite(__data,sizeof(__data),1,stdout)!=1;}\n')
 exe=out/'wild_encounters'
 subprocess.run([str(cc),'-I'+str(pathlib.Path(__file__).resolve().parent.parent/'src/compat/include'),'-I'+str(root/'include'),str(generated),'-o',str(exe)],check=True)
 data=subprocess.check_output([str(exe.resolve())]);count=len(json.loads(original.read_text())['encounters'])
 if len(data)!=196*count:raise ValueError('Original encounter record size changed')
 pack(out/'resources/wild_encounters.narc',[data[i*196:(i+1)*196] for i in range(count)])

if len(sys.argv)>3:
 # Indexed artwork and palettes are separate in the original build: front
 # PNG supplies normal PLTE, back PNG supplies shiny PLTE (make_pokegra.py).
 import zlib
 sprite_folder=root/'files/poketool/pokegra/pokegra'
 def png_chunks(data):
  if data[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('Invalid battle PNG')
  at=8;chunks=[]
  while at<len(data):
   n=struct.unpack_from('>I',data,at)[0];tag=data[at+4:at+8];payload=data[at+8:at+8+n]
   if at+n+12>len(data) or zlib.crc32(tag+payload)!=struct.unpack_from('>I',data,at+8+n)[0]:raise ValueError('Invalid PNG chunk')
   chunks.append((tag,payload));at+=n+12
  return chunks
 def palette(data):
  return next(payload for tag,payload in png_chunks(data) if tag==b'PLTE')
 def recolor(data,pal):
  chunks=png_chunks(data)
  ihdr=chunks[0][1]
  if struct.unpack_from('>II',ihdr) not in ((160,80),(1,1)) or ihdr[9]!=3:raise ValueError('Unexpected indexed battle image')
  result=bytearray(data[:8])
  for tag,payload in chunks:
   if tag==b'PLTE':
    if len(payload)!=len(pal):raise ValueError('Battle palette size mismatch')
    payload=pal
   result+=struct.pack('>I',len(payload))+tag+payload+struct.pack('>I',zlib.crc32(tag+payload))
  return bytes(result)
 sprites=[]
 for species in range(494):
  folder=sprite_folder/f'{species:04}'
  palettes=[]
  for facing in ('front','back'):
   candidates=[(folder/gender/(facing+'.png')).read_bytes() for gender in ('female','male')]
   palettes.append(next((palette(data) for data in candidates if data),None))
  for shiny in range(2):
   for gender in ('male','female'):
    for facing in ('front','back'):
     data=(folder/gender/(facing+'.png')).read_bytes()
     sprites.append(recolor(data,palettes[shiny]) if data and palettes[shiny] else b'')
 pack(out/'resources/battle_sprites_png.narc',sprites)
