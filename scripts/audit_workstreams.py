#!/usr/bin/env python3
"""Group measured compiler barriers; index original functions without inventing coverage."""
import argparse
import collections
import csv
import json
import pathlib
import re
from audit_reuse import c_functions

# A finite set of implementation contracts, not a promise that one patch fixes
# every member. Unrecognized diagnostics remain explicit review work.
CONTRACTS = {
    'resources': ('Generated resources', 'Generate message/archive indices from original manifests through the existing preparation pipeline.', 'Every referenced generated include resolves; archive indices match actual members.'),
    'abi': ('Native/guest ABI and declarations', 'Share audited scalar/record declarations; keep guest addresses and native pointers distinct. Replace header suppression only as its real interface becomes available.', 'Independent size/offset vectors, pointer round trips and boundary failure tests.'),
    'graphics': ('Graphics and UI services', 'Translate original GX/NNS state and resource lifetimes into the existing DS decoders and host renderer; retain original scene/menu control.', 'Scene/state comparisons and connected UI tests, including resource destruction.'),
    'audio': ('Audio services', 'Provide a shared sound archive/player backend with real handle lifetimes and completion semantics.', 'Decoded sound and sequence output plus original wait/completion behavior.'),
    'platform': ('Clock, tasks, input and filesystem', 'Bind original interfaces to host clocks, existing task queues, bounded resource reads and input; replace hardware register paths at the boundary.', 'Deterministic task/input/time scenarios and bounded IO failure tests.'),
    'storage': ('Persistent game state', 'Give original save blocks stable typed ownership and implement serialization/checksum/atomic host persistence at the save boundary.', 'New-game/save/load round trips, corruption rejection and cancellation rollback.'),
    'network': ('Communications and peripherals', 'Isolate wireless, online, microphone and accessory backends behind explicit capability contracts; unavailable operations must fail visibly.', 'Supported service scenarios and deterministic unavailable-service outcomes.'),
    'cpu': ('Original ARM/Thumb execution', 'Reuse the guest execution engine and typed import bridge; select a broader CPU backend only against measured missing instructions and workloads.', 'Assemble for the guest target, execute differential vectors and validate every guest/native import.'),
    'gameplay': ('Original subsystem integration', 'Build coherent original call closures on the shared services; connect field, scripts, party, menus and battles through owned state.', 'Continuous gameplay scenarios through actual original entry points.'),
    'review': ('Unclassified compiler barrier', 'Inspect the recorded diagnostic, then assign a verified boundary rather than supplying a guessed type or success stub.', 'No unclassified first barriers remain; rerun to expose deeper dependencies.'),
}


def family(token):
    token = token.lower()
    if re.search(r'\.naix|msg_\d+\.h', token): return 'resources'
    if re.search(r'nnssnd|sndhandle|sound|nitro/snd|sndarc', token): return 'audio'
    if re.search(r'dwc|nitrowifi|wireless|wvr|wcm|mic_|spi.*mic|pokewalker', token): return 'network'
    if re.search(r'nnsys|nitro/gx|nitro/fx|gx[a-z_]|nns[g_]|g2d|g3d|fx32|m[ix]x|vecfx', token): return 'graphics'
    if re.search(r'nitro/(?:rtc|os|mi|spi|fs|hw)|rtcd|rtct|irq|fsfile|fsoverlay|os[a-z_]|mi_cpu|narc', token): return 'platform'
    if re.search(r'player_name_length|pokemon_name_length|party_size|string|window|bgconfig|heapid|localmapobject|bgevent|msgdata_load|nitro\.h|null\.h', token): return 'abi'
    if re.search(r'save_|savedata|flash|backup', token): return 'storage'
    return 'review'


