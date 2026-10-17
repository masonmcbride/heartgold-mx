import pathlib,re,struct,sys
root,out=map(pathlib.Path,sys.argv[1:]);base=root/'files/data/mmodel/mmodel'
files={int(p.stem.split('_')[-1]):p for p in base.iterdir() if p.suffix in ('.NSBMD','.NSBTX')}
image=bytearray();fat=bytearray()
for i in range(max(files)+1):
 start=len(image)
 if i in files:image.extend(files[i].read_bytes())
 fat.extend(struct.pack('<II',start,len(image)));image.extend(b'\xff'*((-len(image))%4))
def block(tag,data):return tag+struct.pack('<I',8+len(data))+data
body=block(b'BTAF',struct.pack('<HH',max(files)+1,0)+fat)+block(b'BTNF',struct.pack('<IHH',4,0,1))+block(b'GMIF',image)
(out/'resources/characters.narc').write_bytes(struct.pack('<4sHHIHH',b'NARC',0xfffe,0x100,16+len(body),16,3)+body)
names={int(i):name for name,i in re.findall(r'^#define\s+MMODEL_(\w+)\s+(\d+)\s*$',(root/'include/constants/mmodel.h').read_text(),re.M) if int(i)<266}
(out/'character_names.inc').write_text('static const char *const character_names[266] = {\n'+''.join(f'[{i}]="{name}",\n' for i,name in sorted(names.items()))+'};\n')

plain=set(re.findall(r"\.short\s+SPRITE_\w+,\s*MMODEL_(\w+),\s*0x000\s*\|\s*\(0 << 10\)",(root/'asm/overlay_01_sprite_data.s').read_text()))
(out/'character_supported.inc').write_text('static const unsigned char character_supported[266] = {\n'+''.join(f'[{i}]=1,\n' for i,name in names.items() if name in plain)+'};\n')
