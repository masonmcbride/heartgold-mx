"""Shared bounded ARM/Thumb assembly and relocation; unknown imports remain traps."""
from generated_files import write_text_if_changed
import re,struct,subprocess

def assemble(raw,root,out,cc,stem,prefix):
    source=out/(stem+'_input.S');write_text_if_changed(source, raw)
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
    assembly=out/(stem+'.s');obj=out/(stem+'.o');write_text_if_changed(assembly, '\n'.join(lines)+'\n')
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
    header=['/* Original assembled bytes: generated build-local. */',f'static const unsigned char {prefix.lower()}_image[] = {{',','.join(map(str,image)),'};',f'#define {prefix}_CODE_SIZE {code_size}u']
    for name,value,index in symbols:
     if index in bases and name and not name.startswith('$'):header.append(f'#define {prefix}_{name} {address((name,value,index))&~1}u')
    header += [f'static const struct {{ unsigned address;const char *name; }} {prefix.lower()}_imports[] = {{']
    header += [f'{{{a}u,"{n}"}},' for n,a in imports.items()];header+=['};']
    write_text_if_changed(out/(stem+'.inc'), '\n'.join(header)+'\n')
    return image,code_size,imports
