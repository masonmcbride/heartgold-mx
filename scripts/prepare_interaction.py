"""Compile original event constants/script macros into build-local resources."""
import hashlib,json,pathlib,re,struct,subprocess,sys,shlex,concurrent.futures
root,out,cc=map(pathlib.Path,sys.argv[1:]);resources=out/'resources'
# Existing archive writer remains the sole writer used by these two tools.
from narc_tools import pack

from script_assembly import sections,assemble as assemble_script
def assemble(source,target):return assemble_script(root,out,cc,source,target)

folder=root/'files/fielddata/eventdata/zone_event'
inputs=[folder/name for name in re.findall(r"'([^']+\.json)'",(folder/'meson.build').read_text())]
tool=out/'jsonproc';tool_source=root/'tools/jsonproc/jsonproc.cpp'
signature=hashlib.sha256(str(cc).encode()+b''.join(p.read_bytes() for p in sorted(tool_source.parent.rglob('*')) if p.is_file())).hexdigest()
stamp=out/'jsonproc.sha256'
if not tool.is_file() or not stamp.is_file() or stamp.read_text()!=signature:
 subprocess.run([str(cc),'-x','c++','-std=c++17','-O1','-I'+str(tool_source.parent),'-lc++',str(tool_source),'-o',str(tool)],check=True)
 stamp.write_text(signature)

template=root/'files/fielddata/eventdata/zone_event.json.txt'
lines=[]
for index,path in enumerate(inputs):
 generated=out/'event_current.S'
 subprocess.run([str(tool.resolve()),str(path),str(template),str(generated)],check=True)
 rendered=generated.read_text().replace('.rodata','.section .rodata')
 # Put the label after the original template's section directive.
 section=rendered.index('.section .rodata')+len('.section .rodata')
 lines.extend([rendered[:section],f'.global hg_event_{index}',f'hg_event_{index}:',rendered[section:],f'.size hg_event_{index}, .-hg_event_{index}'])
source=out/'events.S';source.write_text('\n'.join(lines)+'\n')
data=assemble(source,out/'events.o');table,content,name=sections(data,True)
symtab=next(s for s in table if s[1]==2);strings=content(table[symtab[6]]);members={}
for offset in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
 n,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,offset)
 text=strings[n:strings.index(b'\0',n)].decode()
 if text.startswith('hg_event_'):
  member=content(table[index])[value:value+size];assert len(member)<2048
  # Undefined original constants make this member unsupported, never zero-fill
  # its references and present the result as original event data.
  bad=False
  for relocation in table:
   if relocation[1]==9 and relocation[7]==index:
    for position in range(relocation[4],relocation[4]+relocation[5],relocation[9]):
     location=struct.unpack_from('<I',data,position)[0]
     if value<=location<value+size:bad=True
  members[int(text[9:])]=b'' if bad else member
assert sorted(members)==list(range(len(inputs)))
pack(resources/'zone_event.narc',[members[i] for i in range(len(inputs))])
banks=[];headers=[];script_members={}
script_folder=root/'files/fielddata/script/scr_seq'
scripts=[script_folder/name for name in re.findall(r"'(scr_seq_[^']+\.s)'",(script_folder/'meson.build').read_text())]
if [int(p.stem.split('_')[2]) for p in scripts]!=list(range(len(scripts))):raise ValueError('Script manifest/member order changed')
# The original ordered table owns standard-script routing, including numeric
# thresholds. Reject unparsed rows instead of silently inventing a mapping.
manager=root/'src/script_manager.c'
mapping=re.search(r'const struct ScriptBankMapping sScriptBankMapping\[(\d+)\] = \{(.*?)\n\};',manager.read_text(),re.S)
if not mapping:raise ValueError('Original standard-script mapping changed')
body=re.sub(r'//[^\n]*','',mapping[2])
rows=re.findall(r'\{\s*(_std_\w+|\d+),\s*scr_seq_(\d+)_bin,\s*msg_(\d+)\s*\},',body)
if len(rows)!=int(mapping[1]) or re.sub(r'\{\s*(_std_\w+|\d+),\s*scr_seq_(\d+)_bin,\s*msg_(\d+)\s*\},','',body).strip():
 raise ValueError('Ambiguous original standard-script mapping')
