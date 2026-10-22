"""Recursive source dependency inventory, with explicit unresolved edges.

Used by audit_chapters. Source reachability is not execution evidence. Function
pointer expressions remain open even when candidate tables are also traversed.
"""
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path
from audit_reuse import strip_c
from generated_files import write_text_if_changed


def source_index(upstream, cache):
    upstream=upstream.resolve();cache=cache.resolve()
    paths=sorted(p for folder in ('src','include','asm','lib') for p in (upstream/folder).rglob('*') if p.suffix in ('.c','.h','.s','.inc'))
    names={str(p.relative_to(upstream)):p for p in paths}
    project=upstream.parent.parent
    for folder in (project/'src',cache.parent.parent/'generated'):
        for p in folder.rglob('*'):
            if p.suffix in ('.c','.h','.inc'):names['@host/'+str(p.relative_to(project))]=p
    inputs={name:hashlib.sha256(p.read_bytes()).hexdigest() for name,p in names.items()}
    identity=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if cache.exists():
        saved=json.loads(cache.read_text())
        if saved.get('inputs')==inputs and saved.get('extractor')==identity:return saved['definitions']
    definitions=defaultdict(list)
    for relative,p in sorted(names.items()):
        source=p.read_text();clean=strip_c(source)
        # Balanced bodies, not a regex ending at an arbitrary closing brace.
        patterns=[('function',r'^(?:[\w*]+[ \t]+)+\**([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{'),
                  ('table',r'^(?:static\s+)?(?:const\s+)?[\w* \t]+\b(\w+)\s*(?:\[[^\n;]*?\])+\s*=\s*\{')]
        for kind,pattern in patterns:
            for m in re.finditer(pattern,clean,re.M):
                at=m.end();depth=1;conditions=[]
                # Preserve all conditional arms while balancing each arm from
                # the same entry depth (e.g. BUGFIX_IGT_MAX's duplicate if).
                tokens=re.finditer(r'^\s*#\s*(ifn?def|if|elif|else|endif)\b[^\n]*|[{}]',clean[at:],re.M)
                for token in tokens:
                    directive=token[1]
                    if directive in ('if','ifdef','ifndef'):conditions.append([depth,[]])
                    elif directive in ('else','elif') and conditions:
                        conditions[-1][1].append(depth);depth=conditions[-1][0]
                    elif directive=='endif' and conditions:
                        start,arms=conditions.pop();arms.append(depth)
                        if len(set(arms))!=1:raise ValueError(('conditional brace mismatch',p,m[1]))
                    elif not directive:depth+=(token[0]=='{')-(token[0]=='}')
                    if depth==0:
                        at=m.end()+token.end();break
                if depth:raise ValueError(('unclosed definition',p,m[1]))
                definitions[m[1]].append(dict(source=relative,line=clean.count('\n',0,m.start())+1,kind=kind,body=clean[m.start():at]))
        for m in re.finditer(r'^#define[ \t]+(\w+)([^\n]*(?:\\\n[^\n]*)*)',clean,re.M):
            definitions[m[1]].append(dict(source=relative,line=clean.count('\n',0,m.start())+1,kind='macro',body=m[2].replace('\\\n',' ')))
        if p.suffix in ('.s','.inc'):
            for m in re.finditer(r'^\s*(?:thumb_func_start|arm_func_start)\s+(\w+)\s*\n(.*?)^\s*(?:thumb_func_end|arm_func_end)\s+\1',source,re.M|re.S):
                definitions[m[1]].append(dict(source=relative,line=source.count('\n',0,m.start())+1,kind='assembly',body=m[2]))
    cache.parent.mkdir(parents=True,exist_ok=True)
    write_text_if_changed(cache,json.dumps(dict(inputs=inputs,extractor=identity,definitions=definitions),sort_keys=True)+'\n')
    return definitions


