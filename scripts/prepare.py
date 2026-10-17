#!/usr/bin/env python3
"""Generate private build inputs without modifying the local upstream checkout."""
import csv, pathlib, re, sys
root, out = map(pathlib.Path, sys.argv[1:])
text = (root / 'src/item.c').read_text()
for name in ('item_data', 'item_icon'):
    dest = out / 'files/itemtool/itemdata' / (name + '.naix')
    dest.parent.mkdir(parents=True, exist_ok=True)
    symbols = sorted(set(re.findall(r'\bNARC_' + name + r'_\w+', text)))
    lines = []
    for symbol in symbols:
        match = re.search(r'_(\d+)(?:_|$)', symbol)
        if not match:
            raise ValueError(symbol)
        lines.append(f'#define {symbol} {int(match[1])}\n')
    dest.write_text(''.join(lines))
p = out / 'files/msgdata/msg.naix'
p.parent.mkdir(parents=True, exist_ok=True)
p.write_text(''.join(f'#define msg_{n} {int(n)}\n' for n in sorted(set(re.findall(r'\bmsg_(\d+)', text)))))
rows = list(csv.DictReader((root / 'files/itemtool/itemdata/item_data.csv').open()))
manifest = (root / 'files/itemtool/itemdata/item_data.txt').read_text().splitlines()
party = [line.split(':')[0] for line in manifest[manifest.index('slp_heal:u8.1:bool'):] if ':pad' not in line]
lines = ['/* Private generated upstream data; never commit. */\nstatic const ItemData native_items[] = {\n']
mapping = dict(re.findall(r'\[(ITEM_\w+)\] = \{ NARC_item_data_(\d+)_bin', text))
for index, row in enumerate(rows):
    if int(mapping[row['item']]) != index:
        raise ValueError('CSV archive order changed')
    def value(k):
        v = row[k]
        if not re.fullmatch(r'[A-Za-z_0-9+-]+', v):
            raise ValueError(v)
        return {'true':'1', 'false':'0'}.get(v, v)
    fields = [f'.{k}={value(k)}' for k in row if k != 'item' and k not in party]
    fields += ['.partyUseParam={' + ','.join(f'.{k}={value(k)}' for k in party) + '}']
    lines.append('[' + str(index) + ']={' + ','.join(fields) + '},\n')
lines.append('};\n')
(out / 'item_records.inc').write_text(''.join(lines))
