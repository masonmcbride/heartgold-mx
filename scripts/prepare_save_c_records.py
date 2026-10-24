"""Extract unchanged C save callbacks and their bounded initializer closure."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,subprocess,sys
root,out=map(pathlib.Path,sys.argv[1:3]); out.mkdir(parents=True,exist_ok=True)
repo=pathlib.Path(__file__).resolve().parent.parent
# Explicit existing owners, validated by the final link; never inspect stale
# build products to decide whether a callback should be emitted.
existing=set("""ApricornTrees_Init DestroyMsgData GameStats_Init GameStats_sizeof
GetMonData MI_CpuClear32 MI_CpuClear8 MI_CpuClearFast MI_CpuFill16 MTRandom
Mail_Init NewMsgDataFromNarc Options_Init Party_InitWithMaxSize PlayerProfile_Init
SaveData_Pokegear_Init SaveData_Pokegear_sizeof Save_Bag_Init Save_Bag_sizeof
Save_LocalFieldData_Init Save_LocalFieldData_sizeof Save_MapObjects_Init
Save_MapObjects_sizeof Save_Pokedex_Init Save_VarsFlags_Init Save_VarsFlags_sizeof
StringFillEOS ZeroBoxMonData ZeroMonData SaveData_Get SaveSubstruct_UpdateCRC
ReadMsgDataIntoU16Array GF_RTC_CopyDateTime sub_0202C08C SaveArray_Get SaveArray_Const_Get
""".split())
# Pokemon rules already belong to the shared original Pokemon closure.
existing.update(re.findall(r'\b(\w+)\([^;{}]*\)\s*;', (out/'pokemon_decls.inc').read_text()))
registry=(root/'src/save_arrays.c').read_text()
roots=(set(re.findall(r'\(SAVE(?:SIZE|INIT)FN\)(\w+)',registry))|set('MailMsg_Init_FromTemplate sub_0203A01C PCStorage_GetActiveBox PCStorage_SetActiveBox PCStorage_FindFirstBoxWithEmptySlot PCStorage_CountEmptySpotsInAllBoxes PCStorage_PlaceMonInBoxFirstEmptySlot PCStorage_GetNumBoxes PCStorage_GetSizeOfBox Save_GetPCBoxModifiedFlags Save_ResetPCBoxModifiedFlags Save_SetAllPCBoxesModified sub_020271A0 Save_Misc_Get SaveMisc_CheckExtraChunksExist SaveMisc_SetExtraChunksExist Save_PlayerData_GetIGTAddr Save_Daycare_Get SaveArray_PCStorage_Get AddIGTSeconds sub_0202AC38 sub_0202AC60 Save_GetGymmickPtr'.split()))-existing
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}'
functions={}
for path in (root/'src').rglob('*.c'):
 for m in re.finditer(pattern,path.read_text(),re.M|re.S):functions.setdefault(m[1],[]).append((path,m))
selected={};pending=list(roots&functions.keys())
while pending:
 name=pending.pop()
 if name in selected or name in existing:continue
 matches=functions[name]
 if len(matches)!=1:raise ValueError(('ambiguous C callback',name))
 path,m=matches[0];selected[name]=(path,m)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',m[0]))&functions.keys())-selected.keys()-existing)
# Generate source only; original headers are isolated by a narrow type boundary.
lines=[];decls=[];records=[]
references=set(re.findall(r'\b\w+\b',''.join(m[0] for _,m in selected.values())))
for path in sorted({p for p,_ in selected.values()}):
 text=path.read_text()
 for inc in re.findall(r'^#include "([^"]+)"',text,re.M):
  if inc.startswith(('constants/','files/msgdata/')): lines.append('#include "'+inc+'"')
 for m in re.finditer(r'^static const [^;{}\n]+?\b(\w+)\s*(?:\[[^;{}]*?\])?\s*=\s*\{.*?^\};',text,re.M|re.S):
  if m[1] in references:lines.append(m[0])
text=(root/'src/save_wifi_history.c').read_text()
lines.append(re.search(r'^#define SAVE_WIFI_HISTORY_REGION_WIDTH.*$',text,re.M)[0])
lines.append(re.search(r'^struct SaveWiFiHistory \{.*?^\};',text,re.M|re.S)[0])
for name in ('SaveRankingsEntry','SaveRankings'):
 text=(root/'src/save_rankings.c').read_text()
 lines.append(re.search(r'^struct '+name+r' \{.*?^\};',text,re.M|re.S)[0])

for name,(path,m) in sorted(selected.items()):
 line=path.read_text()[:m.start()].count('\n')+1
 decls.append(m[0].split(' {\n')[0]+';')
 lines.extend([f'#line {line} "{path}"',m[0]])
 records.append(dict(function=name,source=str(path.relative_to(root)),sha256=hashlib.sha256(m[0].encode()).hexdigest()))
write_text_if_changed(out/'save_c_records_kernel.inc', '\n'.join(lines)+'\n')
write_text_if_changed(out/'save_c_records_decls.inc', '\n'.join(decls)+'\n')
write_text_if_changed(out/'save_c_records_provenance.json', json.dumps(dict(functions=records,reused=sorted(existing&set(re.findall(r'\(SAVE(?:SIZE|INIT)FN\)(\w+)',registry)))),indent=2)+'\n')
print('C save initializer closure:',len(selected),'functions')

write_text_if_changed(out/"save_c_types.inc", re.search(r"typedef struct CameraAngle \{.*?\} CameraAngle;",(root/"include/camera.h").read_text(),re.S)[0]+"\n")
charmap={v:int(k,16) for k,v in re.findall(r'^([0-9A-F]{4})=(.)$',(root/'tools/msgenc/charmap.txt').read_text(),re.M)}
messages=json.loads((root/'files/msgdata/msg/msg_0024.json').read_text())['messages']
boxes=[]
for index in range(6,24):
 text=messages[index]['en_US']
 if not isinstance(text,str) or len(text)>19:raise ValueError('Unexpected PC box name')
 boxes.append('{'+','.join(str(charmap[c]) for c in text)+',65535}')
write_text_if_changed(out/'save_box_names.inc', 'static const u16 save_box_names[18][20]={'+','.join(boxes)+'};\n')

# Preserve the original elapsed-time controller; only its hardware timer is hosted.
controller=(root/'src/unk_02018380.c').read_text()
controller=re.sub(r'^#include .*\n','',controller,flags=re.M)
write_text_if_changed(out/'save_play_time.inc', controller)
