"""Compile original ARM32 2D owners with their pinned SDK declarations.

SDK sources and generated output remain build-local.  This module adds no native
Sprite/renderer representation and leaves every unprovided SDK operation an
ordinary rejecting import in the existing guest assembler.
"""
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

from generated_files import write_text_if_changed
from prepare_original_field import functions


def sdk_paths(root, sdk_root=None, nns_root=None):
    """Resolve existing pinned inputs; never download or mutate an SDK tree."""
    evidence = root.parent.parent / 'build/evidence/weather'
    sdk = Path(sdk_root or os.environ.get('HG_NITROSDK', evidence / 'NitroSDK'))
    nns = Path(nns_root or os.environ.get('HG_NITROSYSTEM', evidence / 'NitroSystem'))
    for path, wrap, required in (
        (sdk, 'NitroSDK', 'include/nitro.h'),
        (nns, 'NitroSystem', 'include/nnsys/g2d.h'),
    ):
        revision = re.search(r'^revision\s*=\s*(\S+)',
                             (root / 'subprojects' / (wrap + '.wrap')).read_text(), re.M)[1]
        if not (path / required).is_file():
            raise FileNotFoundError(
                f'{wrap} ARM32 headers required at {path}; use the existing '
                f'upstream subprojects/{wrap}.wrap revision {revision} '
                f'(or set HG_{"NITROSDK" if wrap == "NitroSDK" else "NITROSYSTEM"}).')
        commit = subprocess.run(['git', '-C', str(path), 'rev-parse', 'HEAD'],
                                capture_output=True, text=True, check=True).stdout.strip()
        if commit != revision:
            raise ValueError(f'{wrap} input revision {commit} differs from upstream pin {revision}: {path}')
    return sdk.resolve(), nns.resolve()


def prepare_headers(out, sdk_root):
    """Supply freestanding libc declarations and the SDK's generated constants.

    No runtime implementation is supplied here. SDK_CW selects the original
    compiler's packing/inline branch; CLZ uses its declared out-of-line API
    instead of CodeWarrior's inline assembly syntax.
    """
    include = out / 'original_2d_headers'
    (include / 'nitro/fx').mkdir(parents=True, exist_ok=True)
    headers = {
        'stdio.h': '#include <stddef.h>\n',
        'stdlib.h': '#include <stddef.h>\nvoid *malloc(size_t); void free(void *); void abort(void);\n',
        'string.h': ('#include <stddef.h>\nvoid *memset(void *,int,size_t); '
                     'void *memcpy(void *,const void *,size_t); '
                     'void *memmove(void *,const void *,size_t); '
                     'size_t strlen(const char *);\n'),
    }
    for name, text in headers.items():
        write_text_if_changed(include / name, text)
    generator = sdk_root / 'gen/nitro/fx/gen_fx_const.py'
    constants = sdk_root / 'gen/nitro/fx/fx_const.csv'
    temporary = include / 'nitro/fx/fx_const.generated'
    subprocess.run([sys.executable, str(generator), str(constants), str(temporary)], check=True)
    write_text_if_changed(include / 'nitro/fx/fx_const.h', temporary.read_text())
    temporary.unlink()
    return include, [generator, constants]


def compiler_flags(include, sdk, nns):
    return ['-target', 'arm-none-eabi', '-mcpu=arm946e-s', '-mthumb', '-O1',
            '-ffreestanding', '-fwrapv', '-fno-unwind-tables',
            '-fno-asynchronous-unwind-tables', '-DSDK_ARM9', '-DSDK_CW',
            '-DSDK_FINALROM',
            '-DPLATFORM_INTRINSIC_FUNCTION_BIT_CLZ32=MATH_CountLeadingZerosFunc',
            '-Wno-c23-extensions', '-Wno-unknown-pragmas',
            '-I' + str(include), '-I' + str(sdk / 'include'),
            '-I' + str(nns / 'include')]


