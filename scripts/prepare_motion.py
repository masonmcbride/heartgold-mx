import pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
text=(root/'include/map_events_internal.h').read_text()
m=re.search(r'struct ObjectEvent\s*\{.*?\n\};',text,re.S)
if not m:raise ValueError('Original ObjectEvent declaration missing')
(out/'motion_types.inc').write_text('/* Original declaration; generated build-local. */\n'+m.group(0)+'\n')
