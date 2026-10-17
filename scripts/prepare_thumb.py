#!/usr/bin/env python3
"""Assemble original Thumb with spelling-only GNU-to-LLVM compatibility.
Generated code and original-derived bytes remain in the ignored build tree.
"""
import hashlib, json, pathlib, re, struct, subprocess, sys
upstream, output, compiler = map(pathlib.Path, sys.argv[1:])
source = upstream / 'asm/overlay_01_021FAD1C.s'
helper_source=upstream / 'asm/unk_02020B8C.s'
text = source.read_text()+'\n'+helper_source.read_text()
lines = ['.syntax unified', '.cpu arm946e-s', '.thumb']
for line in text.splitlines():
    line = line.split(';')[0].strip()
    if not line or line.startswith(('#', '.include', 'thumb_func_end')):
        continue
    if line.startswith('thumb_func_start'):
        name = line.split()[1]
        lines.extend([f'.global {name}', '.thumb_func'])
        continue
    # GNU divided Thumb spelling implies flag-setting forms where unified
    # syntax requires explicit S. High-register/SP ADD and MOV preserve flags.
    match = re.match(r'(add|sub|mov|lsl|lsr|asr|mul|adc|orr|neg|eor)\s+(.*)', line)
    if match:
        op, args = match.groups()
        regs = re.findall(r'\b(?:r\d+|sp|lr|pc|ip)\b', args)
        high = any(r in ('sp','lr','pc','ip') or r.startswith('r') and int(r[1:])>=8 for r in regs)
        if op not in ('add','sub','mov') or not high:
            line = op + 's ' + args
    lines.append(line)
output.mkdir(parents=True, exist_ok=True)
assembly = output / 'terrain_thumb.s'
obj = output / 'terrain_thumb.o'
assembly.write_text('\n'.join(lines)+'\n')
subprocess.run([str(compiler), '-target', 'arm-none-eabi', '-c', str(assembly), '-o', str(obj)], check=True)
data = obj.read_bytes()
assert data[:6] == b'\x7fELF\x01\x01'
shoff = struct.unpack_from('<I',data,32)[0]
entsize, count, names = struct.unpack_from('<HHH',data,46)
sections = [struct.unpack_from('<10I',data,shoff+i*entsize) for i in range(count)]
def payload(s): return data[s[4]:s[4]+s[5]]
def string(table,offset): return table[offset:table.index(b'\0',offset)].decode()
section_names=payload(sections[names])
text_index=next(i for i,s in enumerate(sections) if string(section_names,s[0])=='.text')
code=payload(sections[text_index])
symbols={}
for s in sections:
    if s[1]!=2: continue
    strings=payload(sections[s[6]])
    for pos in range(s[4],s[4]+s[5],s[9]):
        name,value,size,info,other,index=struct.unpack_from('<IIIBBH',data,pos)
        name=string(strings,name)
        if index==text_index and name and not name.startswith('$'):symbols[name]=value&~1
# Link only the terrain unit plus the first two original arithmetic helpers.
# Other functions from the second object remain outside the executable range.
end=symbols['GetDistanceFromPointToLine']
code=bytearray(code[:end])
bridges={'_ll_mul':0x02010000,'FX_Div':0x02010004}
relocations=[]
for section in sections:
    if section[1]!=9 or section[7]!=text_index:continue
    symtab=sections[section[6]];strings=payload(sections[symtab[6]])
    for pos in range(section[4],section[4]+section[5],section[9]):
        offset,info=struct.unpack_from('<II',data,pos)
        if offset>=end:continue
        symbol_index=info>>8;kind=info&255
        name,value,size,sinfo,other,index=struct.unpack_from('<IIIBBH',data,symtab[4]+symbol_index*symtab[9])
        name=string(strings,name)
        assert kind==10, f'unsupported relocation {kind}: {name}'
        target=0x02000000+(value&~1) if index==text_index else bridges[name]
        # ARMv5 Thumb BL: relocation's implicit -4 addend accounts for PC+4.
        hi,lo=struct.unpack_from('<HH',code,offset)
        addend=((hi&2047)<<12)|((lo&2047)<<1)
        if addend&(1<<22):addend-=1<<23
        delta=target-(0x02000000+offset)+addend
        assert delta%2==0 and -(1<<22)<=delta<(1<<22)
        struct.pack_into('<HH',code,offset,0xf000|((delta>>12)&2047),0xf800|((delta>>1)&2047))
        relocations.append({'offset':offset,'symbol':name,'target':target})
origin=int('021FAD1C',16)
for name,value in symbols.items():
    if name.startswith('ov01_') and value<end:
        assert value==int(name.rsplit('_',1)[1],16)-origin, 'original function layout changed'
provenance={
    'sources':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (source,helper_source)},
    'assembler_version':subprocess.check_output([str(compiler),'--version'],text=True).splitlines()[0],
    'object_sha256':hashlib.sha256(data).hexdigest(),
    'enabled_code_sha256':hashlib.sha256(code).hexdigest(),
    'enabled_bytes':len(code),
    'enabled_functions':{name:value for name,value in symbols.items() if value<end and (name.startswith('ov01_') or name.startswith('sub_'))},
    'relocations':relocations, 'native_arithmetic_boundaries':bridges,
}
(output/'terrain_thumb_provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
header=['/* Generated from unchanged original assembly; do not distribute. */',
        'static const unsigned char hg_terrain_thumb[] = {',
        ','.join(str(b) for b in code), '};']
for name,address in bridges.items():header.append(f'#define HG_THUMB_BOUNDARY_{name} {address}u')
for name,value in symbols.items():
    if value<end and (name.startswith('ov01_') or name.startswith('sub_')):header.append(f'#define HG_THUMB_{name} {value}u')
(output/'terrain_thumb.inc').write_text('\n'.join(header)+'\n')
print(f'Original terrain Thumb: {len(code)} bytes, terrain solver and helpers assembled/relocated')
