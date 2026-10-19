"""Build-local unchanged Pokemon function closure and original resource records."""
import csv,hashlib,json,pathlib,re,struct,subprocess,sys
from narc_tools import pack
root,out,cc=map(pathlib.Path,sys.argv[1:])
roots={
 'src/pokemon.c':['CreateMon','GetMonData','SetMonData','ZeroMonData','MonDecryptSegment','CalcMonChecksum','sub_020720FC','MonIsShiny'],
 'src/script_pokemon_util.c':['GetIdxOfFirstPartyMonWithMove'],
 'src/trainer_memo.c':['BoxMonSetTrainerMemo'],
 'src/pokedex.c':['Save_Pokedex_Init','Pokedex_SetMonCaughtFlag','Pokedex_CheckMonCaughtFlag','Pokedex_Enable','Pokedex_IsEnabled'],
 'src/unk_02091278.c':['LanguageToDexFlag'],
 'src/player_data.c':['PlayerProfile_SetVersion','PlayerProfile_SetMoney','PlayerProfile_GetMoney','PlayerProfile_Init','PlayerProfile_GetTrainerGender','PlayerProfile_TestBadgeFlag','PlayerProfile_GetNamePtr','PlayerProfile_GetTrainerID','PlayerProfile_GetPlayerName_NewString'],
 'src/use_item_on_mon.c':['HealParty'],
 'src/sav_system_info.c':['Save_SysInfo_SetMysteryGiftActive','Save_SysInfo_GetMysteryGiftActive'],
 'src/options.c':['Options_Init'],
 'src/party.c':['Party_InitWithMaxSize','Party_AddMon','Party_GetMonByIndex'],
 'src/pm_string.c':['String_New','String_Delete','String_Copy','CopyStringToU16Array','CopyU16ArrayToString'],
 'src/string_util.c':['StringNotEqual','StringFillEOS'],
 'src/mail.c':['Mail_New','Mail_Copy'],
 'src/mail_message.c':['MailMsg_Init'],
 'src/seal_case.c':['CopyCapsule'],
 'src/move.c':['GetMoveAttr','GetMoveMaxPP'],
}
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}'
functions={};sources={}
for path in roots:
 source=(root/path).read_text();sources[path]=source
 for m in re.finditer(pattern,source,re.M|re.S):
  name=m.group(1)
  if name in functions:raise ValueError(('ambiguous definition',name))
  functions[name]=(path,m)
selected=set();pending=[n for ns in roots.values() for n in ns]
while pending:
 name=pending.pop()
 if name in selected:continue
 if name not in functions:raise ValueError(('missing definition',name))
 selected.add(name)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',functions[name][1][0]))&functions.keys())-selected)
header=(root/'include/pokemon.h').read_text()
(out/'pokemon_macros.inc').write_text(header[header.index('#define LEVEL_UP_LEARNSET_END'):header.index('#define NUM_BANNED')])
pokedex_header=(root/'include/pokedex.h').read_text()
record_lines=[m[0] for m in re.finditer(r'^#define (?:ROUND_UP|CEILDIV|NUM_DEX_FLAG_WORDS).*$',pokedex_header,re.M)]
record_lines.append(re.search(r'typedef struct Pokedex \{.*?\} Pokedex;',pokedex_header,re.S)[0])
record_lines.append('typedef struct GameStats GameStats;')
record_lines.append(re.search(r'^struct GameStats \{.*?^\};',(root/'src/game_stats.c').read_text(),re.M|re.S)[0])
sys_info=(root/'include/sav_system_info.h').read_text()
record_lines.append('typedef struct { u32 hour,minute,second; } RTCTime;')
sys_records=re.findall(r'typedef struct SysInfo(?:_RTC)? \{.*?\} SysInfo(?:_RTC)?;',sys_info,re.S)
if len(sys_records)!=2:raise ValueError('Original system-information records changed')
record_lines.extend(sys_records)
(out/'pokemon_records.inc').write_text('\n'.join(record_lines)+'\n')
# Original macro environments and stat table remain verbatim.
lines=[]
dex=sources['src/pokedex.c'];lines.append(dex[dex.index('#define POKEDEX_MAGIC'):dex.index('static void Pokedex_InitDeoxys')])
lines.append('#define SETMETDATEPARAM_EGG 0\n#define SETMETDATEPARAM_MON 1')
lines.append(re.search(r'^static const u8 sDexFlagToLanguage.*?^\};',sources['src/unk_02091278.c'],re.M|re.S)[0])
poke=sources['src/pokemon.c']
lines.append(poke[poke.index('#define ENCRY_ARGS_PTY'):poke.index('static const s8 sFlavor')])
lines.append(re.search(r'^#define VALUE\(type\).*$',poke,re.M)[0])
lines.append(re.search(r'^const s8 gNatureStatMods.*?^\};',poke,re.M|re.S)[0])
party=sources['src/party.c'];lines.append(party[party.index('#define PARTY_ASSERT_SLOT'):party.index('\n\n',party.index('#define PARTY_ASSERT_SLOT'))])
strings=sources['src/pm_string.c'];lines.append(strings[strings.index('#define STRING_MAGIC'):strings.index('String *String_New')])
struct_string=re.search(r'typedef struct String \{.*?\} String;', (root/'include/pm_string.h').read_text(),re.S)[0]
(out/'pokemon_string_type.inc').write_text(struct_string+'\n')
decls=[];records=[]
for name in sorted(selected):
 path,m=functions[name];line=sources[path][:m.start()].count('\n')+1
 decls.append(m[0].split(' {\n')[0]+';')
 lines.extend([f'#line {line} "{root/path}"',m[0]])
 records.append({'source':path,'function':name,'line':line,'slice_sha256':hashlib.sha256(m[0].encode()).hexdigest()})
