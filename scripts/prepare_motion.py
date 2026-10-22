from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
text=(root/'include/map_events_internal.h').read_text()
m=re.search(r'struct ObjectEvent\s*\{.*?\n\};',text,re.S)
if not m:raise ValueError('Original ObjectEvent declaration missing')
write_text_if_changed(out/'motion_types.inc', '/* Original declaration; generated build-local. */\n'+m.group(0)+'\n')
write_text_if_changed(out/'motion_provenance.json', json.dumps({
 'whole_c_inputs':{'src/map_object.c':hashlib.sha256((root/'src/map_object.c').read_bytes()).hexdigest()},
 'declaration_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ('include/map_events_internal.h','include/map_object.h')},
 'boundary_translation_unit':'src/compat/map_objects.c',
 'scope':'Complete unchanged map_object.c included at a native boundary; compilation does not establish execution.'
},indent=2)+'\n')
