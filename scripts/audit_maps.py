#!/usr/bin/env python3
"""Render every original map in isolated processes; keep failures reviewable."""
import argparse, collections, concurrent.futures, json, pathlib, subprocess
p=argparse.ArgumentParser();p.add_argument('--build',type=pathlib.Path,default=pathlib.Path('build'));a=p.parse_args()
build=a.build.resolve();binary=build/'heartgold_field_catalog'
def probe(map_id):
    try:
        result=subprocess.run([str(binary),str(map_id)],capture_output=True,text=True,timeout=15)
        if result.returncode: return {'id':map_id,'rendered':False,'error':f'Process exited {result.returncode}','diagnostic':result.stderr[-1000:]}
        row=json.loads(result.stdout);assert row['id']==map_id
        if result.stderr:row['diagnostic']=result.stderr[-1000:]
        return row
    except (subprocess.TimeoutExpired,ValueError,AssertionError) as e:
        return {'id':map_id,'rendered':False,'error':type(e).__name__}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:rows=list(pool.map(probe,range(540)))
output=build/'evidence/map-catalog';output.mkdir(parents=True,exist_ok=True)
(output/'maps.json').write_text(json.dumps(rows,indent=2)+'\n')
counts=collections.Counter(row['error'] or 'Rendered' for row in rows)
report=['# Map rendering audit','',f'{sum(r["rendered"] for r in rows)} / {len(rows)} maps rendered at 96×96.',
        '', 'This exercises scene loading and rasterization, not visual accuracy against hardware.','', '| Result | Count |','| --- | ---: |']
report += [f'| {reason} | {count} |' for reason,count in counts.most_common()]
(output/'REPORT.md').write_text('\n'.join(report)+'\n')
print('\n'.join(report))