def event_closure(upstream, generated, source, entry, output):
    from audit_chapters import Graph, strip_comments
    text=(upstream/source).read_text();macros=dict(re.findall(r'^\s*\.macro\s+(\w+)[^\n]*\n(.*?)^\s*\.endm',(upstream/'asm/macros/script.inc').read_text(),re.M|re.S))
    commands=re.findall(r'^    (\w+),$',(upstream/'src/data/fieldmap/script_cmd_table.h').read_text(),re.M)
    standards={}
    for p in (upstream/'include/constants').glob('*.h'):
        for name,value in re.findall(r'^#define\s+(std_\w+)\s+(\d+)\s*$',p.read_text(),re.M):standards[name]=int(value)
    # Original standard-bank ranges come from the same generated catalog as VM.
    catalog=subprocess.check_output(['xcrun','clang','-E','-P','-x','c','-I'+str(upstream/'include'),'-'],input='#include \"constants/std_script.h\"\n'+(generated/'script_standard_banks.inc').read_text(),text=True)
    banks=[tuple(map(int,row)) for row in re.findall(r'\{(\d+),(\d+),(\d+)\}',catalog)]
    graph=Graph();pending=[(source,entry)];visited=set();handlers=set();open_edges=[]
    def macro(name):
        key='macro:'+name
        if key in graph.nodes:return key
        graph.node(key,'script_macro')
        body=macros[name];op=re.search(r'^\s*\.short\s+(\d+)\s*$',body,re.M)
        if op:
            handler=commands[int(op[1])];target=graph.node('handler:'+handler,'handler',opcode=int(op[1]));graph.edge(key,target);handlers.add(handler)
        for line in body.splitlines():
            words=line.strip().split()
            if words and words[0] in macros:graph.edge(key,macro(words[0]))
        return key
    while pending:
        path,label=pending.pop();key=path+':'+label
        if key in visited:continue
        visited.add(key);graph.node(key,'script_block',source=path)
        data=(upstream/path).read_text();labels=list(re.finditer(r'^(\w+):\s*$',data,re.M));positions={m[1]:i for i,m in enumerate(labels)}
        if label not in positions:open_edges.append(dict(user=key,kind='missing_script_label',target=label));continue
        at=positions[label];body=data[labels[at].end():labels[at+1].start() if at+1<len(labels) else len(data)]
        instructions=[]
        for line in strip_comments(body).splitlines():
            words=line.strip().split(None,1)
            if not words or words[0].startswith(('.','#')):continue
            name=words[0];arg=words[1] if len(words)>1 else '';instructions.append(name)
            if name in macros:graph.edge(key,macro(name))
            # Follow every branch/call/movement label, regardless of predicate.
            for target in re.findall(r'\b\w+\b',arg):
                if target in positions:graph.edge(key,path+':'+target);pending.append((path,target))
            if name=='CallStd':
                number=standards.get(arg.strip())
                if number is None:open_edges.append(dict(user=key,kind='dynamic_standard_script',target=arg));continue
                candidates=[r for r in banks if number>=r[0]]
                if not candidates:raise ValueError(('standard range',number))
                first,bank,_=max(candidates);files=list((upstream/'files/fielddata/script/scr_seq').glob(f'scr_seq_{bank:04d}*.s'))
                if len(files)!=1:raise ValueError(('standard source',bank))
                destination=str(files[0].relative_to(upstream));defs=re.findall(r'^\s*ScrDef\s+(\w+)',files[0].read_text(),re.M);target=defs[number-first]
                graph.edge(key,destination+':'+target);pending.append((destination,target))
        if at+1<len(labels) and (not instructions or instructions[-1] not in ('End','Return','GoTo','EndMovement')):
            target=labels[at+1][1];graph.edge(key,path+':'+target);pending.append((path,target))
    definitions=source_index(upstream,output.parent/'source-index.json');todo=[(n,None,'handler:'+n) for n in handlers];expanded=set()
    keywords={'if','for','while','switch','return','sizeof','typeof','_Static_assert','asm','__asm__'}
    while todo:
        name,local,user=todo.pop();rows=definitions.get(name,[])
        local_rows=[r for r in rows if r['source']==local]
        if local_rows:rows=local_rows
        else:
            native=[r for r in rows if r['source'].startswith('@host/') and (r['kind']=='macro' or not r['body'].lstrip().startswith('static '))]
            if native:rows=native
        if not rows:
            target='unresolved:'+name;graph.node(target,'unresolved_symbol');graph.edge(user,target);open_edges.append(dict(user=user,kind='missing_definition',target=name));continue
        for row in rows:
            key=row['source']+':'+name+':'+str(row['line']);graph.node(key,row['kind'],source=row['source'],line=row['line']);graph.edge(user,key)
            if key in expanded:continue
            expanded.add(key);body=row['body']
            if row['kind']=='function':body=body[body.index('{')+1:]
            refs=set(re.findall(r'\b[A-Za-z_]\w*\b',body))&definitions.keys();refs.discard(name)
            calls=set(re.findall(r'\b(\w+)\s*\(',body))-keywords if row['kind'] in ('function','macro') else set()
            if row['kind']=='assembly':
                calls.update(re.findall(r'^\s*blx?\s+((?!r[0-9]+\b)\w+)',body,re.M))
                for register in re.findall(r'^\s*blx?\s+(r[0-9]+)\b',body,re.M):open_edges.append(dict(user=key,kind='indirect_assembly_call',target=register))
            for called in calls-refs-{name}:
                # Function declaration name is not a call; macro params and
                # indirect pointer calls stay open for explicit target review.
                if called not in definitions:open_edges.append(dict(user=key,kind='unresolved_call',target=called))
            for expr in re.findall(r'\b\w+\s*(?:->|\.)\s*\w+\s*(?:\[[^\]]*\])?\s*\(',body):open_edges.append(dict(user=key,kind='indirect_call',target=expr))
            for ref in refs:todo.append((ref,row['source'],key))
    # Materialize unresolved references as graph nodes; never hide missing leaves.
    for edge in open_edges:
        target='open:'+edge['kind']+':'+edge['target'];graph.node(target,'open_dependency');graph.edge(edge['user'],target)
    graph.seeds['event'].add(source+':'+entry);_,components,_=graph.partition()
    report=dict(root=source+':'+entry,inputs='source-index.json',complete=not open_edges,nodes=graph.nodes,edges={k:sorted(v) for k,v in graph.edges.items()},cycles=[c for c in components if len(c)>1],open_edges=open_edges,handlers=sorted(handlers),evidence='conservative recursive source references; unresolved indirect calls require target proofs; no execution claim')
    output.parent.mkdir(parents=True,exist_ok=True);write_text_if_changed(output,json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(nodes=len(graph.nodes),handlers=len(handlers),open_edges=len(open_edges),report=str(output))))
    return report


