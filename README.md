# UFUG2106 — Truth Table to Circuit

Workspace: `~/2106Proj`. Coding-agent entry: [AGENTS.md](AGENTS.md);
[OpenCode handoff](docs/WORKSPACE_HANDOFF.md).

A single-threaded C++17 truth-table synthesizer, baseline, independent verifier, and reproducible experiments for the published Stage 1 cases.

**Provenance:** this workspace was developed with AI assistance. AI use has been confirmed for this project. The code and measurements are recorded transparently; students remain responsible for understanding and presenting the work. See `docs/AI_WORK_LOG.md` and `docs/REQUIREMENTS_AND_REVIEW.md`.

Team membership recorded on Canvas: GROUP 2 — Zeyun DU and Xinchen HUANG. This does not attribute any unconfirmed contribution to either student.

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

The instructor-provided Linux evaluator and 25 case files are included in this private repository under `official/`. They are not bundled in the source-only archive; use your course copies when reproducing it.

```bash
mkdir -p results/reproduction
./official/evaluator ./build/synth ./official/stage1 \
  --report results/reproduction/optimized.csv \
  --save-outputs results/reproduction/outputs
```

The output directory must be new or empty. The evaluator writes **CSV**, even if given a filename ending in `.json`. Read `target_met` separately from `status`: a correct but late circuit can be PASS while missing the delay target.

## Independent correctness and regression

The synthesizer itself needs only C++. Additional Python checks require Python 3.10+ and NumPy. Report generation additionally uses Markdown, WeasyPrint, pypdf and Liberation Sans fonts.

```bash
python3 tools/check_circuit.py input.txt output.txt
python3 tests/regression.py ./build/synth results/regression.json
python3 tools/verify_all.py official/stage1 results/reproduction/outputs \
  results/reproduction/optimized.csv results/reproduction/independent.json
```

The independent checker evaluates **every input assignment and every output**, validates IDs/arity/topological ordering, and independently recomputes area and delay. It uses Python integer bit lanes, not the BDD implementation or the official evaluator's internals.

## Results and tradeoffs

`results/final/` retains the original technical evaluation; `results/stage1-final-20261110/` is the latest fresh snapshot (25/25 PASS, 25/25 targets, area sum 622,816, median runtime 603.945 ms). `results/reproduction-20261110-verified/` is an earlier verified rerun. Earlier `optimized_v*.csv` and source snapshots document iterations, not different claims about the final version. CPU time and deadline-truncated search can vary across machines, so identical final circuit bytes are not guaranteed. Always report the actual measured outputs of a rerun.

The selection criterion is delay-excess first, then area, then delay; runtime is constrained through the search budget. The teacher has not published a numerical score formula. This heuristic is not a claim of optimality or of an official grade. The report must show the additional synthesis runtime alongside area improvements.

## Remaining project gates

- Students must understand, accurately describe, and own the work they actually do. Keep genuine teamwork records.
- Stage 2 adds another 25 public cases; compare the actual adopted Stage 1 method with the Stage 2 improvement on all 50 when released.
- Stage 2 deadline and live Q&A are still TBD. Both students must attend the Q&A.
- Hidden-case grading cannot be claimed from local tests. Nothing in this workspace has been uploaded to Canvas.

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

## Correct Stage 2 comparison

`releases/stage1_candidate/` freezes the current optimized method. It is a technical candidate, not a claim that it was submitted. Before Stage 2, retain the source snapshot of the method actually adopted for Stage 1. The simpler `src/baseline.cpp` is an engineering ablation and must not replace that Stage 1 comparator.

After evaluating both methods separately on each published stage, compare the four CSVs:

```bash
python3 tools/compare_stage_results.py \
  --pair stage1 stage1_method_on_stage1.csv stage2_method_on_stage1.csv \
  --pair stage2 stage1_method_on_stage2.csv stage2_method_on_stage2.csv \
  --output results/stage2_comparison.json
```

The tool rejects unmatched/duplicated case sets and invalid metrics, preserves failures, and namespaces case IDs by stage. It reports area/delay/runtime differences only where both methods pass. It cannot certify CSV provenance or that an unreleased stage is complete. Retain the evaluated executable, evaluator logs and official case manifests alongside it.

## Stage 2 prototype

`src/synth_stage2.cpp` is an experimental order-screening method for the later
Stage 2 comparison. On the 25 Stage 1 cases it also passes 25/25 and meets 25/25
delay targets, with area sum 617,943 versus 622,816 and median runtime 462.188 ms
versus 603.945 ms. Per-case tradeoffs are retained in
`results/stage2-comparison-20261110.json`; see `docs/STAGE2_PROGRESS_20261110.md`.
This is not a Stage 2 submission because the additional cases are not released.

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

This private repository contains project-authored source, documentation, tests,
reports, selected measurement summaries, and the instructor-provided project
materials in `official/`. Raw run snapshots remain local.

After cloning on Linux x86, run `make` and the snapshot reproduction command
above. The Stage 1 archive, its 25 extracted cases, and the executable evaluator
are included. See `official/README.md` for original Canvas sources. Python verification needs
NumPy; the C++ executable itself has no Python dependency. Original raw run paths
in retained manifests refer to the development workstation; create a fresh run
on your own machine for new provenance and outputs.
