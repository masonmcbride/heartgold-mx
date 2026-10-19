"""Build-local unchanged C function slices and original event declarations.

Full field_control/field_warp_tasks units are NOT compiled by this target.
Source text, line locations and hashes remain reviewable in the build directory.
"""
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
event=(root/'include/map_events_internal.h').read_text()
types=[]
for name in ('BgEvent','WarpEvent','CoordEvent','MapEvents'):
 pattern=(r'typedef struct '+name+r' \{.*?^\} '+name+r';' if name=='MapEvents' else r'struct '+name+r' \{.*?^\};')
 types.append(re.search(pattern,event,re.M|re.S)[0])
field=(root/'include/field_system.h').read_text()
types.append(re.search(r'typedef struct FieldInput \{.*?^\} FieldInput;',field,re.M|re.S)[0])
(out/'event_types.inc').write_text('/* Original declarations; build-local. */\n'+'\n'.join(types)+'\n')
groups={
 'src/field/field_control.c':('PlayerAvatar_GetStandingTileCoords','ShiftFieldCoordsByCompassDirection','PlayerAvatar_GetFacingTileCoords','SetLocation','FieldSystem_MapConnection','FieldSystem_CheckMapTransition'),
 'src/location_backup.c':('Location_SetToPlayerRoom',),
 'src/field_warp_tasks.c':('sub_02052F94',),
}
lines=['/* Unchanged original function slices; not whole-unit coverage. */']
provenance=[]
for relative,names in groups.items():
 path=root/relative;text=path.read_text()
 if relative=='src/location_backup.c':
  records=list(re.finditer(r'^static const Location sLocation_PlayerRoom = \{.*?^\};',text,re.M|re.S))
  if len(records)!=1:raise ValueError('Original player-room location changed')
  lines.append(records[0][0])
 for name in names:
  # Original top-level closing braces start at column zero. Match definitions,
  # not forward declarations; Clang subsequently validates the generated unit.
  matches=list(re.finditer(r'^(?:static )?(?:inline )?(?:void|BOOL|u16|int|u32)\s+'+name+r'\([^;{}]*\) \{\n.*?^\}',text,re.M|re.S))
  if len(matches)!=1:raise ValueError(('original function definition changed',name,len(matches)))
  match=matches[0];body=match[0];line=text[:match.start()].count('\n')+1
  lines.extend([f'#line {line} "{path}"',body])
  provenance.append({'source':relative,'function':name,'line':line,'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()})
(out/'transition_kernel.inc').write_text('\n'.join(lines)+'\n')
whole=('src/map_events.c','src/save_local_field_data.c')
declarations=('include/map_events_internal.h','include/field_system.h','include/field_types_def.h','include/map_events.h','include/save_local_field_data.h')
(out/'transition_provenance.json').write_text(json.dumps({'functions':provenance,
 'whole_c_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in whole},
 'declaration_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in declarations}},indent=2)+'\n')
# Coherent original interaction selector closure, separate from warp services.
control=root/'src/field/field_control.c';text=control.read_text();selected=[]
for name in ('PlayerAvatar_GetStandingTileCoords','ShiftFieldCoordsByCompassDirection','PlayerAvatar_GetFacingTileCoords','FieldSystem_FacingMetatileBehavior','ov01_021E7B70','GetInteractedMetatileScript'):
 matches=list(re.finditer(r'^static (?:inline )?(?:void|BOOL|u8|u16|int|u32)\s+'+name+r'\([^;{}]*\) \{\n.*?^\}',text,re.M|re.S))
 if len(matches)!=1:raise ValueError(('Original interaction selector changed',name,len(matches)))
 match=matches[0];line=text[:match.start()].count('\n')+1
 selected.extend([match[0].split(' {\n',1)[0]+';'])
 selected.extend([f'#line {line} "{control}"',match[0]])
 provenance.append({'source':'src/field/field_control.c','function':name,'line':line,'source_sha256':hashlib.sha256(control.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(match[0].encode()).hexdigest()})
(out/'interaction_control_kernel.inc').write_text('\n'.join(selected)+'\n')
report=json.loads((out/'transition_provenance.json').read_text());report['interaction_functions']=provenance[-6:]
(out/'transition_provenance.json').write_text(json.dumps(report,indent=2)+'\n')
