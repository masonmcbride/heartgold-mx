"""Resolve battle imports against real host archives; unsupported services fail."""
import json,pathlib,re,subprocess,sys
out=pathlib.Path(sys.argv[1]);provider=pathlib.Path(sys.argv[2]);libraries=[pathlib.Path(p) for p in sys.argv[3:]]
available=set()
for library in libraries:
 text=subprocess.check_output(['/usr/bin/nm','-gU',str(library)],text=True,stderr=subprocess.DEVNULL)
 available.update(re.findall(r'\b[TS] _(\w+)$',text,re.M))
implemented=set(re.findall(r'^[^\n;{}]*\b(\w+)\([^;{}]*\)\s*\{',provider.read_text(),re.M))
metadata=json.loads((out/'battle_provenance.json').read_text());selected={r['function'] for r in metadata['functions']}
prototypes={}
for name in ('battle_imports.inc','battle_decls.inc'):
 for match in re.finditer(r'^([^\n;{}]*\b(\w+)\([^;{}]*\));',(out/name).read_text(),re.M):prototypes[match[2]]=match[1]
for match in re.finditer(r'^([^\n;{}]*\b(\w+)\([^;{}]*\));',(provider.parent/'battle.h').read_text(),re.M):prototypes[match[2]]=match[1]
aliases=[];stubs=[];declarations=[]
# These original APIs already have typed native names in compat/pokemon.h.
renamed={'String_Delete':'hg_mon_string_delete','CopyU16ArrayToString':'hg_mon_string_from_array'}
for name in sorted(set(metadata['imports'])-selected):
 if name not in prototypes:continue
 if renamed.get(name,name) in available and name not in implemented:continue
 aliases.append(f'#define {name} hg_battle_boundary_{name}');declarations.append(prototypes[name]+';')
 if name not in implemented:stubs.append(prototypes[name]+' { battle_boundary_failed("'+name+'"); }')
(out/'battle_boundary_decls.inc').write_text('\n'.join(declarations)+'\n')
(out/'battle_aliases.inc').write_text('\n'.join(aliases)+'\n')
(out/'battle_unsupported.inc').write_text('\n'.join(stubs)+'\n')