def native_calls(project, build):
    """Compiler-resolved native calls; scoped internal symbols never alias.

    Uses existing compiler dependency files for cache invalidation. Indirect calls
    are deliberately unresolved until their stores/tables have target proofs.
    """
    import shlex
    folder=build/'audit/llvm';folder.mkdir(parents=True,exist_ok=True)
    result={};public=defaultdict(list)
    database=json.loads((build/'compile_commands.json').read_text())
    for row in database:
        source=Path(row['file'])
        if source.suffix!='.c' or '/tests/' in str(source):continue
        args=shlex.split(row['command']);object_path=Path(args[args.index('-o')+1]);object_path=Path(row['directory'])/object_path
        deps=object_path.with_suffix('.o.d')
        if not deps.exists():raise ValueError(('build compiler dependencies first',source))
        names=shlex.split(deps.read_text().replace('\\\n','').split(':',1)[1]);inputs={p:hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in names}
        key=hashlib.sha256(str(source).encode()).hexdigest()[:16];ir=folder/(key+'.ll');stamp=folder/(key+'.json')
        identity=dict(command=row['command'],inputs=inputs,analysis_flags=['-femit-all-decls'])
        if not ir.exists() or not stamp.exists() or json.loads(stamp.read_text())!=identity:
            at=args.index('-o');del args[at:at+2];args.remove('-c');args+=['-S','-emit-llvm','-femit-all-decls','-o',str(ir)]
            subprocess.run(args,cwd=row['directory'],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
            write_text_if_changed(stamp,json.dumps(identity,sort_keys=True)+'\n')
        data=ir.read_text();unit=str(source.relative_to(project));functions=list(re.finditer(r'^define ([^\n]*?)@([\w.$]+)\([^\n]*\)[^\n]*\{\n(.*?)^}',data,re.M|re.S))
        for match in functions:
            name=match[2];node='llvm:'+unit+':'+name
            calls=re.findall(r'\bcall\b[^\n]*?@([\w.$]+)\(',match[3]);indirect=re.findall(r'\bcall\b[^\n]*?%([\w.]+)\(',match[3])
            result[node]=dict(unit=unit,name=name,calls=sorted(set(calls)),indirect=sorted(set(indirect)))
            if not re.search(r'\binternal\b',match[1]):public[name].append(node)
    linked=subprocess.check_output(['xcrun','nm','-u',str(build/'test_violet')],text=True)
    platform={name[1:] for name in re.findall(r'^\s*(_\S+)',linked,re.M)}
    return dict(functions=result,public=dict(public),platform=sorted(platform),evidence='Clang emitted calls from current built C units; pointer dispatch remains open')


def function_closure(project,upstream,build,roots,context,output):
    """Resolve original imports against the actual native compilation context."""
    import shlex
    native=native_calls(project,build);definitions=source_index(upstream,output.parent/'source-index.json')
    command=next(c for c in json.loads((build/'compile_commands.json').read_text()) if c['file']==str(project/context))
    args=shlex.split(command['command']);at=args.index('-o');del args[at:at+2];args.remove('-c');args+=['-E','-dM']
    macros=dict(re.findall(r'^#define\s+(\w+)(?:\([^\n]*?\))?\s+([^\n]*)',subprocess.check_output(args,cwd=command['directory'],text=True,stderr=subprocess.DEVNULL),re.M))
    nodes={};edges=defaultdict(set);pending=[(n,context,None) for n in roots];opened=[]
    def resolve(name,unit,user):
        alias=macros.get(name,'') if unit==context else ''
        if re.fullmatch(r'[A-Za-z_]\w*',alias):name=alias
        local='llvm:'+unit+':'+name
        choices=[local] if local in native['functions'] else native['public'].get(name,[])
        if choices:return choices
        if name.startswith('llvm.') or name in native['platform']:
            key='platform:'+name;nodes[key]=dict(kind='compiler_intrinsic' if name.startswith('llvm.') else 'linked_platform_ABI');return [key]
        rows=[r for r in definitions.get(name,[]) if r['kind'] in ('function','assembly') and not r['source'].startswith('@host/')]
        if len(rows)==1:
            row=rows[0];key='original:'+row['source']+':'+name
            nodes.setdefault(key,dict(kind='unselected_original_definition',source=row['source'],line=row['line'],body=row['body'],unit=unit));return [key]
        key='open:'+name;nodes[key]=dict(kind='unresolved',candidates=[dict(source=r['source'],line=r['line']) for r in rows]);opened.append(dict(user=user,target=name,kind='missing_or_ambiguous_definition'));return [key]
    expanded=set();root_nodes=[]
    while pending:
        name,unit,user=pending.pop()
        for key in resolve(name,unit,user):
            if user:edges[user].add(key)
            else:root_nodes.append(key)
            if key in expanded:continue
            expanded.add(key)
            if key in native['functions']:
                row=native['functions'][key];nodes[key]=dict(kind='compiled_native_function',unit=row['unit'],name=row['name'])
                for target in row['calls']:pending.append((target,row['unit'],key))
                for target in row['indirect']:opened.append(dict(user=key,target=target,kind='indirect_native_call'))
            elif nodes[key]['kind']=='unselected_original_definition':
                row=nodes[key];body=row['body'];body=body[body.index('{')+1:] if row.get('source','').endswith(('.c','.h')) else body
                calls=set(re.findall(r'\b(\w+)\s*\(',body))-{'if','for','while','switch','sizeof','return','_Static_assert'}
                for target in calls:pending.append((target,row['unit'],key))
                for expr in re.findall(r'\w+\s*(?:->|\.)\s*\w+\s*\(',body):opened.append(dict(user=key,target=expr,kind='indirect_original_call'))
    for value in nodes.values():value.pop('body',None)
    report=dict(roots=root_nodes,context=context,nodes=nodes,edges={k:sorted(v) for k,v in edges.items()},open_edges=opened,complete=not opened,evidence='Compiler-resolved existing native C calls plus unselected original definitions; indirect targets remain open; not execution evidence')
    write_text_if_changed(output,json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(dict(nodes=len(nodes),open_edges=len(opened),complete=not opened,report=str(output))))
    return report
