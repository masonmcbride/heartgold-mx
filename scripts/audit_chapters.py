#!/usr/bin/env python3
"""Conservative original-source dependency ownership, not gameplay coverage.

Warp/progression edges are reported separately: following them as implementation
prerequisites would collapse the connected world into the shared root.
"""
import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def strip_comments(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


class Graph:
    def __init__(self):
        self.nodes = {}
        self.edges = defaultdict(set)
        self.seeds = defaultdict(set)
        self.pinned = set()

    def node(self, key, kind, **data):
        value = dict(kind=kind, **data)
        if key in self.nodes and self.nodes[key] != value:
            raise ValueError(f'conflicting node {key}')
        self.nodes[key] = value
        return key

    def edge(self, user, dependency):
        self.edges[user].add(dependency)

    def partition(self):
        for node, targets in list(self.edges.items()):
            if node not in self.nodes or any(t not in self.nodes for t in targets):
                raise ValueError('dependency edge references an undefined node')
        if any(n not in self.nodes for nodes in self.seeds.values() for n in nodes):
            raise ValueError('seed references an undefined node')
        # Tarjan components preserve genuine cycles as one ownership unit.
        index, low, stack, active, components = {}, {}, [], set(), []
        def visit(node):
            index[node] = low[node] = len(index)
            stack.append(node)
            active.add(node)
            for target in sorted(self.edges[node]):
                if target not in index:
                    visit(target)
                    low[node] = min(low[node], low[target])
                elif target in active:
                    low[node] = min(low[node], index[target])
            if low[node] == index[node]:
                component = []
                while True:
                    target = stack.pop()
                    active.remove(target)
                    component.append(target)
                    if target == node:
                        break
                components.append(sorted(component))
        for node in sorted(self.nodes):
            if node not in index:
                visit(node)
        membership = {n: i for i, group in enumerate(components) for n in group}
        dag = defaultdict(set)
        for node, targets in list(self.edges.items()):
            for target in targets:
                if membership[node] != membership[target]:
                    dag[membership[node]].add(membership[target])
        consumers = defaultdict(set)
        for owner, nodes in self.seeds.items():
            pending, seen = [membership[n] for n in nodes], set()
            while pending:
                group = pending.pop()
                if group in seen:
                    continue
                seen.add(group)
                consumers[group].add(owner)
                pending.extend(dag[group])
        # A shared implementation dependency and every prerequisite belong above
        # all children. In the requested flat tree that least ancestor is root.
        shared = {i for i, users in consumers.items() if len(users) > 1 or 'root' in users}
        shared.update(membership[n] for n in self.pinned)
        pending = list(shared)
        while pending:
            group = pending.pop()
            for target in dag[group]:
                if target not in shared:
                    shared.add(target)
                    pending.append(target)
        owners = {}
        for group, nodes in enumerate(components):
            users = consumers[group]
            owner = 'root' if group in shared else next(iter(users)) if users else 'unclaimed'
            owners.update((node, owner) for node in nodes)
        for node, targets in self.edges.items():
            for target in targets:
                if owners[target] not in ('root', owners[node]):
                    raise ValueError(f'cross-owner implementation edge {node} -> {target}')
        return owners, components, {node: sorted(consumers[membership[node]]) for node in self.nodes}


def build(upstream, manifest):
    graph, unresolved, traversal = Graph(), [], []
    services = {
        'persistence': 'All original save records, initialization, durable save/load and continue',
        'script-runtime': 'Original script dispatch, applications, state ownership and supported-service contracts',
        'field': 'Player/follower motion, ledges, collision, objects, puzzles and transitions',
        'battles': 'Trainer/wild battles, capture, items, party changes and blackout return',
        'menus': 'Bag, party, PC, shops, healing and required applications',
        'platform': 'Input, rendering, audio, RTC, resources and host file boundary',
    }
    for name, contract in services.items():
        node = graph.node('service:' + name, 'root_contract', contract=contract)
        graph.seeds['root'].add(node)
        graph.pinned.add(node)
    source_hashes, source_texts = {}, {}
    def read(path):
        if path not in source_texts:
            data = path.read_bytes()
            source_hashes[str(path.relative_to(upstream))] = hashlib.sha256(data).hexdigest()
            source_texts[path] = data.decode()
        return source_texts[path]
    # Reuse the exact registry parser; records remain required even when no
    # currently admitted chapter touches them.
    from audit_save import tables
    normal, extra = tables(read(upstream / 'src/save_arrays.c'))
    for kind, rows in (('normal', normal), ('extra', extra)):
        for ident, page, size, init in rows:
            node = graph.node(f'save:{kind}:{ident}', 'save_record', original_id=ident,
                              page=page, size_callback=size, initializer=init)
            graph.edge('service:persistence', node)
    headers = strip_comments(read(upstream / 'src/data/map_headers.h'))
    matches = re.findall(r'\[(MAP_\w+)\]\s*=\s*\{([^{}]*)\}', headers, re.S)
    if (not matches or len({m[0] for m in matches}) != len(matches)
            or len(re.findall(r'\[MAP_\w+\]\s*=', headers)) != len(matches)):
        raise ValueError('ambiguous map-header records')
    unknown_overrides = set(manifest.get('map_overrides', {})) - {m[0] for m in matches}
    if unknown_overrides:
        raise ValueError(f'unknown override maps: {sorted(unknown_overrides)}')
    chapter_ids = [c['id'] for c in manifest['chapters']]
    if len(chapter_ids) != len(set(chapter_ids)) or 'root' in chapter_ids:
        raise ValueError('duplicate/reserved chapter IDs')
    sections = {}
    for chapter in manifest['chapters']:
        team = chapter.get('test_team', [])
        if len(team) != 6 or len({m['species_id'] for m in team}) != 6:
            raise ValueError(f"chapter {chapter['id']} needs six diverse fixture Pokemon")
        for mon in team:
            if not (1 <= mon['level'] <= 100 and 1 <= len(mon['move_ids']) <= 4 and len(mon['moves']) == len(mon['move_ids'])):
                raise ValueError(f"invalid chapter fixture: {chapter['id']} {mon}")
        for section in chapter['map_sections']:
            if section in sections:
                raise ValueError(f'duplicate section ownership: {section}')
            sections[section] = chapter['id']
    for section in manifest['root']['shared_map_sections']:
        if section in sections:
            raise ValueError(f'root/child overlap: {section}')
        sections[section] = 'root'
    groups = [set(sections), set(manifest.get('excluded_map_sections', {})),
              set(manifest.get('unresolved_map_sections', {}))]
    if any(groups[i] & groups[j] for i in range(len(groups)) for j in range(i)):
        raise ValueError('overlapping section classifications')
    script_folder = upstream / 'files/fielddata/script/scr_seq'
    script_names = re.findall(r"'(scr_seq_[^']+\.s)'", read(script_folder / 'meson.build'))
    script_symbols = {Path(n).stem + '_bin': script_folder / n for n in script_names}
    event_folder = upstream / 'files/fielddata/eventdata/zone_event'
    event_names = re.findall(r"'([^']+\.json)'", read(event_folder / 'meson.build'))
    event_symbols = {'NARC_zone_event_' + Path(n).stem + '_bin': event_folder / n for n in event_names}
    commands = re.findall(r'^    (\w+),$', read(upstream / 'src/data/fieldmap/script_cmd_table.h'), re.M)
    if not commands:
        raise ValueError('missing original opcode table')
    macro_source = strip_comments(read(upstream / 'asm/macros/script.inc'))
    macros = re.findall(r'^\s*\.macro\s+(\w+)[^\n]*\n(.*?)^\s*\.endm', macro_source, re.M | re.S)
    if len({m[0] for m in macros}) != len(macros):
        raise ValueError('duplicate script macros')
    bodies = dict(macros)
    def instructions(text):
        for line in strip_comments(text).splitlines():
            line = line.strip()
            if line and not line.startswith(('.', '#')) and not line.endswith(':'):
                yield line.split()[0]
    def macro_node(name):
        key = 'command:' + name
        if key in graph.nodes:
            return key
        graph.node(key, 'script_macro', source='asm/macros/script.inc')
        body = bodies[name]
        opcode = re.search(r'^\s*\.short\s+(0x[0-9a-fA-F]+|\d+)\s*$', body, re.M)
        if opcode:
            number = int(opcode[1], 0)
            if number >= len(commands):
                raise ValueError(f'opcode outside table: {name}')
            handler = commands[number]
            target = graph.node('handler:' + handler, 'original_handler', opcode=number)
            graph.edge(key, target)
            graph.pinned.add(target)  # Shared VM/native compatibility implementation.
            graph.edge(target, 'service:script-runtime')
        for called in set(instructions(body)):
            if called in bodies:
                graph.edge(key, macro_node(called))
        return key
    def bank_node(path):
        key = 'source:' + str(path.relative_to(upstream))
        if key in graph.nodes:
            return key
        graph.node(key, 'script_source', source=str(path.relative_to(upstream)))
        text = read(path)
        uses = set(instructions(text))
        for name in sorted(uses & bodies.keys()):
            graph.edge(key, macro_node(name))
        # These tokens belong to movement/data assembly, not field opcodes.
        movement_source = strip_comments(read(upstream / 'asm/macros/movement.inc'))
        movement = set(re.findall(r'^\s*\.macro\s+(\w+)', movement_source, re.M))
        unknown = uses - bodies.keys() - movement
        if unknown:
            unresolved.append(dict(kind='unresolved_source_tokens', source=str(path.relative_to(upstream)), tokens=sorted(unknown)))
        return key
    excluded, classifications = [], {}
    fields_needed = ('mapsec', 'regionNo', 'scriptsBank', 'scriptHeaderBank', 'eventsBank', 'msgBank', 'matrixId', 'wildEncounterBank')
    for name, body in matches:
        fields = dict(re.findall(r'\.(\w+)\s*=\s*([^,]+),', body))
        if any(f not in fields for f in fields_needed):
            raise ValueError(f'missing map field: {name}')
        fields = {k: v.strip() for k, v in fields.items()}
        override = manifest.get('map_overrides', {}).get(name)
        owner = override['owner'] if override else sections.get(fields['mapsec'])
        if owner is None and fields['mapsec'] in manifest.get('unresolved_map_sections', {}):
            owner = 'root'  # Quarantine investigation has one owner too.
            unresolved.append(dict(kind='unresolved_map_scope', map=name, section=fields['mapsec'],
                                   reason=manifest['unresolved_map_sections'][fields['mapsec']]))
        if owner is None:
            reason = manifest.get('excluded_map_sections', {}).get(fields['mapsec'])
            if reason:
                excluded.append(dict(map=name, section=fields['mapsec'], reason=reason))
                continue
            unresolved.append(dict(kind='unassigned_map', map=name, section=fields['mapsec'], region=fields['regionNo']))
            continue
        if owner not in {'root', *(c['id'] for c in manifest['chapters'])}:
            raise ValueError(f'unknown owner {owner}')
        classifications[name] = owner
        key = graph.node('map:' + name, 'map', map=name, section=fields['mapsec'], source='src/data/map_headers.h')
        graph.seeds[owner].add(key)
        graph.edge(key, 'service:persistence')
        graph.edge(key, 'service:field')
        for field in ('scriptsBank', 'scriptHeaderBank'):
            path = script_symbols.get(fields[field])
            if path is None:
                unresolved.append(dict(kind='unresolved_script_bank', map=name, symbol=fields[field]))
            else:
                graph.edge(key, bank_node(path))
        symbol = fields['eventsBank']
        path = event_symbols.get(symbol)
        if path is None:
            unresolved.append(dict(kind='unresolved_event_bank', map=name, symbol=symbol))
        else:
            resource = graph.node('events:' + symbol, 'event_resource', source=str(path.relative_to(upstream)))
            graph.edge(key, resource)
            events = json.loads(read(path))
            for category in ('objects', 'bgs', 'coords'):
                for index, event in enumerate(events.get(category, [])):
                    event_key = graph.node(f'event:{name}:{category}:{index}', 'event_reference', map=name, category=category, index=index)
                    graph.edge(key, event_key)
                    graph.edge(event_key, resource)
                    # Scope ownership is exact; dynamic script-ID resolution is
                    # explicitly unfinished, never replaced with guessed edges.
            for warp in events.get('warps', []):
                traversal.append(dict(source=name, target=warp['header'], kind='warp'))
        for field in ('msgBank', 'matrixId', 'wildEncounterBank'):
            resource = graph.node(field + ':' + fields[field], 'resource_reference', resource_type=field, symbol=fields[field])
            graph.edge(key, resource)
    # CallStd may select any original standard mapping. Include the entire
    # declared registry conservatively rather than inventing per-chapter subsets.
    manager = read(upstream / 'src/script_manager.c')
    mappings = re.findall(r'const struct ScriptBankMapping sScriptBankMapping\[(\d+)\] = \{(.*?)\n\};', manager, re.S)
    if len(mappings) != 1:
        raise ValueError('ambiguous standard-script registry')
    rows = re.findall(r'\{\s*(_std_\w+|\d+),\s*scr_seq_(\d+)_bin,\s*msg_(\d+)\s*\},', mappings[0][1])
    if len(rows) != int(mappings[0][0]):
        raise ValueError('unparsed standard-script mappings')
    for base, bank, messages in rows:
        symbol = f'scr_seq_{int(bank):04}_bin'
        path = script_symbols.get(symbol)
        if path is None:
            raise ValueError(f'missing standard bank {symbol}')
        dependency = bank_node(path)
        graph.seeds['root'].add(dependency)
        if 'command:CallStd' in graph.nodes:
            graph.edge('command:CallStd', dependency)
    owners, components, consumers = graph.partition()
    # Runtime source/build integration remains single-writer. Chapter workers
    # produce isolated scenarios and request shared services through root.
    worklists = {}
    for owner in ('root', *(c['id'] for c in manifest['chapters'])):
        owned = sorted(n for n in graph.nodes if owners[n] == owner)
        required_root = sorted({b for a, targets in graph.edges.items() if owners[a] == owner
                                for b in targets if owners[b] == 'root' and owner != 'root'})
        worklists[owner] = dict(owned_nodes=owned, required_root_nodes=required_root,
                               writable_paths=['src/**', 'scripts/**', 'CMakeLists.txt', 'shared tests and build integration']
                               if owner == 'root' else [f'tests/chapters/{owner}.c', f'docs/chapters/{owner}.md'],
                               state_policy='private save directory per scenario; never merge save bytes',
                               admission='scenario preparation allowed; gameplay integration requires its root contracts and predecessor boundary save')
    chapters = {c['id']: c for c in manifest['chapters']}
    for owner, work in worklists.items():
        chapter = chapters.get(owner, {})
        work['integration_dependencies'] = chapter.get('integration_dependencies', [])
        work['boundary_contracts'] = chapter.get('boundary_contracts', [])
        work['test_team'] = chapter.get('test_team', [])
        work['private_build_directory'] = 'build/chapters/' + owner
        work['read_only_paths'] = ['upstream/**', 'other chapter paths']
    manifest_hash = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    return dict(schema_version=1, evidence='conservative source ownership; not execution or full call closure',
                nodes={n: dict(graph.nodes[n], owner=owners[n], consumers=consumers[n]) for n in sorted(graph.nodes)},
                implementation_edges=sorted([a, b] for a, bs in graph.edges.items() for b in bs),
                strongly_connected_components=[c for c in components if len(c) > 1],
                traversal_edges=traversal, map_owners=classifications, excluded_maps=excluded,
                worklists=worklists, manifest_sha256=manifest_hash,
                progression_contracts=manifest.get('event_dependency_contracts', []),
                unresolved=unresolved, source_hashes=source_hashes,
                readiness='blocked: source inventory is not runtime acceptance',
                missing_evidence=['broader field/application Continue reconstruction beyond validated opening states',
                                  'event-ID/header/dynamic dispatch and standard-script call closure',
                                  'original handler function/import closure and runtime service contracts',
                                  'native presentation and connected chapter execution'])


def runtime_inventory(report, generated):
    """Join source ownership to selected original handlers, never infer execution.

    Native presentation and operand-specific admission are deliberately left for
    contract review. A selected handler does not establish all of its branches.
    """
    inputs = ['script_core_table.inc', 'script_scene_table.inc', 'mart_provenance.json']
    if not all((generated / name).is_file() for name in inputs):
        return dict(available=False, reason='Configure the build to generate original handler selections.')
    selected = set()
    for name in inputs[:2]:
        selected.update(re.findall(r'^\[\d+\]=(ScrCmd_\w+),$', (generated / name).read_text(), re.M))
    selected.update(row['function'] for row in json.loads((generated / inputs[2]).read_text())['functions']
                    if row['function'].startswith('ScrCmd_'))
    edges = defaultdict(set)
    for user, dependency in report['implementation_edges']:
        edges[user].add(dependency)
    chapters = {}
    for owner, work in report['worklists'].items():
        pending, seen, handlers = list(work['owned_nodes']), set(), set()
        while pending:
            node = pending.pop()
            if node in seen:
                continue
            seen.add(node)
            if report['nodes'][node]['kind'] == 'original_handler':
                handlers.add(node.removeprefix('handler:'))
            pending.extend(edges[node])
        chapters[owner] = dict(selected_original_handlers=sorted(handlers & selected),
                               handlers_needing_contract_review=sorted(handlers - selected),
                               admission='scenario preparation only; no accepted chapter boundary save')
    return dict(available=True, basis='Generated original handler selection, not linked/executed behavior or complete opcode admission.',
                input_sha256={name: hashlib.sha256((generated / name).read_bytes()).hexdigest() for name in inputs},
                chapters=chapters)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', type=Path, default=ROOT / 'upstream/pokeheartgold')
    parser.add_argument('--manifest', type=Path, default=ROOT / 'scripts/johto_chapters.json')
    parser.add_argument('--output', type=Path, default=ROOT / 'build/audit/chapters.json')
    parser.add_argument('--build', type=Path, default=ROOT / 'build', help='configured build for original handler selection evidence')
    parser.add_argument('--require-assigned', action='store_true', help='fail if any map remains unassigned')
    parser.add_argument('--function-root', action='append', help='function root for native-context closure')
    parser.add_argument('--host-context', default='src/field_script.c')
    parser.add_argument('--native-calls', action='store_true', help='emit compiler-resolved C call inventory')
    parser.add_argument('--event-source', help='original script source for recursive event closure')
    parser.add_argument('--event-entry', help='original entry label; required with --event-source')
    args = parser.parse_args()
    if args.function_root:
        from dependency_closure import function_closure
        function_closure(ROOT,args.upstream,args.build.resolve(),args.function_root,args.host_context,args.output.resolve())
        return
    if args.native_calls:
        from dependency_closure import native_calls
        data=native_calls(ROOT,args.build.resolve())
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
        print('Compiler-resolved C definitions:',len(data['functions']))
        return
    if args.event_source:
        if not args.event_entry:parser.error('--event-entry is required')
        from dependency_closure import event_closure
        event_closure(args.upstream,args.build/'generated',args.event_source,args.event_entry,args.output)
        return
    if args.output.resolve().is_relative_to(args.upstream.resolve()):
        parser.error('output must be outside the unmodified upstream tree')
    report = build(args.upstream, json.loads(args.manifest.read_text()))
    report['runtime_inventory'] = runtime_inventory(report, args.build / 'generated')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    summary = ['# Generated chapter ownership', '',
               'Source ownership only. Complete save/runtime acceptance and unresolved references still gate dispatch.', '',
               '| Owner | Maps | Owned nodes | Direct root dependencies |',
               '| --- | ---: | ---: | ---: |']
    for owner, work in report['worklists'].items():
        maps = sum(o == owner for o in report['map_owners'].values())
        summary.append(f"| {owner} | {maps} | {len(work['owned_nodes'])} | {len(work['required_root_nodes'])} |")
    summary += ['', f"Explicitly excluded map headers: {len(report['excluded_maps'])}.",
                f"Unresolved references/scope decisions: {len(report['unresolved'])}.", '',
                'Private child writes are restricted to their reserved scenario/document paths. Shared runtime/build changes have one root owner.', '',
                'These counts are inventory sizes, not remaining effort or completion percentages.', '']
    runtime = report['runtime_inventory']
    if runtime['available']:
        summary += ['## Current handler review', '', runtime['basis'], '',
                    '| Owner | Selected original handlers | Other handlers requiring contract review |',
                    '| --- | ---: | ---: |']
        for owner, work in runtime['chapters'].items():
            summary.append(f"| {owner} | {len(work['selected_original_handlers'])} | {len(work['handlers_needing_contract_review'])} |")
        summary += ['', 'Other handlers include native presentation adapters and operand-restricted services; these are not counts of missing implementations.', '']
    args.output.with_suffix('.md').write_text('\n'.join(summary))
    counts = defaultdict(int)
    for node in report['nodes'].values():
        counts[node['owner']] += 1
    print(json.dumps(dict(ownership_nodes=dict(counts), maps=len(report['map_owners']),
                          excluded_maps=len(report['excluded_maps']), unresolved=len(report['unresolved']),
                          report=str(args.output), readiness=report['readiness']), indent=2))
    if args.require_assigned and any(x['kind'] == 'unassigned_map' for x in report['unresolved']):
        parser.exit(1, 'Unassigned maps remain; inspect the report.\n')


if __name__ == '__main__':
    main()
