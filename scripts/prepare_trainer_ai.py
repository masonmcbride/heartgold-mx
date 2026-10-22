"""Assemble original trainer AI for the existing bounded Thumb engine.
Generated records translate explicit guest values, never native pointers.
"""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,struct,subprocess,sys
root,out,cc=map(pathlib.Path,sys.argv[1:]);out.mkdir(exist_ok=True,parents=True)
source=root/'asm/overlay_10_trainer_ai.s';raw=source.read_text()
# Forced/voluntary enemy replacement shares the existing AI context boundary.
# Keep the three original selectors unchanged in the same bounded guest image.
replacement_source=root/'asm/overlay_12_battle_controller_opponent.s'
replacement_raw=replacement_source.read_text();replacement_names=('ov12_02258BA0','ov12_02258800','ov12_02258BB4');replacement_slices=[]
for function in replacement_names:
 matches=re.findall(r'\tthumb_func_start '+function+r'\n.*?\tthumb_func_end '+function,replacement_raw,re.S)
 if len(matches)!=1:raise ValueError(('Ambiguous original replacement selector',function))
 replacement_slices.append(matches[0])
raw+='\n.text\n'+'\n'.join(replacement_slices)
lines=['.syntax unified','.cpu arm946e-s','.thumb']
for line in raw.splitlines():
 line=line.split(';')[0].strip()
 if not line or line.startswith(('.include','thumb_func_end')):continue
 if line.startswith('thumb_func_start'):lines+=['.global '+line.split()[1],'.thumb_func'];continue
 if line=='.rodata':line='.section .rodata\n.balign 4'
 if line.startswith('.public'):line=line.replace('.public','.global',1)
 m=re.match(r'(add|sub|mov|lsl|lsr|asr|mul|adc|orr|neg|eor|and|bic|mvn|sbc|ror)\s+(.*)',line)
 if m:
  op,args=m.groups();regs=re.findall(r'\b(?:r\d+|sp|lr|pc|ip)\b',args)
  high=any(r in ('sp','lr','pc','ip') or r.startswith('r') and int(r[1:])>=8 for r in regs)
  if op not in ('add','sub','mov') or not high:line=op+'s '+args
 lines.append(line)
assembly=out/'trainer_ai_guest.s';obj=out/'trainer_ai_guest.o';write_text_if_changed(assembly, '\n'.join(lines)+'\n')
subprocess.run([str(cc),'-target','arm-none-eabi','-c',str(assembly),'-o',str(obj)],check=True)
from script_assembly import sections
data=obj.read_bytes();table,content,name=sections(data,True)
image=bytearray();bases={};base=0x02000000
for i,s in enumerate(table):
 if s[1]==1 and s[2]&2:
  image.extend(b'\0'*((-len(image))%max(s[8],1)));bases[i]=len(image);image.extend(content(s))
code_size=next(s[5] for s in table if name(s[0])=='.text')
symtab=next(s for s in table if s[1]==2);strings=content(table[symtab[6]]);symbols=[]
for pos in range(symtab[4],symtab[4]+symtab[5],symtab[9]):
 n,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,pos)
 symbols.append((strings[n:strings.index(b'\0',n)].decode(),value,index))
imports={n:0x02100000+4*i for i,n in enumerate(sorted({n for n,v,idx in symbols if idx==0 and n}))}
def address(sym):
 n,value,index=sym
 if index in bases:return base+bases[index]+value
 if not index:return imports[n]+1
 if index==0xfff1:return value
 raise ValueError(('unmapped AI symbol',sym))
for s in table:
 if s[1]!=9 or s[7] not in bases:continue
 for pos in range(s[4],s[4]+s[5],s[9]):
  offset,info=struct.unpack_from('<II',data,pos);loc=bases[s[7]]+offset;kind=info&255;target=address(symbols[info>>8])
  if kind==2:struct.pack_into('<I',image,loc,(target+struct.unpack_from('<I',image,loc)[0])&0xffffffff)
  elif kind==10:
   hi,lo=struct.unpack_from('<HH',image,loc);add=((hi&2047)<<12)|((lo&2047)<<1)
   if add&(1<<22):add-=1<<23
   delta=(target&~1)-(base+loc)+add
   if delta%2 or not -(1<<22)<=delta<(1<<22):raise ValueError('AI call relocation overflow')
   struct.pack_into('<HH',image,loc,0xf000|((delta>>12)&2047),0xf800|((delta>>1)&2047))
  else:raise ValueError(('Unsupported AI relocation',kind))
header=['static const unsigned char hg_ai_image[]={',','.join(map(str,image)),'};',f'#define HG_AI_CODE_SIZE {code_size}u']
for n,v,idx in symbols:
 if idx in bases and n and not n.startswith('$'):header.append(f'#define HG_AI_{n} {address((n,v,idx))&~1}u')
header+=['static const struct {unsigned address;const char *name;} hg_ai_imports[]={']+[f'{{{a}u,"{n}"}},' for n,a in imports.items()]+['};']
write_text_if_changed(out/'trainer_ai_guest.inc', '\n'.join(header)+'\n')
h=(root/'include/battle/battle.h').read_text();records=[];copies=[]
for typ in ('TrainerAIData','BattleContext'):
 match=re.search(r'typedef struct '+typ+r' \{(.*?)\} '+typ+r';',h,re.S)
 if not match:raise ValueError(('Missing original AI record',typ))
 body=re.sub(r'//[^\n]*','',match[1]);guest=re.sub(r'\bTrainerAIData\b','HGAITrainerAIData',body)
 guest=re.sub(r'\b(?:GetterWork|PokemonStats|ItemData|u32|u8)\s*\*\s*(\w+)\s*;',r'u32 \1;',guest)
 if '*' in guest:raise ValueError(('Unmapped guest pointer',typ))
 records.append(f'typedef struct HGAI{typ} {{'+guest+f'}} HGAI{typ};')
 for statement in body.split(';'):
  statement=statement.strip()
  if not statement:continue
  if ':' in statement:
   if typ!='BattleContext' or not re.fullmatch(r'u32\s+(?:battleContinueFlag\s*:\s*1|unused\s*:\s*31)',statement):raise ValueError(('Unmapped AI bitfield',statement))
   continue # Validated tail bits are transferred explicitly at the host boundary.
  m=re.fullmatch(r'[\w ]+\s+\*?\s*(\w+)(?:\[[^]]+\])*',statement)
  if not m:raise ValueError(('Ambiguous AI record field',statement))
  field=m[1]
  if '*' not in statement and field!='trainerAIData':copies.append(f'AI_FIELD({typ},{field});')
records+=['_Static_assert(sizeof(HGAITrainerAIData)==0x1de0,"guest AI record");','_Static_assert(offsetof(HGAIBattleContext,trainerAIData)==0x354,"guest AI offset");','_Static_assert(offsetof(HGAIBattleContext,battleMons)==0x2d40,"guest mon offset");','_Static_assert(sizeof(BattleMon)==0xc0,"guest battle mon stride");']
write_text_if_changed(out/'trainer_ai_records.inc', '\n'.join(records)+'\n')
write_text_if_changed(out/'trainer_ai_copy.inc', '\n'.join(copies)+'\n')
write_text_if_changed(out/'trainer_ai_provenance.json', json.dumps({'source':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'replacement_source':str(replacement_source),'replacement_sha256':hashlib.sha256(replacement_source.read_bytes()).hexdigest(),'replacement_functions':replacement_names,'image_bytes':len(image),'imports':imports},indent=2)+'\n')
print(f'Original trainer AI: {len(image)} guest bytes, {len(imports)} named boundaries')