(out/'script_standard_banks.inc').write_text('static const struct { unsigned first,bank,messages; } standard_script_banks[] = {\n'+''.join(f'{{{base},{int(bank)},{int(msg)}}},\n' for base,bank,msg in rows)+'};\n')
def compiled(script):
 bank=int(script.stem.split('_')[2]);target=out/f'field_script_{bank:04}.o'
 try:
  data=assemble(script,target);table,content,name=sections(data)
  return content(next(s for s in table if name(s[0])=='.text')),None
 except (ValueError,AssertionError,StopIteration) as error:
  log=out/f'field_script_{bank:04}.error.txt';log.write_text(str(error)+'\n')
  return None,str(log)
with concurrent.futures.ThreadPoolExecutor(max_workers=8) as workers:
 compiled_scripts=list(workers.map(compiled,scripts))
unsupported=[]
for script,(bytecode,error) in zip(scripts,compiled_scripts):
 source=script.read_text();bank=int(script.stem.split('_')[2])
 if error:
  unsupported.append({'bank':bank,'source':str(script),'source_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),'diagnostic':error});continue
 if script.stem.endswith('_hdr'):
  script_members[bank]=bytecode
  headers.append({'bank':bank,'source':str(script),'source_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),'bytes':len(bytecode)})
  continue
 message_names=re.findall(r'#include "msgdata/msg/(msg_\d+(?:_[^"]+)?)\.h"',source)
 if not message_names:raise ValueError(('Missing script message bank',script))
 group=int(message_names[0].split('_')[1])
 script_members[bank]=bytecode
 (resources/f'field_script_{bank:04}.bin').write_bytes(bytecode)
 banks.append({'bank':bank,'entries':len(re.findall(r'^\s*ScrDef\s+\w+',source,re.M)),
  'script_source':str(script),'script_source_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),
  'script_sha256':hashlib.sha256(bytecode).hexdigest(),'bytes':len(bytecode),'message_bank':group})
(out/'field_script_banks.inc').write_text('static const struct { unsigned bank,entries; } field_script_banks[] = {\n'+''.join(f'{{{b["bank"]},{b["entries"]}}},\n' for b in banks)+'};\n')
pack(resources/'field_scripts.narc',[script_members.get(i,b'') for i in range(len(scripts))])
# Keep original ScriptContext declaration/API without unrelated SDK headers.
text=(root/'include/script.h').read_text()
context=re.search(r'struct ScriptContext \{.*?\n\};',text,re.S)[0]
prototypes=text[text.index('void InitScriptContext'):text.index('\n#endif',text.index('void InitScriptContext'))]
(out/'script_types.inc').write_text('typedef struct ScriptContext ScriptContext;\ntypedef BOOL (*ScrCmdFunc)(ScriptContext *);\n'+context+'\n'+prototypes+'\n')
(out/'interaction_provenance.json').write_text(json.dumps({'event_members':len(inputs),'unsupported_event_members':[i for i,m in members.items() if not m],'event_sources':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'script_banks':banks,'header_banks':headers,'unsupported_script_banks':unsupported,'script_manifest':str(script_folder/'meson.build'),'standard_mapping':{'source':str(manager),'source_sha256':hashlib.sha256(manager.read_bytes()).hexdigest(),'rows':[{'first':base,'bank':int(bank),'messages':int(msg)} for base,bank,msg in rows]}},indent=2)+'\n')
print(f'Original event data: {len(inputs)} members; {len(banks)} script banks; {len(headers)} headers; {len(unsupported)} unsupported banks; {len(rows)} original standard routes')
