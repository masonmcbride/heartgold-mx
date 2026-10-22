"""Unchanged original field-time closure; bounded existing owners are reused."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,struct,subprocess,sys
from narc_tools import pack
root,out,cc=map(pathlib.Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}'
functions={}
for path in (root/'src').rglob('*.c'):
 for m in re.finditer(pattern,path.read_text(),re.M|re.S):functions.setdefault(m[1],[]).append((path,m))
# These declarations are freshly generated earlier in this configure, not
# inferred from stale link products. Whole units retain their original owners.
existing=set()
for name in ('pokemon_decls.inc','script_kernel_decls.inc','save_c_records_decls.inc','phone_decls.inc'):
 existing.update(re.findall(r'\b(\w+)\([^;{}]*\)\s*;',(out/name).read_text()))
for relative in ('src/save_pokegear.c','src/save_vars_flags.c','src/apricorn_tree_sys.c','src/map_header.c','src/save_local_field_data.c'):
 existing.update(m[1] for m in re.finditer(pattern,(root/relative).read_text(),re.M|re.S))
existing.update('AllocAndReadPhoneBook StartMapSceneScript IsSEPlaying PlaySE_SetPitch Heap_Alloc Heap_AllocAtEnd Heap_Free GF_RTC_CopyDateTime GF_CalcCRC16 SaveData_Get SaveSubstruct_UpdateCRC SaveSubstruct_AssertCRC FieldSystem_BugContest_Get GearPhone_ToggleRinging GfGfxLoader_LoadFromNarc SaveArray_Get SaveArray_Const_Get MTRandom LCRandom SaveArray_Party_Get Save_VarsFlags_Get Save_Pokedex_Get'.split())
roots='Field_GetTimeOfDay FieldSystem_StartBugContestTimer GearPhoneRingManager_New SysInfoRTC_HandleContinueOnNewConsole Save_SysInfo_MacAddressIsMine Save_SysInfo_RTCOffsetIsMine Save_SysInfo_InitFromSystem Party_ResetAllShayminToLandForm Save_TrainerCard_Get GetShininessOfBadgeI Options_SetButtonModeOnMain Save_BerryPotRTC_Get sub_02093070 sub_020930C4 ov02_02251F20 SysTask_RingGearPhone'.split()
selected={};pending=list(roots)
while pending:
 name=pending.pop()
 if name in selected or name in existing or name not in functions:continue
 matches=functions[name]
 if len(matches)!=1:raise ValueError(('ambiguous time function',name))
 path,m=matches[0];selected[name]=(path,m)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',m[0]))&functions.keys())-selected.keys()-existing)
lines=[];decls=[];records=[]
references=set(re.findall(r'\b\w+\b',''.join(m[0] for _,m in selected.values())))
for path in sorted({p for p,_ in selected.values()}):
 source=path.read_text()
 for inc in re.findall(r'^#include "([^"]+)"',source,re.M):
  if inc.startswith(('constants/','files/msgdata/')):lines.append('#include "'+inc+'"')
 for m in re.finditer(r'^static const [^;{}\n]+?\b(\w+)\s*(?:\[[^;{}]*?\])?\s*=\s*\{.*?^\};',source,re.M|re.S):
  if m[1] in references:lines.append(m[0])
for name,(path,m) in sorted(selected.items()):
 line=path.read_text()[:m.start()].count('\n')+1
 decls.append(m[0].split(' {\n')[0]+';');lines.extend([f'#line {line} "{path}"',m[0]])
 records.append(dict(function=name,source=str(path.relative_to(root)),sha256=hashlib.sha256(m[0].encode()).hexdigest()))
write_text_if_changed(out/'field_time_kernel.inc', '\n'.join(lines)+'\n')
write_text_if_changed(out/'field_time_decls.inc', '\n'.join(decls)+'\n')
types=[re.search(r"typedef enum RTC_TimeOfDay \{.*?\} TIMEOFDAY;",(root/"include/gf_rtc.h").read_text(),re.S)[0]]
for relative,patterns in {
 'src/field/overlay_2_gear_phone.c':[r'typedef struct UnkStruct_02253C86 \{.*?\} UnkStruct_02253C86;'],
 'include/field_system.h':[r'struct GearPhoneRingManager \{.*?^\};'],
 'include/wild_encounter.h':[r'typedef struct EncounterSlot \{.*?\} EncounterSlot;'],
 'src/save_wifi_history.c':[r'struct SaveWiFiHistory \{.*?^\};'],
 'src/unk_02097268.c':[r'typedef struct \w+ \{.*?\} \w+;'],
}.items():
 for pat in patterns:
  found=re.findall(pat,(root/relative).read_text(),re.M|re.S)
  if not found:raise ValueError(('missing time type',relative,pat))
  types.extend(found)
types.insert(0,re.search(r'^#define SAVE_WIFI_HISTORY_REGION_WIDTH.*$',(root/'src/save_wifi_history.c').read_text(),re.M)[0])
write_text_if_changed(out/'field_time_types.inc', '\n'.join(types)+'\n')
declaration_inputs=['include/field_system.h','include/wild_encounter.h','include/unk_02097268.h','include/bug_contest_internal.h','include/safari_zone.h','include/sav_system_info.h']
write_text_if_changed(out/'field_time_provenance.json', json.dumps(dict(functions=records,reused=sorted(existing),declaration_inputs={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in declaration_inputs}),indent=2)+'\n')
# Compile the original Safari assembly template and split its ELF object symbols.
# No second encounter-data packer or copied species table.
source=root/'files/arc/safari_enc.json';assembly=out/'safari_enc.S';obj=out/'safari_enc.o'
subprocess.run([str((out/'jsonproc').resolve()),str(source),str(source.with_suffix('.json.txt')),str(assembly)],check=True)
write_text_if_changed(assembly, assembly.read_text().replace('@object','%object'))
subprocess.run([str(cc),'-target','arm-none-eabi','-c','-x','assembler-with-cpp','-I'+str(root/'include'),str(assembly),'-o',str(obj)],check=True)
data=obj.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'
shoff=struct.unpack_from('<I',data,32)[0];ents,count,_=struct.unpack_from('<HHH',data,46)
sections=[struct.unpack_from('<10I',data,shoff+i*ents) for i in range(count)]
def payload(s):return data[s[4]:s[4]+s[5]]
symtab=next(s for s in sections if s[1]==2);names=payload(sections[symtab[6]]);members={}
area_ids={n:int(v) for n,v in re.findall(r'^#define (SAFARI_ZONE_AREA_\w+)\s+(\d+)$',(root/'include/constants/safari.h').read_text(),re.M)}
for at in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
 name,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,at)
 name=names[name:names.index(b'\0',name)].decode()
 if name.startswith('sMap_'):
  ident=area_ids[name[len('sMap_'):]];section=payload(sections[index]);members[ident]=section[value:value+size]
if set(members)!=set(range(12)):raise ValueError(('Safari areas changed',members.keys()))
pack(out/'resources/safari_enc.narc',[members[i] for i in range(12)])
print('Original field-time closure:',len(selected),'functions; Safari:',len(members),'areas')
