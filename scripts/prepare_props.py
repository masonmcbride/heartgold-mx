#!/usr/bin/env python3
"""Retain selected original declarations without importing unused SDK objects."""
from generated_files import write_text_if_changed
import pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
selected=[('include/field/overlay_01_02204004.h','Field3dRenderObj'),('include/field/overlay_01_02204004.h','Field3dRenderObjManager'),('include/unk_02097268.h','SafariObjectConfig')]
parts=['/* Generated unchanged declarations from the selected upstream headers. */']
for relative,name in selected:
    text=(root/relative).read_text()
    match=re.search(r'typedef struct '+name+r'\s*\{[^}]*\}\s*'+name+r';',text)
    if not match:raise ValueError(name)
    parts.extend([f'#line {text[:match.start()].count(chr(10))+1} "{root/relative}"',match.group()])
text=(root/'include/field/map_load_manager.h').read_text()
parts.append(re.search(r'^#define MAP_PROP_MAX\s+\d+',text,re.M).group())
write_text_if_changed(out/'prop_types.inc', '\n'.join(parts)+'\n')
# Original render-object pool on the existing native model/render backend.
from prepare_original_field import functions
manager_code,manager_records=functions(root,{'src/field/overlay_01_02204004.c':(
 'Field3dRenderObjManager_New','Field3dRenderObjManager_Delete',
 'Field3dRenderObjManager_AllocRenderObj','Field3dRenderObjManager_FreeRenderObjInternal',
 'Field3dObjectList_GetRenderObjectByID','Field3dRenderObjManager_AllocRenderObjByIDInternal',
 'Field3dRenderObjManager_GetOrAllocRenderObjByID','Field3dRenderObjManager_FreeRenderObj',
 'Field3dRenderObjManager_IsModelAllocatedByIndex','Field3dRenderObjManager_IsResFileHeaderLoadedByIndex',
 'Field3dRenderObjManager_AllocInternal')})
write_text_if_changed(out/'render_object_manager.inc',manager_code)
import json
write_text_if_changed(out/'render_object_manager_provenance.json',json.dumps(manager_records,indent=2)+'\n')
# Original one-shot playback and frame/loop control. Native arenas replace DS
# byte-count allocations (the original pointer-bearing slots are 32-bit).
import hashlib,json
anim_header=(root/'include/field/map_prop_animation.h').read_text()
base_header=(root/'include/field/overlay_01_02204004.h').read_text()
parts=[]
for name,source in [('MapPropAnimation',base_header),('FieldSystemUnkSubCC_Sub0',base_header),('FieldSystemUnkSubC8',base_header),('FieldSystemUnkSubCC',base_header),('FieldSystemUnkSub104_Sub8',base_header),('FieldSystemUnkSub104',base_header)]:
    pattern=(r'(?:typedef )?struct '+name+r' \{.*?^\}[^\n]*;')
    matches=list(re.finditer(pattern,source,re.M|re.S))
    if len(matches)!=1:raise ValueError((name,len(matches)))
    parts.append(matches[0][0])
for match in re.finditer(r'^#define MAP_PROP_\w+\s+\d+\s*$',anim_header,re.M):parts.append(match[0])
for match in re.finditer(r'^typedef struct \w+ \{.*?^\} \w+;',anim_header,re.M|re.S):parts.append(match[0])
rtc_enum=re.findall(r'typedef enum RTC_TimeOfDay \{.*?\} TIMEOFDAY;', (root/'include/gf_rtc.h').read_text(),re.S)
if len(rtc_enum)!=1:raise ValueError('original time enum')
write_text_if_changed(out/'prop_animation_types.inc', rtc_enum[0]+'\n'+'typedef struct FieldSystemUnkSubCC_Sub0 FieldSystemUnkSubCC_Sub0;\n'+'\n'.join(parts)+'\n')
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
boundaries={'ov01_02204470','ov01_02204500','ov01_022046A4'}
pending=['MapPropOneShotAnimationManager_LoadPropAnimations','MapPropOneShotAnimationManager_PlayAnimation',
 'MapPropOneShotAnimationManager_UnloadAnimation','MapPropOneShotAnimationManager_IsAnimationLoopFinished',
 'MapPropAnimationManager_GetPropAnimationCount','ov01_02204350',
 'MapPropAnimationManager_Init','MapPropAnimationManager_Free','MapPropAnimationManager_GetAnimListNARCFileCount',
 'MapPropOneShotAnimationManager_New','MapPropOneShotAnimationManager_Free','ov01_022041C4','ov01_02204278','ov01_021E8F3C','ov01_0220460C','ov01_02204634','ov01_02204698']
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
  original=body;adaptations=[]
  if name=='ov01_022041C4':
   tail='    unkC8->unk8 = 0;\n}'
   if body.count(tail)!=1 or re.search(r'\breturn\b',body):raise ValueError('native pool constructor source drift')
   body=body.replace(tail,'    unkC8->unk8 = 0;\n    return unkC8;\n}')
   adaptations.append('restore the missing constructor return on native C')
  if name=='ov01_02204698':
   expected='void ov01_02204698(FieldSystemUnkSubCC *unkCC) {\n    ov01_0220434C(unkCC->unk0);\n}'
   if body!=expected:raise ValueError('aux count source drift')
   body=body.replace('void ov01_02204698','u16 ov01_02204698').replace('    ov01_0220434C','    return ov01_0220434C')
   adaptations.append('restore count return required by original ARM callers')
  if name=='ov01_022041D8':
   if body.count('count << 5')!=2 or body.count('count << 2')!=1:raise ValueError('native animation arena source drift')
   body=body.replace('count << 5','count * sizeof(*ret->unk10)').replace('count << 2','count * sizeof(*ret->unk14)')
   adaptations.append('native sizeof for animation arena and pointer slots')

  decls.append(body.split(' {',1)[0]+';');code.extend([f'#line {line} "{root/rel}"',body])
  records.append({'source':rel,'function':name,'sha256':hashlib.sha256(original.encode()).hexdigest(), 'emitted_sha256':hashlib.sha256(body.encode()).hexdigest(), 'adaptations':adaptations})
visual_table=re.findall(r'^static const u8 sTimeOfDayVisualState\[RTC_TIMEOFDAY_COUNT\] = \{.*?^\};',(root/'src/field/overlay_01_02204004.c').read_text(),re.M|re.S)
if len(visual_table)!=1:raise ValueError('original visual state table')
write_text_if_changed(out/'prop_animation_kernel.inc', visual_table[0]+'\n'+'\n'.join(decls+code)+'\n')
write_text_if_changed(out/'prop_animation_provenance.json', json.dumps(records,indent=2)+'\n')
