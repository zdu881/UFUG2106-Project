# UFUG2106 — Truth Table to Circuit

## Stage 2 public comparison

Build the integrated solver with `make stage2`, then run `./build/synth_stage2 INPUT OUTPUT`.
The Stage 2 solver searches variable orders and chooses output circuits by their
mapped area and delay, counting shared gates once. If memory runs short before
the first circuit is complete, it writes gates in dependency order using bounded
lookup tables. The retested source ZIP passed all 50 released cases and all delay
targets. Area and delay matched the preceding candidate on every case. The total
area was 872,632 and total synthesis time was 32.86 s on this workstation.

On three generated dense tables, with 32, 48 and 64 MiB limits, the old solver
failed official evaluation and the new one passed. All three new circuits also
passed independent full truth-table replay. The separate structured 20-input
case still receives MEMORY_LIMIT from the course evaluator at 64 MiB. Its circuit
passed all 1,048,576 assignments under a hard address-space cap. The same
evaluator flags a zero-gate eight-wire example. A lightweight grader-side worker
measures the solver below the limit and detects a real over-limit process.
[Reproducer and measurements](reports/stage2-rss-accounting-20261121/README.md)

Run `make stage2-source` to create `artifacts/UFUG2106_Group2_Stage2_Source.zip`.
Rebuild the report with Python and ReportLab:

```sh
python3 tools/report_public_comparison.py reports/stage2-memory-20261121 /tmp/ufug2106-report.pdf --merged
```

[Report](reports/stage2-memory-20261121/UFUG2106_Group2_Stage2_Report.pdf) ·
[Stage 1 comparison](reports/stage2-memory-20261121/comparison.json) ·
[Memory controls](reports/stage2-memory-20261121/memory-controls.json) ·
[Dense input generator](reports/stage2-memory-20261121/generate_dense.py) ·
[Prior variable-order and output-selection experiments](reports/stage2-schedule-20261121/UFUG2106_Group2_Stage2_Report.pdf) ·
[Round 5.1 attribution](src/round51/ORIGIN.md)

Workspace: `~/2106Proj`. Coding-agent entry: [AGENTS.md](AGENTS.md);
[OpenCode handoff](docs/WORKSPACE_HANDOFF.md).

A single-threaded C++17 truth-table synthesizer, baseline, independent verifier, and reproducible experiments for the published Stage 1 cases.

Group 2 — Zeyun DU (50025854) and Jincan Li (50032637). See the [teamwork log](docs/TEAMWORK_LOG.md) and [AI assistance record](docs/AI_WORK_LOG.md).

Stage 1 was resubmitted on 17 November (attempt 2). The [submission record](docs/STAGE1_DELIVERY_20261115.md) links the uploaded source and seven-page report.

## Build and execute

Requirements: Linux, a C++17 compiler (GCC 11 or later is suitable), and Make. No network access or external synthesis package is used by the submitted program.

```bash
make
./build/synth input.txt output.txt
./build/baseline input.txt baseline-output.txt
```

The program reads the entire official input format, including optional limits. It emits only AND2, OR2 and NOT gates, consecutive IDs, and the final output-ID line. Runtime diagnostics go to stderr. The executable takes exactly two paths. Do not include the stderr diagnostics in the circuit file.

`baseline` uses a fixed natural variable order and ordinary Shannon mapping. `synth` adds complemented edges, algebraic mapping, variable-order search, balanced gates and two-level decision expansion. Both prune dead gates before writing. See `docs/METHOD.md` for the correctness argument.

## Official evaluation

The instructor-provided Linux evaluator and both banks of 25 cases are available under `official/`. They are not bundled in the source-only archive; use your course copies when reproducing it.

```bash
mkdir -p results
run_dir=$(mktemp -d results/reproduction-XXXXXX)
./official/evaluator ./build/synth ./official/stage1 \
  --report "$run_dir/optimized.csv" \
  --save-outputs "$run_dir/outputs"
```

The output directory must be new or empty. The evaluator writes **CSV**, even if given a filename ending in `.json`. Read `target_met` separately from `status`: a correct but late circuit can be PASS while missing the delay target.

## Independent correctness and regression

The synthesizer itself needs only C++. Additional Python checks require Python 3.10+ and NumPy. Report generation additionally uses Markdown, WeasyPrint, pypdf and Liberation Sans fonts.

```bash
./build/synth examples/shared_outputs.txt "$run_dir/example.txt"
python3 tools/check_circuit.py examples/shared_outputs.txt "$run_dir/example.txt"
python3 tests/regression.py ./build/synth "$run_dir/regression.json"
python3 tools/verify_all.py official/stage1 "$run_dir/outputs" \
  "$run_dir/optimized.csv" "$run_dir/independent.json"
```

The independent checker evaluates **every input assignment and every output**, validates IDs/arity/topological ordering, and independently recomputes area and delay. It uses Python integer bit lanes, not the BDD implementation or the official evaluator's internals.

## Results and tradeoffs

`results/final/` retains the original technical evaluation; `results/stage1-final-20261110/` is the snapshot used for the report’s performance table (25/25 PASS, 25/25 targets, area sum 622,816, median runtime 603.945 ms). `results/reproduction-20261110-verified/` is an earlier verified rerun. Earlier `optimized_v*.csv` and source snapshots document iterations, not different claims about the final version. CPU time and deadline-truncated search can vary across machines, so identical final circuit bytes are not guaranteed. Always report the actual measured outputs of a rerun.

