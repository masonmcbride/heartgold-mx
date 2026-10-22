"""Bounded original contest closure and schema-checked original CSV resources."""
from generated_files import write_text_if_changed
import csv,hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}'
functions={}
source_inputs=('src/overlay_bug_contest.c','src/party.c','src/pokemon_storage_system.c','src/script_pokemon_util.c','src/pokemon.c')
for relative in source_inputs:
 path=root/relative
 for m in re.finditer(pattern,path.read_text(),re.M|re.S):
  if m[1] in functions:raise ValueError(('duplicate contest function',m[1]))
  functions[m[1]]=(path,m)
existing={'SaveArray_Party_Get','SaveArray_PCStorage_Get','Save_Pokedex_Get'}
for name in ('pokemon_decls.inc','script_kernel_decls.inc','save_c_records_decls.inc'):
 existing.update(re.findall(r'\b(\w+)\([^;{}]*\)\s*;',(out/name).read_text()))
roots='BugContest_New BugContest_Delete BugContest_Judge BugContest_GetEncounterSlot BugContest_ContestantIsRegistered GetIdxOfFirstAliveMonInParty_CrashIfNone'.split()
selected={};pending=roots[:]
while pending:
 name=pending.pop()
 if name in selected or name in existing or name not in functions:continue
 path,m=functions[name];selected[name]=(path,m)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',m[0]))&functions.keys())-selected.keys()-existing)
lines=[];decls=[];provenance=[]
party=(root/'src/party.c').read_text();lines.append(party[party.index('#define PARTY_ASSERT_SLOT'):party.index('\n\n',party.index('#define PARTY_ASSERT_SLOT'))])
for name,(path,m) in sorted(selected.items()):
 line=path.read_text()[:m.start()].count('\n')+1
 decls.append(m[0].split(' {\n')[0]+';');lines.extend([f'#line {line} "{path}"',m[0]])
 provenance.append(dict(function=name,source=str(path.relative_to(root)),sha256=hashlib.sha256(m[0].encode()).hexdigest()))
write_text_if_changed(out/'bug_contest_kernel.inc', '\n'.join(lines)+'\n');write_text_if_changed(out/'bug_contest_decls.inc', '\n'.join(decls)+'\n')
enc=(root/'include/wild_encounter.h').read_text();matches=re.findall(r'typedef struct EncounterSlot \{.*?\} EncounterSlot;',enc,re.S)
if len(matches)!=1:raise ValueError('EncounterSlot layout changed')
write_text_if_changed(out/'bug_contest_types.inc', matches[0]+'\n')
species=dict((k,int(v)) for k,v in re.findall(r'^#define (SPECIES_\w+)\s+(\d+)\s*$',(root/'include/constants/species.h').read_text(),re.M))
schemas={'mushi_encount':['species:u16:constants/species.h','lvlmin:u8','lvlmax:u8','rate:u8','score:u8','dummy:pad2'],'mushi_trainer':['national:u8','day:u8','species:u16:constants/species.h','score:u16','randmod:u16']}
data=[];inputs=list(source_inputs)+['include/wild_encounter.h','include/bug_contest_internal.h','include/constants/species.h','files/msgdata/msg/msg_0246.json']
for name,schema in schemas.items():
 base=root/'files/data/mushi'/name
 if base.with_suffix('.txt').read_text().splitlines()!=schema:raise ValueError(('Contest schema changed',name))
 rows=list(csv.DictReader(base.with_suffix('.csv').open()))
 if len(rows)!={'mushi_encount':40,'mushi_trainer':80}[name]:raise ValueError(('Contest row count',name))
 encoded=[]
 for row in rows:
  for column in schema:
   pieces=column.split(':');key,kind=pieces[:2]
   if kind=='pad2':encoded.extend([0,0]);continue
   value=species[row[key]] if key=='species' else int(row[key],0)
   width={'u8':1,'u16':2}[kind];encoded.extend(value.to_bytes(width,'little'))
 data.append('static const unsigned char '+name+'[]={'+','.join(map(str,encoded))+'};')
 inputs.extend([str(base.with_suffix(s).relative_to(root)) for s in ('.txt','.csv')])
source=(root/'src/overlay_bug_contest.c').read_text()
tables=re.findall(r'^const u16 sBugContestOpponentClasses\[\] = \{.*?^\};',source,re.M|re.S)
if len(tables)!=1:raise ValueError('Contest class table changed')
messages=json.loads((root/'files/msgdata/msg/msg_0246.json').read_text())['messages']
name_index=next(i for i,m in enumerate(messages) if m['id']=='msg_0246_00078')
write_text_if_changed(out/'bug_contest_format.inc', '#define msg_0246_00078 '+str(name_index)+'\n'+tables[0].replace('const u16','static const u16',1)+'\n')
write_text_if_changed(out/'bug_contest_resources.inc', '\n'.join(data)+'\n')
write_text_if_changed(out/'bug_contest_provenance.json', json.dumps(dict(functions=provenance,inputs={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs}),indent=2)+'\n')
print('Original Bug-Catching Contest closure:',len(selected),'functions')
