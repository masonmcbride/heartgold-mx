#!/usr/bin/env python3
"""Read-only reuse inventory of built symbols and explicit guest boundaries.

Does not execute game functions or infer execution coverage from test linkage.
Source call lists are lexical hints, not a complete call graph or boot closure.
"""
import argparse
import collections
import csv
import hashlib
import json
import pathlib
import re
import shlex
import subprocess

from audit_native import manifest


def command(args):
    return subprocess.check_output(args, text=True, stderr=subprocess.PIPE)


def symbols(path):
    """Mach-O text definitions, including static functions; no data symbols."""
    return [(m[2], m[1]) for line in command(['xcrun', 'nm', str(path)]).splitlines()
            if (m := re.match(r'^\s*[0-9a-fA-F]+\s+([tT])\s+(.+)$', line))]


def decision(source, name):
    if source.startswith('upstream/'):
        return 'original game', 'keep original implementation', 'Clang / guest CPU where necessary'
    if source.startswith('tests/'):
        return 'test', 'retain behavioral evidence', 'existing test tools'
    if source == 'src/ds_thumb.c':
        return 'CPU', 'retain bounded baseline; evaluate replacement for expansion', 'Dynarmic spike'
    if source in ('src/motion_guest.c', 'src/terrain_guest.c'):
        return 'guest/native ABI', 'retain typed boundary; CPU library cannot supply game ABI', 'custom bridge'
    if source.startswith('src/ds_'):
        if source in ('src/ds_resources.c', 'src/ds_texture.c'):
            return 'shared formats', 'retain tested runtime; compare offline before extending formats', 'ndspy offline oracle'
        if source == 'src/ds_raster.c':
            return 'graphics', 'retain diagnostic oracle; evaluate Metal for production raster', 'Metal; melonDS external reference'
        return 'graphics/data', 'retain supported semantics; reuse only a verified matching component', 'existing DS decoder / Metal'
    if source.startswith('src/compat/'):
        if name.startswith(('_Heap_', '_MI_Cpu')):
            return 'SDK boundary', 'already delegates allocation/copy/clear to libc; retain policy boundary', 'libc'
        return 'SDK/game boundary', 'retain explicit semantics; widen only for reachable original paths', 'custom adapter; libnds reference only'
    if source.endswith('.m'):
        return 'host', 'retain working host frontend', 'AppKit / Metal / GameController'
    return 'game integration', 'retain; no candidate library supplies HeartGold lifecycle', 'original game + custom ownership'


def strip_c(text):
    # Keep offsets stable so extracted spans still identify the source function.
    return re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text, flags=re.S)


