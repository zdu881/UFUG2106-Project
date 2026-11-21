# UFUG2106 Stage 1 — Group 2

Zeyun DU (50025854) and Jincan Li (50032637). This is an AI-assisted project; see
[team review and provenance](docs/TEAM_REVIEW.md).

## Compile and run

Requires Linux, Make and a C++17 compiler (tested with GCC 11.4).
The synthesizer needs no Python, network or third-party library.

```bash
make
./build/synth input.txt output.txt
```

The submission executable is `build/synth`, compiled from `src/synth.cpp`.
It runs on one CPU thread and takes exactly two arguments: input and output
paths. Output contains only AND2/OR2/NOT gates and the final output-ID line.
Diagnostics go to stderr. `make baseline` optionally builds the simpler
engineering comparator; it is not the submitted optimized method.

A direct build without Make is also supported:

```bash
g++ -O2 -std=c++17 -Wall -Wextra -Wpedantic src/synth.cpp -o synth
./synth input.txt output.txt
```

## Method and limitations

The program constructs shared complemented BDDs from the complete truth table,
tries variable orders and cost-aware gate mappings, balances selected cones and
keeps the best completed circuit by delay excess, area, then delay. See
[method and correctness](docs/METHOD.md).

The first candidate is constructed without the deadline used for later search.
Stage 1 parses the RAM limit but does not actively schedule memory against it.
Neither arbitrary tight resource limits nor globally optimal circuits are
promised. Wall-clock search truncation can change results between reruns.

## Verification

Python 3.10+ and NumPy are needed only for optional independent checking.
Obtain the instructor's Stage 1 cases and Linux evaluator from Canvas. Place
25 input `.txt` files under `official/stage1/` and the executable at
`official/evaluator`; these instructor materials are not in this source ZIP.

```bash
chmod +x official/evaluator
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/evaluate_release.py --output results/my-fresh-run
python3 tests/regression.py ./build/synth results/my-fresh-regression.json
./build/synth examples/shared_outputs.txt example.output.txt
python3 tools/check_circuit.py examples/shared_outputs.txt example.output.txt
```

Use new result paths. The evaluator runner snapshots inputs/source/tools, builds,
runs the official evaluator, and independently checks every truth-table output.
Inspect `official.csv`, `independent.json`, `manifest.json` and logs.
Correctness PASS and meeting the circuit-delay target are separate outcomes.

## Retained report evidence

The companion PDF's performance tables use the retained 10 November optimized
run, not an estimate from a new machine: 25/25 PASS and targets, area 622,816,
median runtime 603.945 ms. The baseline is 25/25 PASS, 12/25 targets, area
1,103,455 and median runtime 4.628 ms. These are separate retained measurements.

- `evidence/stage1-final-20261110/`: official CSV, independent checks and run metadata.
- `evidence/baseline.csv`: baseline evaluator results.
- `evidence/cap250.csv`, `evidence/cap500.csv`: historical search-budget comparisons.
- `evidence/final/`: historical generated, sanitizer and structured stress checks.

The report's historical sanitizer/stress evidence is not claimed as newly run
when packaging. An extracted-package rerun is separate evidence and may yield
slightly different area/runtime. The Stage 2 prototype is not part of this
Stage 1 source archive. No official score or hidden-case result is predicted.

## Additional selection evidence, 16 November

The seven-page report now includes three component ablations and three shorter
search ceilings, each measured twice against the frozen method. All 350 official
case-runs and independent checks pass; shorter caps miss case 024's delay target.
See `docs/STAGE1_STUDY_20261116.md` and `evidence/stage1-study/` for exact scope,
results and reproduction. The production source and retained 10 November
per-case report tables are unchanged. New measurements are explicitly separate.
