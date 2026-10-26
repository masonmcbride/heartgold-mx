# Development validation

Use affected targets and the smallest relevant check during ordinary work. Full
validation is manual, for completed milestones or explicit requests.

```sh
python3 scripts/validate.py micro
python3 scripts/validate.py focused
python3 scripts/validate.py full
```

Micro runs the existing memory, heap/module/key and original scheduler tests in
one process, twice with fresh mutable owners, plus CRC reference equivalence.
Focused builds/runs the micro, original constructor/LOAD and prop animation tests.
Full retains all 80 original CTests plus the microtest, seven sanitizer checks,
projection/assembly checks, native smoke and diff/upstream checks. Inspect the
recovered scene after native smoke. Results/logs stay under `build/audit/feedback`.
No successful test result or mutable session is cached. CTest's existing serial
properties and dependencies remain in force. Do not launch competing builds or
expensive validation workloads alongside a timing run.

For the current private loader adapter, the smaller normal iteration is:

```sh
cmake --build build --target test_original_field_constructor -j 6
./build/test_original_field_constructor
```

Record investigation/editing as well as validation time:

```sh
python3 scripts/validate.py begin --iteration NAME
# Investigate and edit.
python3 scripts/validate.py focused --iteration NAME
```

`wall_seconds` measures the validation command; `iteration_wall_seconds` measures
elapsed wall time since begin, including investigation, edits, failures and idle
time. `profile` explicitly benchmarks fresh-process and subsequent OS-cache-warm
runs and enables aggregate fixture/archive/interpreter timing. It does not purge
OS caches; these are not physical disk-cold measurements. Profiling is opt-in.

Recorded samples during this work: startup 1.8–3.8ms; micro executable median
20.9ms; constructor median 35.3ms, down from 42.0ms. Save recovery fell from
12.08s to 5.02s with equivalent CRC acceleration and unchanged fault cases.
The constructor reads 25.9MB from 20 archives; reads/parsing take about 4ms and
0.04ms respectively. A no-op focused build took 0.30s; first reconfiguration took
11.05s. No additional fixture cache was justified.

Measured focused build/test commands took 0.72–1.36s, while successful actual
investigation/edit/validation iterations took 53.26s, 108.75s and 79.97s. The latter
imply roughly 20, 9 and 13 iterations per 18 minutes respectively; they are varied
work samples, not a forecast. A six-second validation budget permits 180 checks
in 18 minutes before editing time, not 180 demonstrated development iterations.

The latest dispatcher-specific measurements and original-engine checkpoint are
in section 28 of [the ownership plan](original-engine-ownership-plan.md).
