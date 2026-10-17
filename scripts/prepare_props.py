#!/usr/bin/env python3
"""Retain selected original declarations without importing unused SDK objects."""
import pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
selected=[('include/field/overlay_01_02204004.h','Field3dRenderObj'),('include/unk_02097268.h','SafariObjectConfig')]
parts=['/* Generated unchanged declarations from the selected upstream headers. */']
for relative,name in selected:
    text=(root/relative).read_text()
    match=re.search(r'typedef struct '+name+r'\s*\{[^}]*\}\s*'+name+r';',text)
    if not match:raise ValueError(name)
    parts.extend([f'#line {text[:match.start()].count(chr(10))+1} "{root/relative}"',match.group()])
text=(root/'include/field/map_load_manager.h').read_text()
parts.append(re.search(r'^#define MAP_PROP_MAX\s+\d+',text,re.M).group())
(out/'prop_types.inc').write_text('\n'.join(parts)+'\n')
