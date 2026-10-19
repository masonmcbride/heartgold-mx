"""Assemble whole original movement units and relocate a bounded guest image.
Unprovided imports receive named failure traps, never successful stubs.
"""
import hashlib,json,pathlib,re,struct,subprocess,sys
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

# Original Apricorn availability/type → sprite selector and its original LUT.
apricorn_path=root/'asm/unk_020632B0.s'
apricorn_matches=list(re.finditer(r'\tthumb_func_start sub_020640A4\n.*?\tthumb_func_end sub_020640A4',apricorn_path.read_text(),re.S))
if len(apricorn_matches)!=1:raise ValueError('Original Apricorn sprite selector changed')
sprite_source=(root/'asm/overlay_01_sprite_data.s').read_text()
tables=re.findall(r'ov01_02207274:\n((?:\t.word SPRITE_\w+\n)+)',sprite_source)
if len(tables)!=1:raise ValueError('Original Apricorn sprite table changed')
raw+='\n#include "constants/sprites.h"\n.text\n'+apricorn_matches[0][0]+'\n.rodata\n.public ov01_02207274\nov01_02207274:\n'+tables[0]+'\n'


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
 header=out/'msgdata/msg'/p.with_suffix('.h').name;header.parent.mkdir(parents=True,exist_ok=True)
 header.write_text(''.join(f'#define {m["id"]} {i}\n' for i,m in enumerate(json.loads(p.read_text())['messages'])))
source=out/'field_guest_input.S';source.write_text(raw)
text=subprocess.check_output([str(cc),'-E','-P','-DPM_ASM','-x','assembler-with-cpp','-I'+str(out),'-I'+str(root/'include'),'-I'+str(root/'files'),str(source)],text=True)
lines=['.syntax unified','.cpu arm946e-s','.thumb']
for line in text.splitlines():
 line=line.split(';')[0].strip()
 if not line or line.startswith(('#','.include','thumb_func_end')):continue
 if line.startswith('thumb_func_start'):
  lines += ['.global '+line.split()[1],'.thumb_func'];continue
 if line=='.rodata':line='.section .rodata\n.balign 4' # Original units have independent word-aligned sections.
 if line.startswith('.public'):line=line.replace('.public','.global',1)
 m=re.match(r'(add|sub|mov|lsl|lsr|asr|mul|adc|orr|neg|eor|and|bic|mvn|sbc|ror)\s+(.*)',line)
 if m:
  op,args=m.groups();regs=re.findall(r'\b(?:r\d+|sp|lr|pc|ip)\b',args)
  high=any(r in ('sp','lr','pc','ip') or r.startswith('r') and int(r[1:])>=8 for r in regs)
  if op not in ('add','sub','mov') or not high:line=op+'s '+args
 lines.append(line)
assembly=out/'field_guest.s';obj=out/'field_guest.o';assembly.write_text('\n'.join(lines)+'\n')
subprocess.run([str(cc),'-target','arm-none-eabi','-c',str(assembly),'-o',str(obj)],check=True)
data=obj.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'
shoff=struct.unpack_from('<I',data,32)[0];ents,count,names=struct.unpack_from('<HHH',data,46)
sections=[struct.unpack_from('<10I',data,shoff+i*ents) for i in range(count)]
def payload(s):return data[s[4]:s[4]+s[5]]
def string(table,offset):return table[offset:table.index(b'\0',offset)].decode()
strings=payload(sections[names]);image=bytearray();bases={};base=0x02000000
for i,s in enumerate(sections):
 if s[2]&2 and s[1]==1:
  image.extend(b'\0'*((-len(image))%max(s[8],4)));bases[i]=len(image);image.extend(payload(s))
text_index=next(i for i,s in enumerate(sections) if string(strings,s[0])=='.text');code_size=sections[text_index][5]
assert bases[text_index]==0, 'executable section must begin the image'
symtab=next(s for s in sections if s[1]==2);symstrings=payload(sections[symtab[6]]);symbols=[]
for pos in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
 name,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,pos)
 symbols.append((string(symstrings,name),value,index))
imports={name:0x02100000+4*i for i,name in enumerate(sorted({n for n,v,idx in symbols if idx==0 and n}))}
def address(sym):
 name,value,index=sym
 if index in bases:return base+bases[index]+value
 if index==0:return imports[name]+1
 if index==0xfff1:return value
 raise ValueError(('unmapped symbol',sym))
for section in sections:
 if section[1]!=9 or section[7] not in bases:continue
 for pos in range(section[4],section[4]+section[5],section[9]):
  offset,info=struct.unpack_from('<II',data,pos);location=bases[section[7]]+offset;kind=info&255;target=address(symbols[info>>8])
  if kind==2:struct.pack_into('<I',image,location,(target+struct.unpack_from('<I',image,location)[0])&0xffffffff)
  elif kind==10:
   hi,lo=struct.unpack_from('<HH',image,location);addend=((hi&2047)<<12)|((lo&2047)<<1)
   if addend&(1<<22):addend-=1<<23
   delta=(target&~1)-(base+location)+addend;assert delta%2==0 and -(1<<22)<=delta<(1<<22)
   struct.pack_into('<HH',image,location,0xf000|((delta>>12)&2047),0xf800|((delta>>1)&2047))
  else:raise ValueError(('unsupported relocation',kind))
header=['/* Original assembled bytes: generated build-local. */','static const unsigned char hg_field_guest_image[] = {',','.join(map(str,image)),'};',f'#define HG_FIELD_GUEST_CODE_SIZE {code_size}u']
for name,value,index in symbols:
 if index in bases and name and not name.startswith('$'):header.append(f'#define HG_FIELD_GUEST_{name} {address((name,value,index))&~1}u')
header += ['static const struct { unsigned address;const char *name; } hg_field_guest_imports[] = {']
header += [f'{{{a}u,"{n}"}},' for n,a in imports.items()];header+=['};']
(out/'field_guest.inc').write_text('\n'.join(header)+'\n')
(out/'field_guest_provenance.json').write_text(json.dumps({'sources':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources+message_sources},'function_slices':[{'source':str(effect_path),'function':'ov01_02200480','source_sha256':hashlib.sha256(effect_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(effect_slice.encode()).hexdigest()}],'interaction_slices':[{'source':str(surf_path),'function':name,'source_sha256':hashlib.sha256(surf_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()} for name,body in [('Field_PlayerCanSurfOnTile',surf_matches[0][0]),('MetatileBehavior_IsRockClimbInDirection',rock_matches[0][0])]],'image_sha256':hashlib.sha256(image).hexdigest(),'image_size':len(image),'code_size':code_size,'imports':imports},indent=2)+'\n')
print(f'Original field guest: {len(image)} bytes, {len(imports)} named import traps')

# Record the selector and LUT independently of whole-assembly coverage.
provenance_path=out/'field_guest_provenance.json'
provenance=json.loads(provenance_path.read_text())
provenance['apricorn_selector']={'source':str(apricorn_path),'function':'sub_020640A4','source_sha256':hashlib.sha256(apricorn_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(apricorn_matches[0][0].encode()).hexdigest(),'table_source':'asm/overlay_01_sprite_data.s','table_sha256':hashlib.sha256(tables[0].encode()).hexdigest()}
provenance_path.write_text(json.dumps(provenance,indent=2)+'\n')

provenance=json.loads(provenance_path.read_text())
provenance['brightness_controller']=[{'source':str(fade_path),'function':name,'source_sha256':hashlib.sha256(fade_path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()} for name,body in fade_slices]
provenance_path.write_text(json.dumps(provenance,indent=2)+'\n')
