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
# Original one-shot playback and frame/loop control. Native arenas replace DS
# byte-count allocations (the original pointer-bearing slots are 32-bit).
import hashlib,json
anim_header=(root/'include/field/map_prop_animation.h').read_text()
base_header=(root/'include/field/overlay_01_02204004.h').read_text()
parts=[]
for name,source in [('MapPropAnimation',base_header),('FieldSystemUnkSubCC_Sub0',base_header),('FieldSystemUnkSubC8',base_header)]:
    pattern=(r'(?:typedef )?struct '+name+r' \{.*?^\}[^\n]*;')
    matches=list(re.finditer(pattern,source,re.M|re.S))
    if len(matches)!=1:raise ValueError((name,len(matches)))
    parts.append(matches[0][0])
for match in re.finditer(r'^#define MAP_PROP_\w+\s+\d+\s*$',anim_header,re.M):parts.append(match[0])
for match in re.finditer(r'^typedef struct \w+ \{.*?^\} \w+;',anim_header,re.M|re.S):parts.append(match[0])
(out/'prop_animation_types.inc').write_text('typedef struct FieldSystemUnkSubCC_Sub0 FieldSystemUnkSubCC_Sub0;\n'+'\n'.join(parts)+'\n')
# Query the reachable source closure once instead of maintaining its helpers.
# DS arena allocation and NNS resource objects are explicit native boundaries.
source_paths=('src/field/map_prop_animation.c','src/field/overlay_01_02204004.c')
functions={}
for relative in source_paths:
 text=(root/relative).read_text()
 for match in re.finditer(r'^(?:(?:static|const) )*[A-Za-z_][^\n;{}]*\b(\w+)\([^;{}]*\) \{[^\n]*\n.*?^\}',text,re.M|re.S):
  name=match[1]
  if name in functions:raise ValueError(('Ambiguous prop definition',name))
  functions[name]=(relative,match)
boundaries={'ov01_022042FC','ov01_0220431C','ov01_02204470','ov01_02204500'}
pending=['MapPropOneShotAnimationManager_LoadPropAnimations','MapPropOneShotAnimationManager_PlayAnimation',
 'MapPropOneShotAnimationManager_UnloadAnimation','MapPropOneShotAnimationManager_IsAnimationLoopFinished',
 'MapPropAnimationManager_GetPropAnimationCount','ov01_02204350']
selected=set()
while pending:
 name=pending.pop()
 if name in selected or name in boundaries:continue
 if name not in functions:raise ValueError(('Missing prop definition',name))
 selected.add(name)
 pending.extend((set(re.findall(r'\b(\w+)\s*\(',functions[name][1][0]))&functions.keys())-selected-boundaries)
selection={relative:sorted(name for name in selected if functions[name][0]==relative) for relative in source_paths}
code=[];decls=[];records=[]
for rel,names in selection.items():
 text=(root/rel).read_text()
 for name in names:
  matches=list(re.finditer(r'^(?:(?:static|const) )*[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{[^\n]*\n.*?^\}',text,re.M|re.S))
  if len(matches)!=1:raise ValueError((name,len(matches)))
  body=matches[0][0];line=text[:matches[0].start()].count('\n')+1
  decls.append(body.split(' {',1)[0]+';');code.extend([f'#line {line} "{root/rel}"',body])
  records.append({'source':rel,'function':name,'sha256':hashlib.sha256(body.encode()).hexdigest()})
(out/'prop_animation_kernel.inc').write_text('\n'.join(decls+code)+'\n')
(out/'prop_animation_provenance.json').write_text(json.dumps(records,indent=2)+'\n')