def generate(upstream, audit):
    data = json.loads((audit/'summary.json').read_text())
    units, functions = [], []
    # Original-header pass is a control, not a second set of remaining units.
    for row in data['results']:
        if row['mode'] == 'unadapted': continue
        path = upstream / row['source']
        text = path.read_text(errors='replace')
        diagnostics = (audit/row['log']).read_text(errors='replace')
        errors = re.findall(r'(?:fatal )?error: (.*)', diagnostics)
        evidence = [row['missing_header']] if row['missing_header'] else []
        evidence += errors
        families = sorted({family(t) for t in evidence if t})
        if row['mode'] == 'assembly':
            primary = 'cpu'
            families = sorted(set(families) | {'cpu'})
        elif row['compiled']:
            primary, families = 'gameplay', ['gameplay']
        else:
            primary = family(row['missing_header'] or (errors[0] if errors else ''))
        unit = {k: row[k] for k in ('source','group','mode','compiled','category','missing_header','first_error','log')}
        unit.update(primary_workstream=primary, visible_workstreams=families, errors=errors)
        units.append(unit)
        names = ([(name, 'C lexical definition') for name, _ in c_functions(text)]
                 if row['mode'] == 'compat' else
                 [(name, 'assembly function marker') for name in re.findall(r'^\s*(?:thumb_func_start|arm_func_start)\s+(\w+)', text, re.M)])
        for name, kind in names:
            functions.append({'source':row['source'], 'function':name, 'index_kind':kind,
                              'unit_compiled':row['compiled'], 'primary_workstream':primary,
                              'execution':'not measured by this survey'})
    assert len(units) == len({u['source'] for u in units}), 'Duplicate manifest sources need explicit treatment'
    counts = collections.Counter(u['primary_workstream'] for u in units)
    missing = collections.Counter(u['missing_header'] for u in units if u['missing_header'])
    payload = {'revision':data['revision'], 'created_utc':data['created_utc'],
               'upstream_inputs_unchanged':data['upstream_inputs_unchanged'],
               'provider_libraries':data.get('provider_libraries', {}),
               'limitations':['Compiler diagnostics are capped and expose only current barriers.',
                              'C definitions are a lexical index, not a preprocessor-aware complete call graph.',
                              'A failed unit can contain already integrated function slices.',
                              'Compiled units and provider symbols do not establish linkage or execution.',
                              'Primary categories group current evidence; shared fixes expose deeper barriers.'],
               'contracts':{k:dict(zip(('name','solution','acceptance'),v)) for k,v in CONTRACTS.items()},
               'units':units, 'functions':functions,
               'unresolved_symbols_from_successful_units':data['undefined_symbols']}
    (audit/'workstreams.json').write_text(json.dumps(payload,indent=2)+'\n')
    with (audit/'remaining-functions.csv').open('w') as f:
        writer=csv.DictWriter(f,fieldnames=list(functions[0]));writer.writeheader();writer.writerows(functions)
    lines=['# Full-build workstreams','',f"Revision `{data['revision']}`; {len(units)} selected source units; {len(functions)} lexical C definitions / assembly markers indexed.",'',
           'This is a bounded set of solution contracts, not a bounded count of remaining patches. Diagnostics reveal current barriers only. No full-game link or execution is claimed. Failed whole units may already supply executed slices. All per-unit errors and indexed functions are in `workstreams.json` and `remaining-functions.csv`.','',
           '| Workstream | Units assigned by current evidence | Shared solution | Completion evidence |','| --- | ---: | --- | --- |']
    for key,(name,solution,acceptance) in CONTRACTS.items():
        lines.append(f'| {name} | {counts[key]} | {solution} | {acceptance} |')
    lines += ['', 'Compiled C units are assigned to integration, because compilation alone does not retire their runtime obligations. Assembly is assigned to guest execution even when preprocessing stops first. Categories are implementation workstreams, not estimates or independent fixes.', '', '## Highest fan-out visible includes', '', '| Missing include | Units |', '| --- | ---: |']
    lines += [f'| `{name}` | {count} |' for name,count in missing.most_common(20)]
    lines += ['', '## Sequence', '',
              '1. Resolve shared ABI/header and generated-resource dependencies using actual interfaces; rerun the survey after each shared change.',
              '2. Build original call closures and resolve imports against all native archives; select guest execution for assembly, never feed ARM instructions to the host assembler as a proposed fix.',
              '3. Connect platform, graphics, audio and persistent-state contracts to original subsystem owners.',
              '4. Validate connected field/starter/follower gameplay, then menus, battles, saving and transitions. Compilation is a prerequisite, not the acceptance test.',
              '5. Audit optional communications/peripherals against the full-game scope and retain explicit unsupported outcomes until implemented.', '',
              'The machine-readable list retains an explicit review category instead of guessing services from ambiguous diagnostics. Rerun after fixes; first-error counts cannot predict the complete remaining workload.','']
    (audit/'WORKSTREAMS.md').write_text('\n'.join(lines))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream',type=pathlib.Path,required=True)
    parser.add_argument('--audit',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if args.audit.resolve().is_relative_to(args.upstream.resolve()):
        parser.error('Output must be outside upstream')
    generate(args.upstream.resolve(),args.audit.resolve())
    print(f'Grouped workstreams: {args.audit / "WORKSTREAMS.md"}')

if __name__=='__main__': main()
