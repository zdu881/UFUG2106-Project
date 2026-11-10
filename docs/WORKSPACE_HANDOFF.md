# OpenCode handoff — 10 November 2026

## Start here

Real repository: `/home/nyz/2106Proj` (`~/2106Proj`).
Remote: https://github.com/zdu881/UFUG2106-Project (private), default branch `main`.
The former path `/home/nyz/canvasmonitor/projects/ufug2106` now points here via a symlink.
There is only one working repository, not two diverging copies.

The complete local workspace was moved, including `.git`, ignored builds, original
course files, raw outputs and experimental history. Historical absolute paths inside
logs/manifests remain unchanged to preserve their original records and hashes.
New commands resolve paths relative to this repository.

## Actual implementation state

- C++17 baseline and optimized truth-table-to-circuit synthesizers are implemented.
- Latest verified run: `results/reproduction-20261110-verified/`.
- 25/25 official cases pass; 25/25 meet delay targets; all rows independently checked.
- Area sum: 622,257. Runtime median: 603.201 ms (that run on this workstation).
- Clean extracted-source verification: `results/package-check-20261110/`, also 25/25
  pass and 25/25 delay targets, runtime median 603.731 ms.
- Tool validation suite: 11 passing tests. Original generated/sanitizer tests are retained
  under `results/final/`; do not claim they were newly run during the workspace move.

## Important entry points

| Path | Purpose |
|---|---|
| `official/README.md` | Instructor file sources, original archive/evaluator and hashes |
| `src/synth.cpp` | Current optimized synthesis implementation |
| `src/baseline.cpp` | Natural-order BDD engineering baseline |
| `releases/stage1_candidate/` | Frozen current Stage 1 candidate |
| `tools/evaluate_release.py` | Snapshot, compile, official evaluation and independent verification |
| `tools/compare_stage_results.py` | Failure-preserving paired Stage 1/Stage 2 CSV comparison |
| `examples/README.md` | Small executable circuit walkthrough |
| `docs/METHOD.md` | Algorithm and correctness argument |
| `docs/PROGRESS_20261110.md` | Most recent work and measured results |
| `docs/AI_WORK_LOG.md` | Actual AI-assisted development history |
| `docs/TEAMWORK_LOG.md` | Teamwork record; do not invent missing contributions |
| `reports/` | Existing six-page technical report with original measurements |

## Environment and quick check

Linux; GCC 11.4; system Python 3.10 with NumPy 2.2.6. No GPU is needed.

```bash
cd ~/2106Proj
git status --short
git remote -v
make
python3 -m unittest discover -s tests -p 'test_*.py'
./build/synth examples/shared_outputs.txt /tmp/ufug2106-demo-output.txt
python3 tools/check_circuit.py examples/shared_outputs.txt /tmp/ufug2106-demo-output.txt
```

For a fresh full run:

```bash
python3 tools/evaluate_release.py --output results/opencode-first-run
```

The output directory must not already exist. The pipeline checks NumPy before compilation,
records input/output hashes and returns nonzero if a required stage fails.

Source packaging now defaults to `artifacts/ufug2106-project-20261108/` **inside this repo**;
the default date denotes the original delivery workflow. Prefer an explicit new directory:

```bash
python3 tools/package_source.py --output artifacts/my-new-package \
  --evidence results/reproduction-20261110-verified
```

Earlier shared delivery bundles remain at
`/home/nyz/canvasmonitor/artifacts/ufug2106-project-20261108/` and
`/home/nyz/canvasmonitor/artifacts/ufug2106-project-20261110/`.

## Next work

1. Review the implementation with both students and run the small walkthrough.
2. Reproduce the official results in the new workspace if new algorithm work begins.
3. Check Canvas for new Stage 2 material before claiming it has been released.
4. Preserve the method actually adopted for Stage 1, then compare that method and
   any Stage 2 improvement on both released case sets.

As last checked on 10 November, Stage 1 is due 19 November 2026, 23:59 Beijing time;
Stage 2 dates and live Q&A remain unconfirmed. Nothing has been submitted to Canvas.
The existing AI-policy eligibility notes are retained; technical pass results are not
an assertion of submission eligibility. Proposed student work is not completed contribution.
