"""Prepare unchanged follower C closure and original metadata; build-local only."""
from generated_files import write_text_if_changed
import csv,hashlib,json,pathlib,re,sys
from narc_tools import pack
root,out=map(pathlib.Path,sys.argv[1:]);path=root/'src/follow_mon.c';source=path.read_text()
pattern=r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{[^\n]*\n.*?^\}'
functions={}
for m in re.finditer(pattern,source,re.M|re.S):
 name=m[1]
 if name in functions:raise ValueError(('ambiguous follower definition',name))
 functions[name]=m
pending=['FollowMon_CreateMapObject','sub_02069E14','sub_02069E28','sub_02069E50','sub_02069E84','sub_02069EAC','sub_02069EC0','sub_02069ED4'];selected=set()
while pending:
 name=pending.pop()
 if name in selected:continue
 if name not in functions:raise ValueError(('missing follower definition',name))
 selected.add(name);pending.extend((set(re.findall(r'\b(\w+)\s*\(',functions[name][0]))&functions.keys())-selected)
lines=[];decls=[];records=[]
for table in ('sModelIndexLUT','sFormMaxLUT','sFemaleFlagLUT'):
 matches=re.findall(r'^static const u16 '+table+r'\[\] = \{.*?^\};',source,re.M|re.S)
 if len(matches)!=1:raise ValueError(('follower table',table))
 lines+=matches
for name in sorted(selected):
 m=functions[name];line=source[:m.start()].count('\n')+1
 decls.append(m[0].split(' {',1)[0]+';');lines.extend([f'#line {line} "{path}"',m[0]])
 records.append({'function':name,'source':'src/follow_mon.c','slice_sha256':hashlib.sha256(m[0].encode()).hexdigest()})
write_text_if_changed(out/'follow_public.inc', ''.join(d+'\n' for d in decls if not d.startswith('static ')))
write_text_if_changed(out/'follow_decls.inc', '\n'.join(decls)+'\n');write_text_if_changed(out/'follow_kernel.inc', '\n'.join(lines)+'\n')
schema=(root/'files/fielddata/tsurepoke/tp_param.csv.txt').read_text().splitlines()
if schema!=['index:skip:constants/follow_mon_idx.h','unk0:u8','too_tall:u8:bool','unk2_0:u8.4:bool','hover:u8.4:bool','unk3:u8']:raise ValueError('Follower parameter schema changed')
rows=list(csv.DictReader((root/'files/fielddata/tsurepoke/tp_param.csv').open()))
def number(v):
 if v in ('true','false'):return int(v=='true')
 return int(v,0)
pack(out/'resources/follow_params.narc',[bytes([number(r['unk0']),number(r['too_tall']),number(r['unk2_0'])|(number(r['hover'])<<4),number(r['unk3'])]) for r in rows])
inputs=['src/follow_mon.c','files/fielddata/tsurepoke/tp_param.csv','files/fielddata/tsurepoke/tp_param.csv.txt','include/constants/follow_mon_idx.h','include/constants/sprites.h','include/constants/species.h','include/constants/pokemon.h','include/follow_mon.h']
write_text_if_changed(out/'follow_provenance.json', json.dumps({'functions':records,'sources':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs},'parameter_records':len(rows)},indent=2)+'\n')
