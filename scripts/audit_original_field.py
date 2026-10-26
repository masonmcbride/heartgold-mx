#!/usr/bin/env python3
"""Batch original-owner reachability. Never substitutes host symbols or executes code.

All conditional arms and ambiguous definitions are retained. Indirect calls,
resource selection and native/ARM lifetime contracts remain explicit review work.
The report is a conservative source inventory, not a closed runtime contract.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import hashlib
import re
from collections import defaultdict

from audit_chapters import Graph
from dependency_closure import original_dependencies, source_index
from generated_files import write_text_if_changed

ROOTS = {
    'constructor': ('FieldSystem_New', 'FieldSystem_Delete'),
    'process': ('FieldSystem_Main', 'FieldSystem_Control', 'FieldSystem_LoadFieldOverlayInternal'),
    'parent': ('FieldMap_Init', 'FieldMap_Main', 'FieldMap_Exit', 'ov01_02206378'),
    'input': ('FieldInput_Process',),
    'scripts': ('StartMapSceneScript', 'SetupScriptEngine', 'Task_RunScripts'),
    'field_entry': ('CallFieldTask_NewGame', 'CallFieldTask_ContinueGame_Normal',
                    'sub_02052F94', 'sub_02053284', 'sub_0205316C', 'sub_02055408'),
    'field_objects': ('FollowMon_InitMapObject','sub_0205FD30','sub_0205F12C'),
    'encounter': ('SetupAndStartTrainerBattle',),
}
RESOURCE_ROOTS = {
    'map_load': ('MapLoadManager_New', 'MapLoadManager_InitialLoad', 'MapLoadManager_Tick',
                 'MapLoadManager_RenderLoadedMaps', 'MapLoadManager_End', 'MapLoadManager_HasEnded',
                 'MapLoadManager_FreeNARCAndLoadedMapBuffers'),
    'area': ('AreaDataManager_Alloc', 'AreaDataManager_Load', 'AreaDataManager_Free'),
    'effect_raw_resources': ('GF3dGfxRawResMan_Create', 'GF3dGfxRawResMan_AllocObjAndKeys',
        'GF3dGfxRawResMan_LoadObjTexById', 'GF3dGfxRawResMan_FreeObjVramAndSecondaryHeaderById',
        'GF3dGfxRawResMan_FreeObjById', 'GF3dGfxRawResMan_Destroy',
        'GF3dGfxRawResObj_GetTex', 'GF3dGfxRawResObj_GetTexKey',
        'GF3dGfxRawResObj_GetTex4x4Key', 'GF3dGfxRawResObj_GetPlttKey',
        'ov01_021F1930', 'ov01_021F197C', 'ov01_021F19B4', 'ov01_021F19D0'),
    'bottom_screen': ('FieldSystem_InitBottomScreen', 'FieldSystem_EndBottomScreen',
                      'FieldSystem_IsBottomScreenDone'),
    'render_objects': ('Field3dRenderObjManager_New', 'Field3dRenderObjManager_Delete',
                       'Field3dRenderObjManager_AllocRenderObj', 'Field3dRenderObjManager_GetOrAllocRenderObjByID',
                       'Field3dRenderObjManager_FreeRenderObj', 'Field3dObjectList_GetRenderObjectByID',
                       'Field3dRenderObjManager_IsResFileHeaderLoadedByIndex'),
}

# These are contract cuts, not declarations that existing providers suffice.
# The unrestricted report is always emitted too, including failure/reset paths.
CONTRACTS = {
    **{name: 'src/compat/native_heap.c: checked native budgets/ownership/destruction; DS fragmentation/layout equivalence is not claimed'
       for name in ('Heap_Alloc', 'Heap_AllocAtEnd', 'Heap_Free', 'Heap_FreeExplicit', 'Heap_Create', 'Heap_Destroy')},
    'GF_ASSERT': 'src/compat/native.h: native fatal assertion; original error/reset application is outside this feasibility entry',
    'HandleLoadOverlay': 'src/compat/overlays.c: checked normal/async static residency and hooks; protection/field module implementations remain open',
    'UnloadOverlayByID': 'src/compat/overlays.c: checked residency; original module set remains open',
}


def sdk_contract(name):
    if name in CONTRACTS:
        return CONTRACTS[name]
    if name.startswith(('NNS_', 'GX_', 'GXS_', 'G2_', 'G2S_', 'G3_', 'G3X_', 'G3B_',
                        'G3C_', 'G3BS_', 'G3CS_', 'OS_', 'FS_', 'MI_', 'DC_', 'IC_',
                        'FX_', 'VEC_', 'MTX_', 'PAD_', 'TP_', 'RTC_', 'PM_', 'SND_', 'SPI_')):
        return 'SDK machine/resource contract; provider equivalence and native ABI not established'
    return None


def inventory(definitions, roots, bounded=False):
    graph = Graph()
    opened = set()
    pending = [(name, None, None, group) for group, names in roots.items() for name in names]
    expanded = set()
    while pending:
        name, local, user, group = pending.pop()
        contract = sdk_contract(name) if bounded else None
        if contract:
            key = 'contract:' + name
            graph.node(key, 'unverified_platform_contract', symbol=name, review=contract)
            opened.add((user or group, 'platform_contract', name))
            if user:
                graph.edge(user, key)
            else:
                graph.seeds[group].add(key)
            continue
        rows = definitions.get(name, [])
        # C static names are scoped to their original unit. Without a compiler
        # context, retain competing external/conditional candidates explicitly.
        scoped = [r for r in rows if r['source'] == local]
        if scoped:
            rows = scoped
        if not rows:
            key = 'missing:' + name
            graph.node(key, 'missing_definition', symbol=name)
            opened.add((user or group, 'missing_definition', name))
            if user:
                graph.edge(user, key)
            else:
                graph.seeds[group].add(key)
        if len(rows) > 1:
            opened.add((user or group, 'ambiguous_definition', name))
        for row in rows:
            key = row['source'] + ':' + name + ':' + str(row['line'])
            graph.node(key, row['kind'], symbol=name, source=row['source'], line=row['line'])
            if user:
                graph.edge(user, key)
            else:
                graph.seeds[group].add(key)
            if key in expanded:
                continue
            expanded.add(key)
            calls, refs, indirect = original_dependencies(row, definitions)
            for target in indirect:
                opened.add((key, 'indirect_dispatch', target))
            for target in sorted(calls | refs):
                if target != name:
                    pending.append((target, row['source'], key, group))
    # Open edges participate in reachability and SCC outgoing traversal too.
    for user, kind, target in sorted(opened):
        if user in graph.nodes:
            key = 'open:' + kind + ':' + target
            graph.node(key, 'open_dependency', symbol=target)
            graph.edge(user, key)
    _, components, consumers = graph.partition()
    for key, value in graph.nodes.items():
        value['consumers'] = consumers[key]
    return dict(
        complete=False, bounded_platform_view=bounded,
        evidence='Conservative original-source reachability; no host substitution, compilation or execution claim.',
        limitations=['Indirect dispatch requires table/store and runtime target proofs.',
                     'Macros are not preprocessed; conditional candidates are retained.',
                     'Resource IDs, initialization, ABI and publication/lifetime contracts require review.'],
        roots={k: sorted(v) for k, v in graph.seeds.items()},
        nodes=graph.nodes, edges={k: sorted(v) for k, v in graph.edges.items()},
        cycles=[c for c in components if len(c) > 1],
        open_edges=[dict(user=u, kind=k, target=t) for u, k, t in sorted(opened)],
    )



# Lifecycle roots deliberately include teardown and recurring callbacks. These
# are conservative source seeds, not a claim that LOAD dispatches Main/Exit.
FRONTIER_ROOTS = {
    'initialization': ('FieldMap_Init', 'ov01_021E6580'),
    'callbacks': ('FieldMap_VBlankCallback', 'ov01_021E5FC0', 'ov01_021E6220'),
    'teardown': ('FieldMap_Exit',),
}


def frontier_capability(name):
    if re.search(r'G2d|2DGfx|Sprite|Oam|OAM|ObjChar|ObjPltt',name):
        return 'sprite_resources_renderer_and_oam'
    if re.search(r'Window|MsgData|Message|String',name):
        return 'window_pixels_and_encoded_messages'
    if re.search(r'FieldBGM|Sound|Snd|BGM',name):
        return 'audio_sequence_and_soundplate_lifecycle'
    if re.search(r'Camera|MTX|FX_|VEC_|Glb|ModelAttributes|Fog|Light', name):
        return 'geometry_state_and_environment_records'
    if re.search(r'Weather|Gymmick|Signpost|MapName|TextureManager', name):
        return 'environment_resource_owners'
    if re.search(r'HBlank|VBlank|SysTask|IRQ|Irq|Overlay|BottomScreen', name):
        return 'asynchronous_lifecycle_and_interrupts'
    if re.search(r'Script|ScrCmd|Input|Movement|Player|MapObject', name):
        return 'canonical_objects_scripts_and_input'
    if sdk_contract(name) or re.search(r'Heap|NARC|Resource|Alloc|Free', name):
        return 'memory_resource_and_device_transport'
    return 'original_execution_and_typed_owner_abi'


def cross_reference_frontier(report, definitions, generated, project, native_report=None):
    """Add evidence to the existing graph; never resolve by symbol spelling alone."""
    compiled_public=(native_report or {}).get('public', {})
    native = defaultdict(list)
    for path in sorted((project / 'src').rglob('*.c')):
        for name in re.findall(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\)\s*\{', path.read_text(), re.M):
            native[name].append(str(path.relative_to(project)))
    emitted = defaultdict(list)
    opened = defaultdict(list)
    for path in sorted(generated.glob('*provenance.json')):
        data = json.loads(path.read_text())
        rows = data if isinstance(data, list) else data.get('functions', [])
        for row in rows:
            if 'function' in row:
                emitted[(row.get('source'), row['function'])].append(path.name)
        if isinstance(data, dict):
            for row in data.get('open_contracts', []):
                opened[row['function']].append(dict(manifest=path.name, **row))
    exports = defaultdict(list)
    imports = defaultdict(list)
    for path in sorted(generated.glob('*guest.inc')):
        data = path.read_text()
        prefixes=re.findall(r'^#define (HG_\w+)_CODE_SIZE \d+u$',data,re.M)
        for full in re.findall(r'^#define (HG_\w+) \d+u$', data, re.M):
            for prefix in prefixes:
                prefix += '_'
                if full.startswith(prefix) and full[len(prefix):]!='CODE_SIZE' and not full[len(prefix):].startswith('IMPORT_'):
                    exports[full[len(prefix):]].append(path.name)
                    break
        for name in re.findall(r'\{\s*\d+u,"(\w+)"', data):
            imports[name].append(path.name)
    dispatch_source = (project/'src/compat/original_loader.c')
    dispatch_body = dispatch_source.read_text().split('static int boundary_import(DSThumb *c,HGOriginalLoader *l) {',1)[-1].split('static int call_result',1)[0] if dispatch_source.exists() else ''
    dispatch_mentions = set(re.findall(r'"(\w+)"', dispatch_body))
    seed_keys = {key for keys in report['roots'].values() for key in keys}
    callers = defaultdict(list)
    for caller, targets in report['edges'].items():
        for target in targets: callers[target].append(caller)
    groups = defaultdict(list)
    for key, node in report['nodes'].items():
        name = node.get('symbol', key)
        records = opened.get(name, [])
        original = emitted.get((node.get('source'), name), [])
        status = ('generated_rejecting_boundary' if any('rejects' in r.get('status', '') for r in records)
                  else 'guest_image_export' if exports.get(name)
                  else 'original_native_body_emitted' if original
                  else 'compiled_native_provider_candidate' if compiled_public.get(name) or compiled_public.get('hg_original_'+name)
                  else 'native_provider_candidate' if native.get(name)
                  else 'unresolved_dispatch' if node['kind'] == 'open_dependency'
                  else 'platform_contract_unverified' if sdk_contract(name)
                  else 'original_source_only')
        capability = frontier_capability(name)
        row = next((r for r in definitions.get(name, []) if r.get('source')==node.get('source') and r.get('line')==node.get('line')), None)
        calls, refs, indirect = original_dependencies(row, definitions) if row else (set(), set(), set())
        relationships = []
        for target in report['edges'].get(key, []):
            symbol=report['nodes'][target].get('symbol', target)
            relationships.append(dict(target=target, relationship=
                'call_or_tail_branch' if symbol in calls else
                'address_or_data_reference' if symbol in refs else 'open_dependency'))

        node.update(callers=sorted(callers[key]), dependencies=relationships, implementation_status=status,
                    original_abi=(row['body'].split('{',1)[0].strip() if row and row['kind']=='function' else 'original ARM32 registers/stack or original data layout; typed native adapter required'),
                    implementation_evidence=dict(original_native_manifests=original,
                        guest_exports=exports.get(name, []), guest_imports=imports.get(name, []),
                        native_candidates=native.get(name, []), generated_boundaries=records,
                        compiled_native_candidates=compiled_public.get(name, [])+compiled_public.get('hg_original_'+name, []),
                        loader_dispatch_name_mentioned=name in dispatch_mentions),
                    reachability=('unresolved' if node['kind'] in ('open_dependency', 'missing_definition')
                                  else 'statically_confirmed' if key in seed_keys else 'conditionally_reachable'),
                    dependency_evidence='statically confirmed source reference; branch and indirect target execution not established',
                    required_semantics=['bounded live guest identity and coherent aliases',
                        'ARM32 record layout or explicit typed native adapter',
                        'original callback ordering and ownership through teardown'] +
                        ([sdk_contract(name)] if sdk_contract(name) else []),
                    shared_capability=capability,
                    capability_obligations={
                        'sprite_resources_renderer_and_oam':'original ARM32 records, resource/VRAM leases, animation callbacks, OAM reservations and bounded device publication; Metal presentation separate',
                        'window_pixels_and_encoded_messages':'original pixel ownership and encoded String/message resource representation; native preview records are not binary ABI substitutes',
                        'audio_sequence_and_soundplate_lifecycle':'canonical save/map selection, original sequence state and asynchronous sound ownership; existing native audio provider must be explicitly integrated',
                        'geometry_state_and_environment_records':'fixed-point signs/widths, software global state versus submitted commands, camera histories/retained target lifetime',
                        'environment_resource_owners':'constructor publication, save/time/resource selection, dependent tasks, partial failure and asynchronous destruction',
                        'asynchronous_lifecycle_and_interrupts':'callback ISA/argument identity, priority/phase/IRQ ordering, cancellation and retained owner lifetime',
                        'canonical_objects_scripts_and_input':'canonical array slots, scalar/vector/opaque-work ABI, callback tables and script/input gating',
                        'memory_resource_and_device_transport':'allocation outcome/accounting/alignment, region lifetime, archive bounds, device side effects and DMA visibility',
                        'original_execution_and_typed_owner_abi':'original instruction/data semantics and explicit layout/calling-convention verification',
                    }[capability])
        if status not in ('original_native_body_emitted', 'guest_image_export'):
            groups[capability].append(key)
    # Direct address literals are observations/candidates, not ordinary RAM grants.
    hardware = []
    for key, node in report['nodes'].items():
        for row in definitions.get(node.get('symbol'), []):
            if row.get('source') != node.get('source'): continue
            for address in sorted(set(re.findall(r'0x04[0-9A-Fa-f]{6}\b|reg_[A-Za-z_]\w*', row['body']))):
                hardware.append(dict(operation='MMIO address reference '+address,
                    callers=[key], implementation_status='requires_device_review',
                    reachability='conditionally_reachable',
                    required_semantics='width, side effects, ordering, IRQ/DMA and register ownership; literal alone does not prove access',
                    shared_capability='memory_resource_and_device_transport'))
    report.update(missing_or_unverified_by_capability=dict(groups), hardware_operations=hardware,
        direct_frontier={key:report['edges'].get(key,[]) for key in sorted(seed_keys)},
        native_compiler_evidence=(native_report or {}).get('evidence', 'not supplied; source candidates only'),
        cross_reference_limitations=['Native symbol candidates do not establish ABI equivalence or linkage.',
            'Guest export means assembled code, not transitive import support or execution.',
            'Emitted original bodies may retain explicit rejecting boundaries.',
            'All static edges are conservative; runtime traces remain separate evidence.'])
    return report


def add_runtime_evidence(report, generated, trace):
    names=defaultdict(set)
    for path in generated.glob('*guest.inc'):
        data=path.read_text()
        prefixes=re.findall(r'^#define (HG_\w+)_CODE_SIZE \d+u$',data,re.M)
        for full, value in re.findall(r'^#define (HG_\w+) (\d+)u$',data,re.M):
            for prefix in prefixes:
                if full.startswith(prefix+'_'): names[int(value)].add(full[len(prefix)+1:])
    counts=defaultdict(Counter);events=[]
    for line in trace.read_text().splitlines():
        if 'HG_ORIGINAL_TRACE ' in line:
            event=json.loads(line.split('HG_ORIGINAL_TRACE ',1)[1]);events.append(event)
            symbols={event['symbol']} if 'symbol' in event else names.get(event.get('entry'),set())
            for symbol in symbols: counts[symbol][event['event'] + ('_result_'+str(event['result']) if 'result' in event else '')]+=1
    for node in report['nodes'].values():
        node['runtime_evidence']=dict(counts.get(node.get('symbol'),{}))
    resolved=[]
    for symbol in ('native_default_free_texture','native_default_free_palette'):
        observed=[e for e in events if e.get('symbol')==symbol]
        if observed: resolved.append(dict(service=symbol,requests=len(observed),
            callers_lr=sorted({e['lr'] for e in observed}),
            dependency='NNS_GfdDefaultFuncFreeTexVram' if symbol.endswith('texture') else 'NNS_GfdDefaultFuncFreePlttVram',
            semantics='checked SDK key release; synthetic callable identity, not byte-compatible RAM'))
    report['runtime_trace']=dict(path=str(trace),events=len(events),
        resolved_indirect_services=resolved,
        symbols={name:dict(count) for name,count in sorted(counts.items())},
        outside_static_symbols=sorted(set(counts)-{n.get('symbol') for n in report['nodes'].values()}),
        limitations='Entry/return and import-request observations only. Internal direct calls and untaken branches are not traced; aliases of one PC are not disambiguated. A return event includes its interpreter result, not a blanket success claim.')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', type=Path, default=Path('upstream/pokeheartgold'))
    parser.add_argument('--output', type=Path, default=Path('build/audit/original-field/closure.json'))
    parser.add_argument('--refresh-frontier', type=Path, help='reuse an existing original graph; refresh implementation/trace evidence without rediscovery')
    parser.add_argument('--native-calls', type=Path, help='current dependency_closure.native_calls report; does not establish ABI equivalence')
    parser.add_argument('--trace', type=Path, help='optional HG_TRACE_ORIGINAL execution log; never changes static reachability')
    parser.add_argument('--frontier', action='store_true', help='cross-reference initialization, callbacks and teardown using existing source graph')
    parser.add_argument('--resources', action='store_true', help='inventory resource-manager roots separately')
    parser.add_argument('--generated', type=Path, default=Path('build/generated'))
    parser.add_argument('--measure-resource-units', type=Path, metavar='CLANG',
                        help='compile original Thumb resource units with existing assembler; never execute them')
    args = parser.parse_args()
    definitions = source_index(args.upstream, args.output.parent / 'original-source-index.json', include_host=False)
    roots = FRONTIER_ROOTS if args.frontier else RESOURCE_ROOTS if args.resources else ROOTS
    report = json.loads(args.refresh_frontier.read_text()) if args.refresh_frontier else inventory(definitions, roots)
    if args.refresh_frontier:
        if report.get('roots') is None or report.get('complete') is not False:
            raise ValueError('refresh requires an existing conservative original graph')
        report['refreshed_from']=str(args.refresh_frontier)
        report['graph_reuse_assumption']='Original dependency graph retained; upstream source must remain unchanged. Implementation and runtime evidence are refreshed separately.'
    if args.frontier:
        report = cross_reference_frontier(report, definitions, args.generated, Path(__file__).resolve().parents[1],
            json.loads(args.native_calls.read_text()) if args.native_calls else None)
        if args.native_calls: report['native_compiler_report']=str(args.native_calls)
    if args.trace:
        report = add_runtime_evidence(report, args.generated, args.trace)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_text_if_changed(args.output, json.dumps(report, indent=2, sort_keys=True) + '\n')
    bounded = None if args.refresh_frontier else inventory(definitions, roots, bounded=True)
    if bounded:
        write_text_if_changed(args.output.with_name('platform-contracts.json'), json.dumps(bounded, indent=2, sort_keys=True) + '\n')
    summary = dict(nodes=len(report['nodes']), cycles=len(report['cycles']),
                   open_edges=dict(Counter(e['kind'] for e in report['open_edges'])),
                   per_root={g: sum(g in n['consumers'] for n in report['nodes'].values()) for g in roots},
                   bounded_nodes=len(bounded['nodes']) if bounded else None,
                   bounded_per_root={g: sum(g in n['consumers'] for n in bounded['nodes'].values()) for g in roots} if bounded else None,
                   complete=report['complete'], report=str(args.output))
    if args.measure_resource_units:
        from guest_assembly import assemble
        measurements = []
        for symbol in ('MapLoadManager_New', 'AreaDataManager_Alloc', 'FieldSystem_InitBottomScreen'):
            rows = definitions.get(symbol, [])
            if len(rows) != 1 or rows[0]['kind'] != 'assembly':
                raise ValueError(('resource unit ambiguity', symbol, rows))
            relative = rows[0]['source']; raw = (args.upstream / relative).read_text()
            image, code, imports = assemble(raw, args.upstream, args.output.parent,
                                            args.measure_resource_units, symbol.lower(), 'HG_MEASURE_' + symbol,
                                            include_dirs=(args.generated,))
            measurements.append(dict(source=relative, sha256=hashlib.sha256(raw.encode()).hexdigest(),
                                     code_bytes=code, image_bytes=len(image), executed=False,
                                     imports={name: [dict(source=r['source'], line=r['line'], kind=r['kind'])
                                                     for r in definitions.get(name, [])] for name in imports}))
        write_text_if_changed(args.output.with_name('resource-unit-measurements.json'),
                              json.dumps(dict(complete=False, units=measurements,
                                              evidence='Whole-unit assembly and recursive source inventory, not an implemented or executed ABI bridge'), indent=2, sort_keys=True) + '\n')
        summary['assembly_measurements'] = [{k: v for k, v in m.items() if k not in ('imports', 'sha256')} |
                                            dict(imports=len(m['imports'])) for m in measurements]
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
