"""Build-local original shop decisions and transaction functions; native UI stays explicit."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
from guest_assembly import assemble
root,out,cc=map(pathlib.Path,sys.argv[1:])
paths=['src/scrcmd_mart.c','src/overlay_03/shop_menu.c'];sources={p:(root/p).read_text() for p in paths}
functions={}
for p,s in sources.items():
 for m in re.finditer(r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{\n.*?^\}',s,re.M|re.S):
  if m[1] in functions:raise ValueError(('Ambiguous shop definition',m[1]))
  functions[m[1]]=(p,m)
provided=set('Mart_Init ov03_02258560 ov03_022586BC ov03_022582C0'.split())
pending='ScrCmd_MartBuy ScrCmd_MartSell ScrCmd_SpecialMartBuy ov03_02256BEC ov03_02256C2C ov03_02258120 MartData_PerformTransaction ov03_02257F24 ov03_022587E8'.split();selected={}
while pending:
 n=pending.pop()
 if n in selected or n in provided:continue
 p,m=functions[n];selected[n]=(p,m)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',m[0]))&functions.keys())-selected.keys()-provided)
# Resolve referenced original tables to a fixed point, rejecting ambiguity.
tables={}
for p,s in sources.items():
 for m in re.finditer(r'^const (?:u16\s*\*?\s*(\w+)\[\] = \{[^{}]*\};|struct \w+\s*\*?\s*(\w+)\[\] = \{.*?^\};)',s,re.M|re.S):
  name=m[1] or m[2]
  if name in tables:raise ValueError(('Ambiguous shop table',name))
  tables[name]=m[0]
refs=set(re.findall(r'\b\w+\b',''.join(m[0] for _,m in selected.values())));needed={}
while True:
 new=(refs&tables.keys())-needed.keys()
 if not new:break
 for n in new:needed[n]=tables[n];refs.update(re.findall(r'\b\w+\b',tables[n]))
helper=re.findall(r'^(?:static )?inline u16 ScriptGetVar\(.*?^\}',(root/'include/scrcmd.h').read_text(),re.M|re.S)
if len(helper)!=1:raise ValueError('Original script reader changed')
lines=[helper[0], '/* Unchanged original functions and tables; native modal renderer is separate. */']
lines.append(re.search(r'struct BadgeMartItems \{.*?^\};',sources[paths[0]],re.M|re.S)[0]);lines += [tables[n] for n in tables if n in needed]
decls=[];records=[]
for n,(p,m) in sorted(selected.items()):
 decls.append(m[0].split(' {\n')[0]+';');lines.extend([f'#line {sources[p][:m.start()].count(chr(10))+1} "{root/p}"',m[0]])
 records.append({'function':n,'source':p,'sha256':hashlib.sha256(m[0].encode()).hexdigest()})
write_text_if_changed(out/'mart_decls.inc', '\n'.join(decls)+'\n');write_text_if_changed(out/'mart_kernel.inc', '\n'.join(lines)+'\n')
header=(root/'include/overlay_03.h').read_text()
enums=[re.search(r'enum MartTypes \{.*?\};',header,re.S)[0],re.search(r'enum MartTaskStates \{.*?\};',sources[paths[1]],re.S)[0] if 'enum MartTaskStates' in sources[paths[1]] else re.search(r'enum MartTaskState\w* \{.*?\};',sources[paths[1]],re.S)[0]]
write_text_if_changed(out/'mart_types.inc', re.search(r'struct MartItem \{.*?\};',(root/'include/mart.h').read_text(),re.S)[0]+'\n'+'\n'.join(enums)+'\n')
sale=(root/'asm/overlay_15.s').read_text();match=re.findall(r'\tthumb_func_start ov15_021FD24C\n.*?\tthumb_func_end ov15_021FD24C',sale,re.S)
if len(match)!=1:raise ValueError('Original sale transaction changed')
image,code,imports=assemble('.text\n'+match[0],root,out,cc,'mart_sale_guest','HG_MART_SALE')
write_text_if_changed(out/'mart_provenance.json', json.dumps({'functions':records,'tables':list(needed),'sale_sha256':hashlib.sha256(match[0].encode()).hexdigest(),'sale_image_bytes':len(image),'sale_imports':imports},indent=2)+'\n')

commands=re.findall(r'^    (\w+),$',(root/'src/data/fieldmap/script_cmd_table.h').read_text(),re.M)
ops={name:commands.index(name) for name in ('ScrCmd_MartBuy','ScrCmd_MartSell','ScrCmd_SpecialMartBuy')}
write_text_if_changed(out/'mart_opcodes.inc', 'enum { HG_SCRIPT_MART_BUY='+str(ops['ScrCmd_MartBuy'])+', HG_SCRIPT_MART_SELL='+str(ops['ScrCmd_MartSell'])+', HG_SCRIPT_SPECIAL_MART='+str(ops['ScrCmd_SpecialMartBuy'])+' };\n')
# Shared delivery visibility queries use real saved gift data, not planted flags.
gift=(root/'src/mystery_gift.c').read_text();names=('MysteryGiftTagIsValid','SaveMysteryGift_GetByIdx','SaveMysteryGift_TagAtIndexIsValid','SaveMGDataPtr_GetFirstGiftIndex','SaveMGDataPtr_GetTagByIndex');defs=[];decls=[]
for name in names:
 matches=re.findall(r'^[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',gift,re.M|re.S)
 if len(matches)!=1:raise ValueError(('Original gift query changed',name))
 defs.append(matches[0]);decls.append(matches[0].split(' {\n')[0]+';')
write_text_if_changed(out/'delivery_queries.inc', '\n'.join(decls+defs)+'\n')
