#!/usr/bin/env python3
"""Compare game-C compatibility passes; compilation is not runtime coverage."""
import argparse
import collections
import json
import pathlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before', type=pathlib.Path)
    parser.add_argument('after', type=pathlib.Path)
    args = parser.parse_args()
    before, after = (json.loads(p.read_text()) for p in (args.before, args.after))
    if before['revision'] != after['revision']:
        parser.error('Upstream revisions differ; compare the same source revision')
    def rows(data):
        return {r['source']:r for r in data['results'] if r['mode']=='compat' and r['group']=='game'}
    old,new = rows(before),rows(after)
    if old.keys() != new.keys():
        parser.error('Source sets differ')
    print('# Game C compatibility audit comparison\n')
    print('| Outcome | Before | After |\n| --- | ---: | ---: |')
    oc,nc = (collections.Counter(r['category'] for r in rows.values()) for rows in (old,new))
    for category in sorted(oc.keys()|nc.keys()):
        print(f'| {category} | {oc[category]} | {nc[category]} |')
    print('\n## Changed missing-header counts\n')
    print('| Header | Before | After |\n| --- | ---: | ---: |')
    oh,nh = (collections.Counter(r['missing_header'] for r in rows.values() if r['missing_header']) for rows in (old,new))
    for header in sorted(oh.keys()|nh.keys(),key=lambda h:(-abs(nh[h]-oh[h]),h)):
        if oh[header] != nh[header]:
            print(f'| `{header}` | {oh[header]} | {nh[header]} |')
    for title,predicate in [('Newly compiled',lambda b,a:not b['compiled'] and a['compiled']),('Regressions',lambda b,a:b['compiled'] and not a['compiled'])]:
        print(f'\n## {title}\n')
        paths=[p for p in sorted(new) if predicate(old[p],new[p])]
        print('\n'.join('- `'+p+'`' for p in paths) if paths else 'None.')
    print('\nCounts represent visible compiler blockers, not all downstream errors or native runtime correctness.')

if __name__=='__main__':
    main()
