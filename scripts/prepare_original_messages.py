"""Run upstream's message encoder; keep DS banks separate from preview UTF-8.

Inputs are the existing upstream tool, charmap, manifest and yyjson wrap. No
encoding algorithm or game text is duplicated here. Outputs stay build-local.
"""
import concurrent.futures
import hashlib
import json
from pathlib import Path
import subprocess

from generated_files import write_text_if_changed
from narc_tools import pack


def prepare(root, out, cc, yyjson):
    root, out, yyjson = map(lambda p: Path(p).resolve(), (root, out, yyjson))
    source = root / 'tools/msgenc'
    work = out / 'original_messages'
    work.mkdir(exist_ok=True)
    dependencies = list(source.glob('*.cpp')) + list(source.glob('*.h'))
    dependencies += list(source.glob('*.hpp'))
    dataproc = root / 'tools/dataproc/lib'
    c_sources = [dataproc / 'dataproc.c', dataproc / 'json.c', yyjson / 'src/yyjson.c']
    dependencies += c_sources + [dataproc / 'include/dataproc.h', dataproc / 'private.h',
                                dataproc / 'json.h', yyjson / 'src/yyjson.h']
    if not all(p.is_file() for p in dependencies):
        raise FileNotFoundError('Original msgenc requires upstream-pinned yyjson with upstream packagefile patches; set HG_YYJSON')
    # This is the same release tool and parser configuration as upstream Meson.
    identity = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
    stamp = work / 'tool.json'
    tool = work / 'msgenc'
    if not tool.exists() or not stamp.exists() or json.loads(stamp.read_text()) != identity:
        objects = []
        for i, path in enumerate(c_sources):
            obj = work / f'parser{i}.o'
            subprocess.run([str(cc), '-std=gnu17', '-O2', '-DNDEBUG', '-I' + str(dataproc / 'include'),
                            '-I' + str(yyjson / 'src'), '-c', str(path), '-o', str(obj)], check=True)
            objects.append(obj)
        subprocess.run([str(cc), '-x', 'c++', '-std=c++17', '-O2', '-DNDEBUG',
                        '-I' + str(dataproc / 'include'), *map(str, source.glob('*.cpp')),
                        '-x', 'none', *map(str, objects), '-lc++', '-o', str(tool)], check=True)
        write_text_if_changed(stamp, json.dumps(identity, indent=2) + '\n')
    manifest = root / 'files/msgdata/NARC_msg.txt'
    charmap = source / 'charmap.txt'
    trainer = root / 'files/poketool/trainer'
    dependencies += [manifest, charmap, trainer / 'trainers.json', trainer / 'trname.json.txt']
    trainer_json = work / 'msg_0729.json'
    # Reuse the original template processor already built by the resource tools.
    subprocess.run([str(out / 'jsonproc'), str(trainer / 'trainers.json'),
                    str(trainer / 'trname.json.txt'), str(trainer_json)], check=True)
    banks = []
    for name in manifest.read_text().splitlines():
        stem = name.removeprefix('NARC_msg_').removesuffix('_bin')
        path = trainer_json if stem == 'msg_0729' else root / 'files/msgdata/msg' / (stem + '.json')
        if not path.exists():
            raise FileNotFoundError(path)  # Never fill an absent bank with placeholders.
        banks.append(path)
        if path != trainer_json:
            dependencies.append(path)
    tool_key = hashlib.sha256(tool.read_bytes() + charmap.read_bytes()).hexdigest()

    def encode(path):
        target = work / (path.stem + '.bin')
        key = hashlib.sha256(path.read_bytes() + tool_key.encode()).hexdigest()
        saved = target.with_suffix('.sha256')
        if not target.exists() or not saved.exists() or saved.read_text() != key:
            subprocess.run([str(tool), '-e', '--json', '-c', str(charmap),
                            str(path), str(target)], check=True)
            write_text_if_changed(saved, key)
        return target.read_bytes()

    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        encoded = list(pool.map(encode, banks))
    pack(out / 'resources/original_messages.narc', encoded)
    provenance = dict(tool_inputs=identity, original_manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest(),
                      charmap_sha256=hashlib.sha256(charmap.read_bytes()).hexdigest(), banks=len(banks),
                      members=[dict(index=i, source=str(p), source_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),
                                    encoded_sha256=hashlib.sha256(b).hexdigest()) for i, (p, b) in enumerate(zip(banks, encoded))],
                      boundary='original upstream encoder and original bank order; regenerated format, not ROM-byte equality')
    write_text_if_changed(out / 'original_messages_provenance.json', json.dumps(provenance, indent=2) + '\n')
    write_text_if_changed(out / 'original_message_inputs.cmake',
                         'set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS\n' +
                         ''.join('  "' + str(p) + '"\n' for p in dependencies) + ')\n')


if __name__ == '__main__':
    import sys
    prepare(*sys.argv[1:])