def c_functions(text):
    clean = strip_c(text)
    pattern = r'^\s*(?:[\w*]+\s+)+\**\s*([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{'
    for match in re.finditer(pattern, clean, re.M):
        start = match.end() - 1
        depth, end = 1, start + 1
        while end < len(clean) and depth:
            depth += (clean[end] == '{') - (clean[end] == '}')
            end += 1
        yield match[1], clean[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=pathlib.Path, default=pathlib.Path('build'))
    parser.add_argument('--upstream', type=pathlib.Path, default=pathlib.Path('upstream/pokeheartgold'))
    args = parser.parse_args()
    project = pathlib.Path(__file__).resolve().parents[1]
    build, upstream = args.build.resolve(), args.upstream.resolve()
    # Outputs have a fixed location inside the supplied ignored build tree.
    if build.parent != project or not build.name.startswith('build'):
        parser.error('--build must be a project build* directory')
    out = build / 'reuse-audit'
    out.mkdir(exist_ok=True)
    database = json.loads((build / 'compile_commands.json').read_text())
    products = [build / n for n in ('heartgold_native', 'heartgold_metal', 'heartgold_field_viewer')]
    products += sorted(p for p in build.glob('test_*') if p.is_file() and not p.suffix)
    exports = {p.name: {n for n, binding in symbols(p) if binding == 'T'} for p in products}
    rows, objects, files = [], [], []
    for entry in database:
        path = pathlib.Path(entry['file']).resolve()
        if path.is_relative_to(upstream):
            source = 'upstream/' + path.relative_to(upstream).as_posix()
        else:
            source = path.relative_to(project).as_posix() if path.is_relative_to(project) else str(path)
        argv = entry.get('arguments') or shlex.split(entry['command'])
        output = pathlib.Path(entry.get('output') or argv[argv.index('-o') + 1])
        if not output.is_absolute():
            output = pathlib.Path(entry['directory']) / output
        if not output.is_file():
            raise ValueError(f'Build is incomplete: {output}')
        definitions = symbols(output)
        objects.append({'source': source, 'object': str(output.relative_to(build)),
                        'sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
                        'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest()})
        files.append({'source': source, 'physical_lines': len(path.read_text().splitlines()),
                      'layer': decision(source, '')[0]})
        for name, binding in definitions:
            layer, action, reuse = decision(source, name)
            # Static names recur across translation units. Never attribute a
            # binary's static symbol to a particular object by name alone.
            linked = [binary for binary, names in exports.items() if binding == 'T' and name in names]
            rows.append({'translation_unit': source, 'symbol': name, 'binding': binding,
                         'layer': layer, 'action': action, 'reuse': reuse,
                         'linked_products': ';'.join(n for n in linked if not n.startswith('test_')),
                         'linked_tests': ';'.join(n for n in linked if n.startswith('test_')),
                         'execution': 'not measured by this audit'})
    rows.sort(key=lambda r: (r['translation_unit'], r['symbol']))
    with (out / 'functions.csv').open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    c_sources = manifest(upstream, 'src/meson.build', 'pokeheartgold_c')
    asm_sources = manifest(upstream, 'asm/meson.build', 'pokeheartgold_asm')
    providers, bodies = collections.defaultdict(list), {}
    for source in c_sources:
        for name, body in c_functions(source.read_text()):
            providers[name].append(str(source.relative_to(upstream)))
            bodies[(source.name, name)] = body
    for source in asm_sources:
        text = source.read_text()
        for name in re.findall(r'^\s*(?:thumb_func_start|arm_func_start)\s+(\w+)', text, re.M):
            providers[name].append(str(source.relative_to(upstream)))

    guest = json.loads((build / 'generated/field_guest_provenance.json').read_text())
    text = (project / 'src/motion_guest.c').read_text()
    admitted = set(re.findall(r'!strcmp\(name,\s*"(\w+)"\)', text))
    admitted |= set(re.findall(r'^\s*(?:GET_AVATAR|GET_OBJECT|SET_OBJECT|TILE_PREDICATE)\((\w+)\)', text, re.M))
    imports = [{'symbol': name, 'address': address,
                'boundary': 'conditional provider present; execution not inferred' if name in admitted else 'no provider; executed call fails',
                'game_provider_hints': providers.get(name, [])}
               for name, address in sorted(guest['imports'].items())]
    roots = []
    for filename, name in [('main.c', 'NitroMain'), ('main.c', 'main'),
                           ('fieldmap.c', 'FieldMap_Init'), ('fieldmap.c', 'FieldMap_Main'), ('fieldmap.c', 'FieldMap_Exit')]:
        body = bodies.get((filename, name))
        if body is None:
            continue
        calls = sorted(set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', body)) - {'if', 'for', 'while', 'switch', 'sizeof'})
        roots.append({'symbol': name, 'direct_call_hints': [
            {'symbol': call, 'game_provider_hints': providers.get(call, []),
             'native_viewer_symbol_present': '_' + call in exports.get('heartgold_field_viewer', set())}
            for call in calls]})
    totals = collections.Counter(r['layer'] for r in files)
    unique_files = {row['source']: row for row in files}
    custom_lines = sum(r['physical_lines'] for r in unique_files.values()
                       if not r['source'].startswith(('upstream/', 'tests/')))
    compiled_game = sorted({r['source'] for r in files if r['layer'] == 'original game'})
    transition_path=build/'generated/transition_provenance.json'
    transition=json.loads(transition_path.read_text()) if transition_path.is_file() else {}
    motion_path=build/'generated/motion_provenance.json'
    motion=json.loads(motion_path.read_text()) if motion_path.is_file() else {}
    story_path=build/'generated/script_kernel_provenance.json'
    story=json.loads(story_path.read_text()) if story_path.is_file() else {}
    included=sorted({p for provenance in (transition,motion,story) for p in provenance.get('whole_c_inputs',{}) if 'upstream/'+p not in compiled_game})
    result = {'schema': 1, 'scope': 'compiled translation units, native symbol presence, and explicit guest imports',
              'limitations': ['No execution instrumentation; test linkage is not function execution.',
                              'Static function origin is not inferred from final binary symbol names.',
                              'Source provider/direct-call hints are lexical and omit indirect calls and SDK subprojects.',
                              'An admitted provider may be conditional or intentionally reject unsupported inputs.',
                              'Function layers identify translation-unit ownership; included original C and source slices are listed separately in original_transition_provenance, original_object_provenance and original_story_provenance.',
                              'No minimum SDK set or complete-game boot closure is established.'],
              'project_revision': command(['git', '-C', str(project), 'rev-parse', 'HEAD']).strip(),
              'upstream_revision': command(['git', '-C', str(upstream), 'rev-parse', 'HEAD']).strip(),
              'products': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in products},
              'objects': objects, 'translation_units': files, 'custom_compiled_c_objc_physical_lines': custom_lines,
              'compiled_game_units': compiled_game, 'functions': rows, 'guest_imports': imports,
              'source_entrypoint_cuts': roots,'original_transition_provenance':transition,'original_object_provenance':motion,
              'original_story_provenance':story,
              'included_original_c_inputs':included}
    (out / 'inventory.json').write_text(json.dumps(result, indent=2) + '\n')
    lines = ['# Reuse audit: generated build evidence', '',
             f'{len(rows)} emitted text symbols in {len(objects)} translation units; {len(compiled_game)} original game C units.',
             f'{len(included)} additional original C files included inside boundary units; {len(transition.get("functions",[]))+len(story.get("functions",[]))} original C function slices. These are not standalone whole-unit counts or execution coverage.',
             f'{custom_lines} physical lines of compiled project C/Objective-C (headers, Python, tests and upstream excluded).',
             '', '## Boundary inventory', '',
             f'{len(imports)} field guest imports; {sum(r["symbol"] in admitted for r in imports)} have conditional named provider branches.',
             'Provider presence is not execution evidence or whole-function support.', '',
             'See functions.csv for every emitted function and inventory.json for object/product hashes, guest imports and source-entrypoint cuts.',
             '', '## Interpretation', '']
    lines += ['- ' + note for note in result['limitations']]
    lines += ['', '## Translation-unit scope', '', '| Layer | Units |', '| --- | ---: |']
    lines += [f'| {layer} | {count} |' for layer, count in sorted(totals.items())]
    (out / 'REPORT.md').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines[:9]))
    print(f'Full evidence: {out}')


if __name__ == '__main__':
    main()
