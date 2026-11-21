# OpenCode handoff — 10 November 2026

17 November update: Canvas confirms Stage 1 attempt 2 submitted, with the source
ZIP and unchanged report PDF. Downloaded attachments match the local files.
See [the submission record](STAGE1_DELIVERY_20261115.md). Statements below about
not having submitted describe the earlier preparation work.

Latest delivery: [16 November evidence-strengthened package](STAGE1_DELIVERY_20261115.md),
including student review records, fresh extracted-source verification and a
seven-page report with the new two-repeat component/budget study. See
[the 16 November study](STAGE1_STUDY_20261116.md).

Follow-up: [13 November progress](PROGRESS_20261113.md) records fresh Stage 1
and Stage 2 evaluations and the experimental 25% direct-mapping portfolio.
Read it and [the mapped-order follow-up](STAGE2_MAPPED_SEARCH_20261113.md)
after this historical handoff. The Stage 1 report/package below remain
based on the 10 November measurements.

## Start here

Real repository: `/home/nyz/2106Proj` (`~/2106Proj`).
Remote: https://github.com/zdu881/UFUG2106-Project (private), default branch `main`.
The former path `/home/nyz/canvasmonitor/projects/ufug2106` now points here via a symlink.
There is only one working repository, not two diverging copies.

The complete local workspace was moved, including `.git`, ignored builds, original
course files, raw outputs and experimental history. Historical absolute paths inside
logs/manifests remain unchanged to preserve their original records. New commands
resolve paths relative to this repository.

## Actual implementation state

- C++17 baseline and optimized truth-table-to-circuit synthesizers are implemented.
- Latest verified run: `results/stage1-final-20261110/`.
- 25/25 official cases pass; 25/25 meet delay targets; all rows independently checked.
- Area sum: 622,816. Runtime median: 603.945 ms (that run on this workstation).
- Latest six-page report: `reports/UFUG2106_Stage1_Technical_Report.pdf`.
- Clean extracted-source verification: `results/package-check-20261110/`, also 25/25
  pass and 25/25 delay targets, runtime median 603.731 ms.
- Tool validation suite: 11 passing tests. Original generated/sanitizer tests are retained
  under `results/final/`; do not claim they were newly run during the workspace move.

## Important entry points

| Path | Purpose |
|---|---|
| `official/README.md` | Instructor file sources, original archive and evaluator |
| `src/synth.cpp` | Current optimized synthesis implementation |
| `src/synth_stage2.cpp` | Experimental Stage 2 order-screening prototype |
| `src/baseline.cpp` | Natural-order BDD engineering baseline |
| `releases/stage1_candidate/` | Frozen current Stage 1 candidate |
| `tools/evaluate_release.py` | Snapshot, compile, official evaluation and independent verification |
| `tools/compare_stage_results.py` | Failure-preserving paired Stage 1/Stage 2 CSV comparison |
| `examples/README.md` | Small executable circuit walkthrough |
| `docs/METHOD.md` | Algorithm and correctness argument |
| `docs/PROGRESS_20261110.md` | Most recent work and measured results |
| `docs/STAGE2_PROGRESS_20261110.md` | Stage 2 prototype method and measured tradeoffs |
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
records commands/timings and returns nonzero if a required stage fails.

Source packaging now defaults to `artifacts/ufug2106-project-20261108/` **inside this repo**;
the default date denotes the original delivery workflow. Prefer an explicit new directory:

```bash
python3 tools/package_source.py --output artifacts/my-new-package \
  --evidence results/stage1-final-20261110
```

Earlier shared delivery bundles remain at
`/home/nyz/canvasmonitor/artifacts/ufug2106-project-20261108/` and
`/home/nyz/canvasmonitor/artifacts/ufug2106-project-20261110/`.

## Current work · 20 November

The [Stage 1 receipt](../artifacts/stage1-resubmission-20261117/submission_receipt.json) records the 17 November resubmission. Stage 2 is now open and due **4 December at 23:59 Beijing time**. Its [assignment](https://hkust-gz.instructure.com/courses/3869/assignments/31028) links the revised handout and 25 further cases; live Q&A timing is pending.

The current candidate passes all 50 public cases and delay targets, with total
area 1,177,196 versus 1,206,033 for the submitted method. Stage 2 case 006 now
matches the submitted area of 5,196. The 64 MiB synthetic case still fails
official evaluation. A controlled parent-memory experiment reproduces its
inflated peak reading. Stage 2 case 024 now costs more area than in the
previous prototype. The final report still needs the selected results and
actual individual contributions. [Latest results and reproduction](STAGE2_SEARCH_20261120.md#case-006-follow-up)

The [current review PDF](../artifacts/stage2-review-20261121/UFUG2106_Group2_Stage2_Report.pdf) and [matching source ZIP](../artifacts/stage2-review-20261121/UFUG2106_Group2_Stage2_Source.zip) use the case-006 follow-up. Pages 2 and 3 give both original public banks. Page 4 gives the independent input-renaming check. The ZIP was extracted, compiled using its README and tested on Stage 2 case 006 (area 5,196, delay 48). The students' Stage 2 contribution details are pending.

On 21 November, the [updated report](../artifacts/stage2-reference-20261121/UFUG2106_Group2_Stage2_Report.pdf) incorporated Jincan's documented Stage 2 materials commit. Codex built the archived Round 5.1 research source and ran both original public banks. All 50 official checks, independent replays and delay targets passed. Round 5.1 used 1,038,445 total area versus 1,177,196 for the current candidate, though the current candidate used less area on 35 cases. The [comparison](../results/jincan-reference-20261121/summary.json) is a separate research result. The report and ZIP in the updated delivery still describe the current Stage 2 method. Method selection remains open.

## 21 November — merged Stage 2 solver

Build with `make stage2` and run `./build/synth_stage2 INPUT OUTPUT`. The BDD
and Round 5.1 searches now share a budget capped at 1,800 ms and 45% of a
positive case limit. BDD has half when both searches support the case, and the
full budget otherwise. The solver can select a different circuit for each
output and charges shared gates once. The tested source package builds with
`make stage2-source`. Reproduce one bank with
`python3 tools/evaluate_release.py --source src/synth_stage2.cpp --cases official/stage2 --output results/new-bank2`.

[Report](../reports/stage2-selection-20261121/UFUG2106_Group2_Stage2_Report.pdf) ·
[Per-case comparison](../reports/stage2-selection-20261121/comparison.json) ·
[Held-out and ablation results](../reports/stage2-selection-20261121/control-comparison.json).

## 21 November — variable-order scheduling

The [selected package report](../reports/stage2-schedule-20261121/UFUG2106_Group2_Stage2_Report.pdf) records 872,632 total area and 32.87 s of synthesis time
on 50 released inputs. It includes the submitted Stage 1 comparator, the
previous output-selection solver and a fresh generated control. Rebuild the report
with the command in the project README. The local publication branch is
`stage2-prepared-20261121`. GitHub identified the existing origin as public on
21 November. The full candidate includes restricted course-team materials.

## 21 November — constructing a circuit under a memory limit

The current candidate is the streaming-memory revision. Its four-page
[report](../reports/stage2-memory-20261121/UFUG2106_Group2_Stage2_Report.pdf)
and source ZIP use the build and reproduction commands at the top of the README.
The repository was set to private before the full Stage 2 materials were pushed.