(out/'pokemon_kernel.inc').write_text('\n'.join(lines)+'\n')
(out/'pokemon_decls.inc').write_text('\n'.join(decls)+'\n')
provenance={'functions':records,'sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in roots}}
# Use the upstream template and C struct, not a second personal-data packer.
personal=root/'files/poketool/personal/personal.json'
generated=out/'pokemon_personal.c'
subprocess.run([str((out/'jsonproc').resolve()),str(personal),str(personal.with_suffix('.json.txt')),str(generated)],check=True)
generated.write_text('#include <stdio.h>\n'+generated.read_text()+'\nint main(void){return fwrite(__data,sizeof(__data),1,stdout)!=1;}\n')
exe=out/'pokemon_personal'
subprocess.run([str(cc),'-I'+str(pathlib.Path(__file__).resolve().parent.parent/'src/compat/include'),'-I'+str(root/'include'),str(generated),'-o',str(exe)],check=True)
data=subprocess.check_output([str(exe.resolve())]);count=len(json.loads(personal.read_text())['baseStats'])
if len(data)!=44*count:raise ValueError('Personal record size changed')
pack(out/'resources/personal.narc',[data[i*44:(i+1)*44] for i in range(count)])
rows=list(csv.DictReader((root/'files/poketool/personal/growtbl.csv').open()))
pack(out/'resources/growth.narc',[struct.pack('<101I',*(int(row[f'lv{i:03}']) for i in range(101))) for row in rows])
for source,target in [('files/poketool/personal/wotbl.narc','learnsets.narc'),('files/poketool/waza/waza_tbl.narc','moves.narc')]:
 (out/'resources'/target).write_bytes((root/source).read_bytes())
# Species names retain the original character encoding for original String C.
charmap={v:int(k,16) for k,v in re.findall(r'^([0-9A-F]{4})=(.)$',(root/'tools/msgenc/charmap.txt').read_text(),re.M)}
names=json.loads((root/'files/msgdata/msg/msg_0237.json').read_text())['messages']
encoded=[]
for name in names:
 text=name['en_US'];text=''.join(text) if isinstance(text,list) else text
 codes=[charmap[c] for c in text]
 if len(codes)>10:raise ValueError(('species name too long',text))
 encoded.append('{'+','.join(map(str,codes+[65535]*(11-len(codes))))+'}')
(out/'pokemon_names.inc').write_text('static const u16 species_names[][11]={'+','.join(encoded)+'};\n'+
 'static const char *const species_utf8[]={'+','.join(json.dumps(''.join(m['en_US']) if isinstance(m['en_US'],list) else m['en_US'],ensure_ascii=True) for m in names)+'};\n')
print('Original Pokemon closure:',len(selected),'functions')
inputs=['include/pokemon.h','include/sav_system_info.h','include/pm_string.h','include/struct_defs/personal_narc_structs.h','include/constants/pokemon.h','include/constants/abilities.h','include/constants/items.h','include/constants/moves.h','tools/msgenc/charmap.txt','files/poketool/personal/personal.json','files/poketool/personal/personal.json.txt','files/poketool/personal/growtbl.csv','files/poketool/personal/wotbl.narc','files/poketool/waza/waza_tbl.narc','files/msgdata/msg/msg_0237.json']
provenance['resource_and_declaration_inputs']={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs}
provenance['limitations']=['Function closure and symbol presence do not establish branch execution.','Original starter task, encounter metadata, Pokedex updates, choice UI and follower initialization are not supplied by this kernel.']
(out/'pokemon_provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
