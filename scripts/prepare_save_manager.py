#!/usr/bin/env python3
"""Emit the whole upstream save manager with audited native-only UB fixes.

The original translation unit's includes are supplied by save_manager.c at the
explicit compatibility boundary. Reject changed patterns instead of silently
applying a potentially stale patch. Upstream is never edited.
"""
from generated_files import write_text_if_changed
import argparse
import pathlib
import re


def prepare(source: str) -> str:
    source = re.sub(r'^#include [^\n]+\n', '', source, flags=re.M)
    fixes = {
        'hdr[i].block != hdr[i + 1].block && i + 1 < gNumSaveChunkHeaders':
        'i + 1 < gNumSaveChunkHeaders && hdr[i].block != hdr[i + 1].block',
        'i == headers[j].slot && j < gNumSaveChunkHeaders':
        'j < gNumSaveChunkHeaders && i == headers[j].slot',
    }
    for old, new in fixes.items():
        if source.count(old) != 1:
            raise ValueError(f'Expected exactly one original boundary predicate: {old}')
        source = source.replace(old, new)
    return '/* Generated from upstream save.c; two bounds predicates reordered. */\n' + source


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--upstream', type=pathlib.Path, default=pathlib.Path('upstream/pokeheartgold'))
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    result = prepare((args.upstream / 'src/save.c').read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    write_text_if_changed(args.output, result)


if __name__ == '__main__':
    main()
