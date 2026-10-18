"""Compile original event constants/script macros into build-local resources."""
import hashlib,json,pathlib,re,struct,subprocess,sys
root,out,cc=map(pathlib.Path,sys.argv[1:]);resources=out/'resources'
# Existing archive writer remains the sole writer used by these two tools.
from narc_tools import pack

def sections(data,allow_relocations=False):
 assert data[:6]==b'\x7fELF\x01\x01'
 offset=struct.unpack_from('<I',data,32)[0];size,count,names=struct.unpack_from('<HHH',data,46)
 table=[struct.unpack_from('<10I',data,offset+i*size) for i in range(count)]
 def content(s):return data[s[4]:s[4]+s[5]]
 strings=content(table[names])
 def name(i):return strings[i:strings.index(b'\0',i)].decode()
 assert allow_relocations or not any(s[1] in (4,9) and s[5] for s in table), 'unresolved data relocation'
 return table,content,name

def assemble(source,target):
 subprocess.run([str(cc),'-target','arm-none-eabi','-DPM_ASM','-x','assembler-with-cpp',
  '-I'+str(root/'include'),'-I'+str(root/'asm'),'-I'+str(root/'files'),'-I'+str(out),'-c',str(source),'-o',str(target)],check=True)
 return target.read_bytes()

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
scripts=sorted((root/'files/fielddata/script/scr_seq').glob('scr_seq_*_T20*.s'))
scripts.append(root/'files/fielddata/script/scr_seq/scr_seq_0149.s')
for script in scripts:
 source=script.read_text();bank=int(script.stem.split('_')[2])
 if script.stem.endswith('_hdr'):
  data=assemble(script,out/f'field_header_{bank:04}.o');table,content,name=sections(data)
  bytecode=content(next(s for s in table if name(s[0])=='.text'));script_members[bank]=bytecode
  headers.append({'bank':bank,'source':str(script),'source_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),'bytes':len(bytecode)})
  continue
 message_name=re.search(r'#include "msgdata/msg/(msg_\d+(?:_[^"]+)?)\.h"',source)[1]
 messages=json.loads((root/'files/msgdata/msg'/f'{message_name}.json').read_text())['messages']
 group=int(message_name.split('_')[1]);p=out/'msgdata/msg'/f'{message_name}.h';p.parent.mkdir(parents=True,exist_ok=True)
 p.write_text(''.join(f'#define {m["id"]} {i}\n' for i,m in enumerate(messages)))
 texts=[''.join(m.get('en_US','')) if isinstance(m.get('en_US',''),list) else m.get('en_US','') for m in messages]
 pack(resources/f'msg_{group:04}.narc',[t.encode()+b'\0' for t in texts])
 data=assemble(script,out/f'field_script_{bank:04}.o');table,content,name=sections(data)
 bytecode=content(next(s for s in table if name(s[0])=='.text'))
 script_members[bank]=bytecode
 (resources/f'field_script_{bank:04}.bin').write_bytes(bytecode)
 banks.append({'bank':bank,'entries':len(re.findall(r'^\s*ScrDef\s+\w+',source,re.M)),
  'script_source':str(script),'script_source_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),
  'script_sha256':hashlib.sha256(bytecode).hexdigest(),'bytes':len(bytecode),'message_bank':group})
(out/'field_script_banks.inc').write_text('static const struct { unsigned bank,entries; } field_script_banks[] = {\n'+''.join(f'{{{b["bank"]},{b["entries"]}}},\n' for b in banks)+'};\n')
pack(resources/'field_scripts.narc',[script_members.get(i,b'') for i in range(max(script_members)+1)])
# Keep original ScriptContext declaration/API without unrelated SDK headers.
text=(root/'include/script.h').read_text()
context=re.search(r'struct ScriptContext \{.*?\n\};',text,re.S)[0]
prototypes=text[text.index('void InitScriptContext'):text.index('\n#endif',text.index('void InitScriptContext'))]
(out/'script_types.inc').write_text('typedef struct ScriptContext ScriptContext;\ntypedef BOOL (*ScrCmdFunc)(ScriptContext *);\n'+context+'\n'+prototypes+'\n')
(out/'interaction_provenance.json').write_text(json.dumps({'event_members':len(inputs),'unsupported_event_members':[i for i,m in members.items() if not m],'event_sources':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'script_banks':banks,'header_banks':headers},indent=2)+'\n')
print(f'Original event data: {len(inputs)} members; {len(banks)} local script banks')
