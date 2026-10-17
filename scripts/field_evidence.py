#!/usr/bin/env python3
"""Join compiler call edges, linked symbols and actual instrumented execution.

Never equate a linked function with an implemented service. Contract states are
explicit inputs; compiler edges and runtime observations are measured separately.
Artifacts contain provenance/metadata only and belong in ignored build directories.
"""
import argparse,collections,hashlib,json,pathlib,re,shlex,subprocess
from audit_native import manifest as source_manifest

def run(command,cwd=None):
    return subprocess.run(command,cwd=cwd,check=True,text=True,capture_output=True).stdout

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def walk(node):
    yield node
    for child in node.get('inner',[]):yield from walk(child)

def compile_graph(build,source):
    rows=json.loads((build/'compile_commands.json').read_text())
    row=next(r for r in rows if pathlib.Path(r['file']).resolve()==source)
    obj=pathlib.Path(row['output']);obj=obj if obj.is_absolute() else pathlib.Path(row['directory'])/obj
    if not obj.is_file() or obj.stat().st_mtime_ns<source.stat().st_mtime_ns:raise ValueError('Missing/stale original object; rebuild first')
    args=shlex.split(row['command']);command=[];i=0
    while i<len(args):
        if args[i] in ('-o','-c'):
            i+=2 if args[i]=='-o' else 1;continue
        command.append(args[i]);i+=1
    command+=['-fsyntax-only','-Xclang','-ast-dump=json']
    ast=json.loads(run(command,row['directory']));text=source.read_text();functions={}
    for node in ast.get('inner',[]):
        loc=node.get('loc',{});name=node.get('name','');off=loc.get('offset',-1)
        if node.get('kind')!='FunctionDecl' or not any(c.get('kind')=='CompoundStmt' for c in node.get('inner',[])):continue
        if loc.get('includedFrom') or off<0 or text[off:off+len(name)]!=name:continue
        calls=set();indirect=False
        for item in walk(node):
            if item.get('kind')!='CallExpr':continue
            refs=[n.get('referencedDecl',{}) for n in walk(item['inner'][0]) if n.get('kind')=='DeclRefExpr']
            for ref in refs:
                if ref.get('kind')=='FunctionDecl':calls.add(ref['name'])
                else:indirect=True
        functions[name]={'line':text[:off].count('\n')+1,'calls':sorted(calls),'indirect_calls':indirect}
    if not functions:raise ValueError('No original function bodies in compiler AST')
    return functions,command,obj

