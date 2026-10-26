#!/usr/bin/env python3
"""Measured validation layers. Logs/results are local; successful results never cache."""
import argparse
import json
import os
from pathlib import Path
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
FOCUSED = ['test_compat_micro', 'test_original_field_constructor', 'test_prop_animation']
SANITIZED = FOCUSED + ['test_platform', 'test_native_lifetimes', 'test_save_manager', 'test_save_recovery']
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('layer', choices=['begin', 'micro', 'focused', 'full', 'profile'])
p.add_argument('--build', default='build')
p.add_argument('--sanitize-build', default='build-sanitize')
p.add_argument('--jobs', type=int, default=6)
p.add_argument('--iteration', help='Name started with begin before investigation/editing')
p.add_argument('--repeats', type=int, default=3)
a = p.parse_args()
folder = ROOT / a.build / 'audit' / 'feedback'
folder.mkdir(parents=True, exist_ok=True)
start = time.monotonic_ns()
run_id = str(time.time_ns())
rows = []

def append(path, row):
    with path.open('a') as f:
        f.write(json.dumps(row) + '\n')

def run(label, command, env=None):
    log = folder / (run_id + '-' + label + '.log')
    before = time.monotonic_ns()
    with log.open('w') as f:
        r = subprocess.run(list(map(str, command)), cwd=ROOT, stdout=f, stderr=subprocess.STDOUT, env=env)
    row = dict(label=label, seconds=(time.monotonic_ns()-before)/1e9, exit=r.returncode, log=str(log.relative_to(ROOT)))
    rows.append(row)
    append(folder / 'commands.jsonl', row)
    if r.returncode:
        print(log.read_text()[-6000:], file=sys.stderr)
        raise subprocess.CalledProcessError(r.returncode, command)
    return row

def build(directory, targets=()):
    manifest = ROOT / directory / 'CTestTestfile.cmake'
    if not manifest.exists() or 'compatibility_micro' not in manifest.read_text():
        run('configure-' + directory, ['cmake', '-S', '.', '-B', directory])
    return run('build-' + directory, ['cmake', '--build', directory, '-j', a.jobs] + (['--target'] + list(targets) if targets else []))

state_file = folder / 'iteration.json'
if a.layer == 'begin':
    if not a.iteration:
        p.error('begin requires --iteration NAME')
    if state_file.exists() and not json.loads(state_file.read_text()).get('complete'):
        p.error('an iteration is already open; finish it before beginning another')
    state_file.write_text(json.dumps(dict(name=a.iteration, started_ns=start, wall_started_ns=time.time_ns(), complete=False)))
    print('Iteration started before investigation/editing: ' + a.iteration)
    sys.exit(0)

iteration = None
if a.iteration:
    iteration = json.loads(state_file.read_text())
    if iteration['name'] != a.iteration or iteration['complete']:
        p.error('iteration must match an open begin record')
success = False
try:
    if a.layer in ('micro', 'focused'):
        build(a.build, ['test_compat_micro'] if a.layer == 'micro' else FOCUSED)
        # Run real tests in one invocation where possible. CTest preserves
        # failure reporting and existing independent integration processes.
        if a.layer == 'micro':
            run('micro', [ROOT / a.build / 'test_compat_micro'])
        else:
            run('focused', ['ctest', '--test-dir', a.build, '-j', a.jobs, '--output-on-failure', '-R', '^(compatibility_micro|original_field_construction|original_prop_animation)$'])
    elif a.layer == 'full':
        build(a.build)
        run('regression', ['ctest', '--test-dir', a.build, '-j', a.jobs, '--output-on-failure', '--output-junit', 'audit/feedback/' + run_id + '-regression.xml'])
        build(a.sanitize_build, SANITIZED)
        env = dict(os.environ, UBSAN_OPTIONS='halt_on_error=1')
        run('sanitizers', ['ctest', '--test-dir', a.sanitize_build, '-j', a.jobs, '--output-on-failure', '-R', '^(compatibility_micro|original_field_construction|original_prop_animation|ds_platform|native_heap_module_lifetimes|original_save_manager|original_save_recovery)$'], env)
        run('projection', [sys.executable, '-m', 'unittest', 'discover', '-s', 'tests', '-p', 'test_original_field_audit.py'])
        run('native-smoke', [ROOT / a.build / 'heartgold_field_viewer', '--smoke'])
        run('diff-check', ['git', 'diff', '--check'])
        if subprocess.check_output(['git', 'status', '--porcelain', '--', 'upstream/pokeheartgold'], cwd=ROOT):
            raise RuntimeError('upstream changed')
    else:
        env = dict(os.environ, HG_PROFILE_RESOURCES='1')
        samples = {}
        for name in ['test_sdk_types', 'test_compat_micro', 'test_original_field_constructor', 'test_prop_animation', 'test_save_recovery']:
            samples[name] = [run(name + '-' + str(i), [ROOT / a.build / name], env)['seconds'] for i in range(a.repeats)]
        run('noop-build', ['cmake', '--build', a.build, '--target', 'test_compat_micro', '-j', a.jobs])
        print(json.dumps(dict(process_cold_then_warm=samples, medians={k:statistics.median(v) for k,v in samples.items()}, cache_note='Fresh process each sample; OS cache is not purged. Constructor checks two fresh mutable sessions sharing immutable archive bytes.')))
    success = True
finally:
    report = dict(layer=a.layer, success=success, wall_seconds=(time.monotonic_ns()-start)/1e9, commands=rows)
    if iteration:
        report['iteration'] = a.iteration
        report['iteration_wall_seconds'] = (time.time_ns()-iteration['wall_started_ns'])/1e9
        if success:
            iteration['complete'] = True
            state_file.write_text(json.dumps(iteration))
    append(folder / 'runs.jsonl', report)
    print(json.dumps({k:v for k,v in report.items() if k != "commands"} | {"command_count": len(rows), "build_seconds":sum(r["seconds"] for r in rows if r["label"].startswith("build-")), "results":str((folder / "runs.jsonl").relative_to(ROOT))}))