The selection criterion is delay-excess first, then area, then delay; runtime is constrained through the search budget. The teacher has not published a numerical score formula. This heuristic is not a claim of optimality or of an official grade. The report must show the additional synthesis runtime alongside area improvements.

## Remaining project gates

- The [Stage 2 report](reports/stage2-memory-20261121/UFUG2106_Group2_Stage2_Report.pdf) compares the submitted Stage 1 method with the Stage 2 candidate on all 50 released inputs.
- Stage 2 is due on 4 December at 23:59 Beijing time. The date of the joint live Q&A is pending.
- The reported scores cover the public cases. Hidden-case grading is pending.

## Review experiments

```bash
# Faster-search ablations (only the maximum search budget changes).
g++ -O2 -std=c++17 results/synth_cap250.cpp -o build/synth_cap250
g++ -O2 -std=c++17 results/synth_cap500.cpp -o build/synth_cap500
# Run either executable through the official evaluator with a fresh output directory.

# Sanitizers; performance numbers must come from the ordinary -O2 build.
g++ -O1 -g -std=c++17 -fsanitize=address,undefined \
  -fno-omit-frame-pointer src/synth.cpp -o build/synth_sanitized
ASAN_OPTIONS=detect_leaks=1 python3 tests/regression.py \
  ./build/synth_sanitized results/sanitizers_rerun.json

# Largest-dimension case, with an explicit 128 MiB RAM cap in its input.
python3 tools/generate_stress.py results/stress_rerun/case_101.txt
./official/evaluator ./build/synth results/stress_rerun \
  --report results/stress_rerun.csv \
  --save-outputs results/stress_rerun_outputs

# Rebuild the report from the retained evidence.
python3 tools/build_report.py --run results/stage1-final-20261110 \
  --output reports/UFUG2106_Stage1_Technical_Report.pdf
```

## Reproduce the Stage 2 comparison

Run from the repository root on Linux with a C++17 compiler and Python 3 with
NumPy. The frozen comparator is the source submitted for Stage 1.

```bash
run_dir=$(mktemp -d)
for bank in stage1 stage2; do
  cases="official/$bank"
  python3 tools/evaluate_release.py --source releases/stage1_candidate/synth.cpp \
    --cases "$cases" --output "$run_dir/$bank-frozen" || exit 1
  python3 tools/evaluate_release.py --source src/synth_stage2.cpp \
    --cases "$cases" --output "$run_dir/$bank-current" || exit 1
done
python3 tools/compare_stage_results.py \
  --pair stage1 "$run_dir/stage1-frozen/official.csv" "$run_dir/stage1-current/official.csv" \
  --pair stage2 "$run_dir/stage2-frozen/official.csv" "$run_dir/stage2-current/official.csv" \
  --output "$run_dir/comparison.json"
```

Each run records the evaluator CSV and an independent full truth-table replay.
The comparison pairs identical case names within each bank. Wall-clock search
can produce different circuits across reruns.

## Earlier Stage 2 prototype

`src/synth_stage2.cpp` maps natural and influence orders before screening further
orders, then refines candidates using actual mapped area/delay. Build with
`make stage2`. The [50-case results](docs/STAGE2_SEARCH_20261120.md) retain
per-case regressions as well as gains. Optional search responds to current RSS;
the official 64 MiB stress test still fails. The [memory experiment](docs/STAGE2_SEARCH_20261120.md#64-mib-stress-result) reproduces the effect of the parent process on the measured peak.

```bash
python3 tools/evaluate_release.py --source src/synth_stage2.cpp \
  --output results/stage2-on-stage1
python3 tools/compare_stage_results.py \
  --pair stage1 results/stage1-final-20261110/official.csv \
  results/stage2-on-stage1/official.csv \
  --output results/stage2-preview.json
```

## Snapshot-based reproduction (10 November update)

Run with a Python interpreter that has NumPy installed (`python3` on the current
workstation). The Canvas agent's `.venv/bin/python` does not currently have NumPy.

```bash
python3 tools/evaluate_release.py --output results/my-fresh-run
```

The output directory must not exist. The runner snapshots the source, case files,
evaluator and verifier, checks Python dependencies, builds with C++17, runs the
teacher evaluator, requires an exact case set, and independently checks every row.
It records compiler/platform information, commands, exit codes and timings in
`manifest.json`. Any failed step exits nonzero and retains its evidence.
Missing a delay target is reported separately from an invalid circuit. Change
`--source` and `--cases` to evaluate a frozen Stage 1 method or future released cases.
`--timeout` bounds each pipeline command; it is not the official per-case time limit.

Start learning the implementation with `examples/README.md`, then see the latest
progress and actual rerun measurements in `docs/PROGRESS_20261110.md`.

## GitHub checkout

The full collaboration checkout contains project-authored source, documentation, tests,
reports, selected measurement summaries, and the instructor-provided project
materials in `official/`. Raw run snapshots remain local.

After cloning on Linux x86, run `make` and the snapshot reproduction command
above. Both public case banks and the executable evaluator are included. See `official/README.md` for original Canvas sources. Python verification needs
NumPy; the C++ executable itself has no Python dependency. Original raw run paths
in retained manifests refer to the development workstation; create a fresh run
on your own machine for new provenance and outputs.
