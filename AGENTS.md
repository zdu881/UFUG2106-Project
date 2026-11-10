# UFUG2106 project workspace

Entry point for coding agents. Read `docs/WORKSPACE_HANDOFF.md` and
`docs/PROGRESS_20261110.md` before substantive changes.

## Repository

Remote: https://github.com/zdu881/UFUG2106-Project (private).
The real workspace is `/home/nyz/2106Proj`; the former Canvas project path is a
compatibility symlink. Do not modify the surrounding Canvas-monitor repository
as part of this project.

## Project boundaries

- Build a single-threaded CPU program with exactly two CLI arguments: input and output paths.
- Match the truth table on every assignment and emit only the permitted AND2/OR2/NOT format.
- Keep `releases/stage1_candidate/synth.cpp` frozen as the Stage 1 comparator.
- `src/synth_stage2.cpp` is the experimental Stage 2 prototype; do not replace the
  frozen Stage 1 source with it.
- Keep `src/baseline.cpp` as an engineering ablation; it is not automatically the
  Stage 2 comparator.
- Never overwrite historical measured results. Use a fresh results directory.
- Keep generated binaries, raw run snapshots and delivery archives out of commits
  unless explicitly requested.
- Record actual teammate contributions; do not invent reviews, experiments or submissions.
- AI assistance is confirmed for this project. Describe the work accurately; it is not
  claimed to be independently student-written.
- SHA-256 manifests are not a workflow gate. Reproducibility is demonstrated by the
  retained evaluator CSVs, logs, independent verification and actual reruns.

## Working commands

```bash
make
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/evaluate_release.py --output results/a-new-run
python3 tests/regression.py ./build/synth results/regression.json
```

Use system `python3` on this machine: it has NumPy. The separate Canvas-agent
virtualenv does not. For production algorithm changes, run generated regression
cases and the complete official/independent verification. For documentation-only
edits, verify links/paths instead of rerunning expensive experiments.
