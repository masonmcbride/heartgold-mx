#!/usr/bin/env python3
"""Private NARC containers: unchanged icon bytes; UTF-8 adapters from message JSON."""
import json, pathlib, struct, sys
root,out=map(pathlib.Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
from narc_tools import pack

icons={int(p.stem.rsplit('_',1)[1]):p.read_bytes() for p in (root/'files/itemtool/itemdata/item_icon').glob('item_icon_*') if p.suffix in ('.NCGR','.NCLR','.NCER','.NANR')}
if set(icons)!=set(range(max(icons)+1)):raise ValueError('Icon archive has gaps')
pack(out/'item_icon.narc',[icons[i] for i in range(len(icons))])
message_groups=set()
for source in sorted((root/'files/msgdata/msg').glob('msg_*.json')):
    group=int(source.stem.split('_')[1])
    if group in message_groups:raise ValueError('Duplicate original message bank')
    message_groups.add(group)
    messages=json.loads(source.read_text())['messages']
    members=[]
    if len({m['id'] for m in messages})!=len(messages):raise ValueError('Duplicate message identifiers')
    for index,message in enumerate(messages):
        text=message.get('en_US','');text=''.join(text) if isinstance(text,list) else text
        members.append(text.encode('utf-8')+b'\0')
    pack(out/f'msg_{group:04}.narc',members)

# Retain original archive bytes for bag graphics and fonts.
(out/'font.narc').write_bytes((root/'files/graphic/font.narc').read_bytes())
(out/'bag.narc').write_bytes((root/'files/a/0/1/5').read_bytes())
# Local UTF-8 -> original glyph-code adapter, not a replacement font.
import re
mapping={ord(v):int(k,16) for k,v in re.findall(r'^([0-9A-F]{4})=(.)$',(root/'tools/msgenc/charmap.txt').read_text(),re.M) if 0<int(k,16)<=509}
(out.parent/'charmap.inc').write_text('static const struct { uint32_t unicode; uint16_t glyph; } glyph_map[] = {\n'+''.join(f'{{{u},{g}}},\n' for u,g in sorted(mapping.items()))+'};\n')