def generate(root, out, cc, sdk_root=None, nns_root=None):
    """Return (Thumb assembly, provenance dictionary, dependency Path list)."""
    root, out, cc = Path(root).resolve(), Path(out).resolve(), Path(cc)
    sdk, nns = sdk_paths(root, sdk_root, nns_root)
    include, dependencies = prepare_headers(out, sdk)
    raw_path = root / 'src/unk_02025534.c'
    raw_source = raw_path.read_text()
    start = raw_source.index('GF_2DGfxRawResMan *GF2dGfxRawResMan_Create(')
    end = raw_source.index('// 3D graphics', start)
    names = tuple(m[1] for m in re.finditer(
        r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',
        raw_source[start:end], re.M | re.S))
    raw_code, raw_records = functions(root, {'src/unk_02025534.c': names})
    owner_paths = (root / 'src/unk_0200A090.c', root / 'src/sprite.c')
    checks = '''
_Static_assert(sizeof(void *) == 4, "ARM32 pointers");
_Static_assert(sizeof(Sprite) == 0x104 && sizeof(SpriteList) == 0x124, "ARM sprite owners");
_Static_assert(sizeof(NNSG2dRendererInstance) == 0x98 && sizeof(NNSG2dRenderSurface) == 0x48, "SDK renderer ABI");
_Static_assert(__builtin_offsetof(SpriteList, renderer) == 0x114 && __builtin_offsetof(Sprite, imageProxy) == 0xb4, "embedded ARM SDK views");
_Static_assert(sizeof(GF_2DGfxResMan) == 0x14 && sizeof(SpriteResource) == 0xc && sizeof(GF_2DGfxResHeader) == 0x10, "ARM resource owners");
'''
    source = '#include "sprite.h"\n#include "global.h"\n#include "system.h"\n' + checks + raw_code
    records = list(raw_records)
    for path in owner_paths:
        source += '\n#line 1 "' + str(path) + '"\n' + path.read_text() + '\n'
        names = tuple(m[1] for m in re.finditer(
            r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}', path.read_text(), re.M | re.S))
        _, provenance = functions(root, {str(path.relative_to(root)): names})
        records.extend(provenance)
    c_path, s_path, d_path = (out / ('original_2d.' + ext) for ext in ('c', 's', 'd'))
    write_text_if_changed(c_path, source)
    subprocess.run([str(cc), *compiler_flags(include, sdk, nns),
                    '-I' + str(root / 'include'), '-I' + str(out),
                    '-MMD', '-MF', str(d_path), '-S', str(c_path), '-o', str(s_path)], check=True)
    # Headers are authoritative dependencies; no handwritten layout substitute.
    dependencies += [raw_path, *owner_paths,
                     root / 'subprojects/NitroSDK.wrap',
                     root / 'subprojects/NitroSystem.wrap']
    dependency_text = d_path.read_text().replace('\\\n', ' ')
    dependencies += [Path(p) for p in shlex.split(dependency_text.split(':', 1)[1])
                     if Path(p) not in (c_path,)]
    assembly = re.sub(r'\.L(\w+)', r'.Lgf2d_\1', s_path.read_text())
    provenance = dict(
        functions=records,
        sources={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in (raw_path, *owner_paths)},
        compiler_target='arm-none-eabi arm946e-s Thumb',
        adaptation='unchanged original GF 2D/resource/sprite bodies and real pinned ARM32 SDK headers; compiler/libc declarations only',
        sdk_inputs=dict(NitroSDK=str(sdk), NitroSystem=str(nns)),
        admission='compiled exports are not evidence of runtime execution; unprovided SDK operations remain imports')
    return '\n.set hg_compiler_syntax, 1\n' + assembly + '\n.set hg_compiler_syntax, 0\n', provenance, sorted(set(dependencies))


if __name__ == '__main__':
    import json
    asm, provenance, dependencies = generate(*sys.argv[1:])
    text_out = Path(sys.argv[2])
    write_text_if_changed(text_out / 'original_2d_assembly.s', asm)
    write_text_if_changed(text_out / 'original_2d_provenance.json', json.dumps(provenance, indent=2) + '\n')
    print(f'Compiled {len(provenance["functions"])} original 2D functions; {len(dependencies)} dependencies.')
