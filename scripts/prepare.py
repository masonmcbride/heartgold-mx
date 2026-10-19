#!/usr/bin/env python3
"""Generate private build inputs without modifying the local upstream checkout."""
import csv, hashlib, json, pathlib, re, sys
root, out = map(pathlib.Path, sys.argv[1:])
# Match upstream msgenc Json::WriteHeader's ordinal IDs for every message
# bank. Both include spellings occur upstream. No message text is emitted.
message_inputs = {}
for source in sorted((root/'files/msgdata/msg').glob('*.json')):
    messages = json.loads(source.read_text())['messages']
    ids = [message['id'] for message in messages]
    if len(ids) != len(set(ids)) or any(not re.fullmatch(r'[A-Za-z_]\w*', name) for name in ids):
        raise ValueError(f'Invalid or duplicate message identifiers: {source}')
    definitions = ''.join(f'#define {name} {i}\n' for i, name in enumerate(ids))
    guard = 'HG_GENERATED_' + source.stem.upper() + '_H'
    header = f'#ifndef {guard}\n#define {guard}\n' + definitions + '#endif\n'
    for prefix in ('msgdata/msg', 'files/msgdata/msg'):
        dest = out/prefix/(source.stem+'.h')
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_text(header)
    message_inputs[source.relative_to(root).as_posix()] = hashlib.sha256(source.read_bytes()).hexdigest()
(out/'message_indices_provenance.json').write_text(json.dumps({'sources':message_inputs, 'banks':len(message_inputs)}, indent=2)+'\n')
(out/'message_bank_count.inc').write_text('#define HG_MESSAGE_BANK_COUNT '+str(1+max(int(pathlib.Path(p).stem.split('_')[1]) for p in message_inputs))+'\n')
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
