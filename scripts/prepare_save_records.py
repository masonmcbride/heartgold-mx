"""Generate original save registry and assembled record callbacks, build-local only."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,struct,subprocess,sys
from audit_save import tables
root,out,cc=map(pathlib.Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
normal,extra=tables((root/'src/save_arrays.c').read_text())
if (len(normal),len(extra))!=(42,6):raise ValueError('save registry changed')
callbacks={n for r in normal+extra for n in r[2:]}
# Retain coherent original assembly units including their tables/BSS. This avoids
# fragile manual initializer slices and keeps internal helper calls original.
sources=[];asm_callbacks=set()
for p in sorted((root/'asm').glob('*.s')):
 text=p.read_text();defined=set(re.findall(r'^\s*thumb_func_start (\w+)',text,re.M))
 if defined&callbacks:sources.append(p);asm_callbacks|=defined&callbacks
sources.extend(root/p for p in ('asm/unk_0202D230.s','asm/unk_02066EDC.s'))
raw='\n'.join(p.read_text() for p in sources)
source=out/'save_records_input.S';write_text_if_changed(source, raw)
pre=subprocess.check_output([str(cc),'-E','-P','-DPM_ASM','-x','assembler-with-cpp','-I'+str(out),'-I'+str(root/'include'),'-I'+str(root/'files'),str(source)],text=True)
lines=['.syntax unified','.cpu arm946e-s','.thumb']
for line in pre.splitlines():
 line=line.split(';')[0].strip()
 if not line or line.startswith(('#','.include','thumb_func_end')):continue
 if line.startswith('thumb_func_start'):lines+=['.global '+line.split()[1],'.thumb_func'];continue
 if line=='.rodata':line='.section .rodata\n.balign 4'
 if line.startswith('.public'):line=line.replace('.public','.global',1)
 m=re.match(r'(add|sub|mov|lsl|lsr|asr|mul|adc|orr|neg|eor|and|bic|mvn|sbc|ror)\s+(.*)',line)
 if m:
  op,args=m.groups();regs=re.findall(r'\b(?:r\d+|sp|lr|pc|ip)\b',args)
  high=any(r in ('sp','lr','pc','ip') or r.startswith('r') and int(r[1:])>=8 for r in regs)
  if op not in ('add','sub','mov') or not high:line=op+'s '+args
 lines.append(line)
assembly=out/'save_records_guest.s';obj=out/'save_records_guest.o';write_text_if_changed(assembly, '\n'.join(lines)+'\n')
subprocess.run([str(cc),'-target','arm-none-eabi','-c',str(assembly),'-o',str(obj)],check=True)
data=obj.read_bytes();assert data[:6]==b'\x7fELF\x01\x01'
shoff=struct.unpack_from('<I',data,32)[0];ents,count,names=struct.unpack_from('<HHH',data,46)
sections=[struct.unpack_from('<10I',data,shoff+i*ents) for i in range(count)]
def payload(s):return data[s[4]:s[4]+s[5]]
def string(table,offset):return table[offset:table.index(b'\0',offset)].decode()
strings=payload(sections[names]);image=bytearray();bases={};base=0x02000000
for i,s in enumerate(sections):
 if s[2]&2 and s[1] in (1,8):
  image.extend(b'\0'*((-len(image))%max(s[8],4)));bases[i]=len(image);image.extend(payload(s) if s[1]==1 else bytes(s[5]))
text_index=next(i for i,s in enumerate(sections) if string(strings,s[0])=='.text');code_size=sections[text_index][5]
assert bases[text_index]==0
symtab=next(s for s in sections if s[1]==2);symstrings=payload(sections[symtab[6]]);symbols=[]
for pos in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
 name,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,pos);symbols.append((string(symstrings,name),value,index))
imports={n:0x02100000+4*i for i,n in enumerate(sorted({n for n,v,idx in symbols if idx==0 and n}))}
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
header=['static const unsigned char hg_save_guest_image[]={',','.join(map(str,image)),'};',f'#define HG_SAVE_GUEST_CODE_SIZE {code_size}u']
addresses={n:address((n,v,i))&~1 for n,v,i in symbols if i in bases and n and not n.startswith('$')}
for n in sorted(asm_callbacks|{'Save_Frontier_Load','Save_Frontier_Commit','sub_0202C6FC','sub_0202C714','sub_0202C78C','sub_0202C7DC','sub_02031CCC','sub_020674BC','sub_0206759C','Save_FriendGroup_Get','Save_ApricornBox_Get'}):header.append(f'#define HG_SAVE_GUEST_{n} {addresses[n]}u')
header+=['static const struct { unsigned address;const char *name; } hg_save_guest_imports[]={']+[f'{{{a}u,"{n}"}},' for n,a in imports.items()]+['};']
write_text_if_changed(out/'save_records_guest.inc', '\n'.join(header)+'\n')
# Preserve exact native callback prototypes, then adapt void* only through C
# wrappers. No incompatible function pointers or invented callback types.
prototypes={}
for p in sorted((root/'src').rglob('*.c')):
 text=p.read_text()
 for m in re.finditer(r'^([^\n;{}]*\b([A-Za-z_]\w*)\([^;{}]*\)) \{',text,re.M):
  if m[2] in callbacks-asm_callbacks:
   if m[2] in prototypes:raise ValueError(('duplicate callback definition',m[2]))
   prototypes[m[2]]=m[1]
if set(prototypes)!=callbacks-asm_callbacks:raise ValueError(('missing native callback definitions',callbacks-asm_callbacks-set(prototypes)))
registry=[]
for n in sorted(callbacks):
 if n in asm_callbacks:
  if any(r[2]==n for r in normal+extra):registry.append(f'static u32 hg_registry_{n}(void) {{ return save_guest_size(HG_SAVE_GUEST_{n}); }}')
  else:
   size=next(r[2] for r in normal+extra if r[3]==n)
   registry.append(f'static void hg_registry_{n}(void *p) {{ save_guest_init(HG_SAVE_GUEST_{n},HG_SAVE_GUEST_{size},p); }}')
 else:
  registry.append(prototypes[n]+';')
  if any(r[2]==n for r in normal+extra):registry.append(f'static u32 hg_registry_{n}(void) {{ return {n}(); }}')
  else:registry.append(f'static void hg_registry_{n}(void *p) {{ {n}(p); }}')
for name,rows,typ in [('gSaveChunkHeaders',normal,'SaveChunkHeader'),('gExtraSaveChunkHeaders',extra,'ExtraSaveChunkHeader')]:
 registry.append(f'const struct {typ} {name}[]={{')
 for ident,page,size,init in rows:registry.append('{'+','.join([ident,page,'hg_registry_'+size,'hg_registry_'+init])+'},')
 registry+=['};',f'const int '+('gNumSaveChunkHeaders' if name=='gSaveChunkHeaders' else 'gNumExtraSaveChunkHeaders')+f'={len(rows)};']
write_text_if_changed(out/'save_records_registry.inc', '\n'.join(registry)+'\n')
write_text_if_changed(out/'save_records_provenance.json', json.dumps({'sources':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources},'registry':normal,'extra_registry':extra,'assembly_callbacks':sorted(asm_callbacks),'imports':imports,'image_bytes':len(image),'code_bytes':code_size},indent=2)+'\n')
print(f'Original save records: {len(asm_callbacks)} assembly callbacks, {len(image)} bytes, {len(imports)} explicit import boundaries')