def resolve_trace(events,symbols,base,graph,manifest):
    stack=[];counts=collections.Counter();phases=[]
    for event in events:
        if event['kind']=='phase':phases.append([event['name'],event['live']]);continue
        name=symbols.get(base+event['offset'])
        if name not in graph:raise ValueError(f'Unresolved/non-original instrumented function: {event}')
        event['function']=name
        if event['kind']=='enter':stack.append(name);counts[name]+=1
        elif event['kind']=='exit':
            if not stack or stack.pop()!=name:raise ValueError('Unbalanced original function trace')
        else:raise ValueError('Unknown trace event')
    if stack or phases!=manifest['phases']:raise ValueError('Incomplete lifecycle/trace')
    if missing:=set(manifest['required_execution'])-counts.keys():raise ValueError(f'Missing original execution: {missing}')
    return counts

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--build',type=pathlib.Path,required=True);parser.add_argument('--upstream',type=pathlib.Path,required=True);parser.add_argument('--scenario',type=pathlib.Path,required=True);args=parser.parse_args()
    build=args.build.resolve();root=args.upstream.resolve();manifest=json.loads(args.scenario.read_text());out=build/'evidence'/manifest['id']
    if out.is_relative_to(root):raise ValueError('Evidence must not be written into upstream')
    source=root/manifest['source'];before=digest(source);exe=build/manifest['executable'];graph,command,obj=compile_graph(build,source)
    if exe.stat().st_mtime_ns<obj.stat().st_mtime_ns:raise ValueError("Executable predates original object; rebuild first")
    for original,packed in manifest["resources"].items():
        if digest(root/original)!=digest(build/"generated/resources"/packed):raise ValueError(f"Stale prepared resource: {packed}")
    symbols={};linked=set();base=None
    for line in run(['nm','-n',str(exe)]).splitlines():
        match=re.match(r'^([0-9a-fA-F]+)\s+\S\s+(.+)$',line)
        if not match:continue
        address=int(match[1],16);name=match[2]
        if name=='__mh_execute_header':base=address
        symbols[address]=name.removeprefix('_');linked.add(name.removeprefix('_'))
    if base is None:raise ValueError('Missing Mach-O base symbol')
    raw=run([str(exe)]);events=[json.loads(line) for line in raw.splitlines()];header=events.pop(0)
    if header.get('scenario')!=manifest['id']:raise ValueError('Wrong scenario executable')
    counts=resolve_trace(events,symbols,base,graph,manifest)
    implemented=set(manifest['native_boundaries']);unsupported=set(manifest['unsupported_boundaries'])
    external={call for value in graph.values() for call in value['calls'] if call not in graph}
    # Assertions are precondition failures, not an implementation of game behavior.
    if unknown:=external-implemented-unsupported-{'abort'}:raise ValueError(f'Unclassified boundary contracts: {unknown}')
    def blockers(name,seen=frozenset()):
        if name in seen:return set()
        if name not in graph:return {name} if name not in implemented and name!='abort' else set()
        result=set()
        for call in graph[name]['calls']:result|=blockers(call,seen|{name})
        return result
    for name,row in graph.items():
        row.update({'compiled':True,'linked':name in linked,'observed_calls':counts[name],'execution_evidence':'passing scenario' if counts[name] else 'not observed','unsupported_dependencies':sorted(blockers(name))})
    providers=collections.defaultdict(list)
    for path in source_manifest(root,'src/meson.build','pokeheartgold_c'):
        for name in re.findall(r'^\s*(?:[\w*]+\s+)+\**\s*([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{',path.read_text(errors='replace'),re.M):
            providers[name].append({'path':path.relative_to(root).as_posix(),'language':'C'})
    for path in source_manifest(root,'asm/meson.build','pokeheartgold_asm'):
        for name in re.findall(r'^\s*(?:thumb_func_start|arm_func_start)\s+(\w+)',path.read_text(errors='replace'),re.M):
            providers[name].append({'path':path.relative_to(root).as_posix(),'language':'ARM/Thumb assembly'})
    frontier=[]
    for symbol in unsupported:
        users=[name for name,row in graph.items() if symbol in row['unsupported_dependencies']]
        if users:frontier.append({'boundary':symbol,'blocked_functions':users,'next_targets':[n for n in manifest['next_targets'] if n in users],'provider_hints':providers[symbol]})
    frontier.sort(key=lambda row:(-len(row['next_targets']),-len(row['blocked_functions']),row['boundary']))
    if before!=digest(source):raise ValueError('Upstream source changed during evidence collection')
    payload={'scenario':header,'upstream_revision':run(['git','-C',str(root),'rev-parse','HEAD']).strip(),'source':manifest['source'],'source_sha256':before,'object_sha256':digest(obj),'evidence_tool_sha256':digest(pathlib.Path(__file__)),'scenario_sha256':digest(args.scenario),'executable_sha256':digest(exe),'resource_sha256':{p:digest(root/p) for p in manifest['resources']},'compiler_ast_command':command,'functions':graph,'frontier':frontier,'scope':'Only the original prop-manager translation unit is instrumented. Observed execution is not full branch/semantic coverage. Boundary status is a declared contract, not inferred from linking.'}
    out.mkdir(parents=True,exist_ok=True);(out/'evidence.json').write_text(json.dumps(payload,indent=2)+'\n');(out/'trace.jsonl').write_text('\n'.join(json.dumps(e) for e in events)+'\n')
    lines=['# Original field lifecycle evidence','',f"Scenario `{manifest['id']}` passed: {len(counts)} of {len(graph)} original functions observed. Compiled/linked counts are separate from execution.",'','| Function | Linked | Observed calls | Unsupported dependencies |','| --- | --- | ---: | --- |']
    for name,row in graph.items():lines.append(f"| `{name}` | {row['linked']} | {row['observed_calls']} | {', '.join(row['unsupported_dependencies']) or 'none'} |")
    lines+=['','## Dependency frontier','','Declared unsupported boundaries ranked by the number of next targets they block. This is dependency fan-out, not an effort estimate.','','| Boundary | Next targets blocked | Original functions blocked | Provider hint |','| --- | --- | ---: | --- |']
    for row in frontier:lines.append(f"| `{row['boundary']}` | {', '.join(row['next_targets'])} | {len(row['blocked_functions'])} | {', '.join(p['path']+' ('+p['language']+')' for p in row['provider_hints']) or 'SDK / unindexed'} |")
    lines+=['',payload['scope'],'','Source/resource hashes, exact compiler command and complete call edges are in `evidence.json`. The compiler-instrumented, symbol-resolved trace is in `trace.jsonl`. No original resource bytes or source excerpts are included.']
    (out/'REPORT.md').write_text('\n'.join(lines)+'\n');print(f"{manifest['id']}: {len(counts)}/{len(graph)} original functions executed; {len(frontier)} explicit boundary blockers; evidence in {out}")
if __name__=='__main__':main()
