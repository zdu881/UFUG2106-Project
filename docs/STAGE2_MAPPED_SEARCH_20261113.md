# Stage 2 mapped-order scheduling — 13 November 2026

## Release availability

At 17:30 Beijing time on 13 November, authenticated read-only Canvas requests
checked all pages of course 3869 assignments, visible files, modules and
announcements. No additional Stage 2 assignment or case archive was found in
these endpoints. The three Stage 1 attachment IDs, sizes and modification dates
also match the prior check. Therefore this work evaluates the 25 existing Stage 1
cases plus explicitly synthetic resource tests, not an additional official set.
The visible file listing excludes the assignment attachments, which were checked
separately by their linked IDs. Evidence: `results/release-check-20261113/status.json`
and `linked-files.json` in the same directory.

Sources: [Stage 1 assignment](https://hkust-gz.instructure.com/courses/3869/assignments/30398),
[course files](https://hkust-gz.instructure.com/courses/3869/files),
[announcements](https://hkust-gz.instructure.com/courses/3869/announcements).

## Diagnosis and implementation

Instrumentation of the old prototype and 25% portfolio shows that case 015's
better order (6,619 BDD nodes) was mapped by the old prototype but not the
portfolio. The portfolio consumed screening time on the influence order
(7,881 nodes); its best subsequently mapped order had 6,959 nodes. The problem
was loss of search coverage, not an incorrect gate rewrite. Instrumented source
copies and diagnostics remain in `results/order-investigation-20261113/`.

The updated `src/synth_stage2.cpp` removes that pre-screen portfolio. It retains
35% of the post-natural-order time for BDD screening, then proposes up to four
node-count winners plus influence and reverse-influence orders. Each gets a
bounded weight-4 mapping probe, sharing two-thirds of the remaining time.
Completed probes are ranked by actual pruned circuit delay excess, area and delay;
the remaining time refines the best measured orders with weights 1, 0 and 16.
Every completed candidate remains eligible for the final output. Node count is
still a shortlist heuristic; there is no optimality or per-case improvement guarantee.

Three sequential repetitions per version reproduced case 015 at 32,193 / delay 78
for the old prototype, 33,867 / 79 for the portfolio, and 32,193 / 78 for the new
method. The target is 81. See `results/order-investigation-20261113/repeats.json`.

## Final official-set evaluation

`results/stage2-mapped-final-20261113/`: **25/25 official PASS, 25/25 independent
full-table checks, 25/25 delay targets**, area sum **615,861**, median synthesis
runtime **604.336 ms**. The source snapshot matches the current experimental source.
The frozen Stage 1 rerun was 622,257 / 603.747 ms; aggregate area is 1.03% lower.
The previous portfolio was 616,606 / 588.341 ms. Against that portfolio, area falls
745 (0.12%) and median runtime rises 2.72%. This is a small aggregate gain with
per-case losses, not a uniform improvement or statistical performance claim.

| Case | Portfolio area | New area | Delta |
|---|---:|---:|---:|
| case_001.txt | 56,151 | 55,390 | -761 |
| case_013.txt | 11,810 | 11,826 | +16 |
| case_015.txt | 33,867 | 32,193 | -1,674 |
| case_021.txt | 27,754 | 29,372 | +1,618 |
| case_023.txt | 159 | 215 | +56 |

Other 20 cases have equal area. Against frozen Stage 1 there are 8 lower, 9 equal
and 8 higher-area cases. Full paired reports:
`results/stage2-mapped-final-vs-portfolio-20261113.json` and
`results/stage2-mapped-final-vs-stage1-20261113.json`.
The earlier mapped experiment (`results/stage2-mapped-20261113/`) had area 616,538;
retain it as a separate run. Deadline truncation affects which candidates finish.

### Which benchmark drives the total?

A post-hoc check of the 25 matched rows finds a net saving of 6,396 area units against the frozen Stage 1 rerun. Case 003 alone saves 7,487. Across the other 24 cases the prototype uses **1,091 more** units in total. The median paired area change across all 25 cases is zero. The aggregate gain matters for the cost of this particular benchmark mix; a typical-case speed or area advantage needs separate evidence. [Frozen comparator CSV](../results/stage1-reproduction-20261113/official.csv) · [Prototype CSV](../results/stage2-mapped-final-20261113/official.csv)

## RAM scheduling and remaining limits

Optional search now reads current resident memory from Linux `/proc/self/statm`.
At 75% of a nonzero input RAM limit (KiB), it stops further optional search and
retains the best completed circuit. Checks occur before mapping/search admission
and periodically during BDD construction and gate mapping. Zero or omitted limits
leave this guard disabled. Optional-stage allocation failures also stop search.
The first circuit, input allocation, balancing and finalization are not hard-capped;
25% headroom is a heuristic, not proof that arbitrary tiny limits can be met.

The initial implementation used `ru_maxrss`. A child can inherit the launcher's
pre-exec high-water mark, so this can stop optimization unnecessarily. The final
implementation uses current RSS, and an integration test forks from a parent
with 64 MiB of touched memory to prevent this regression.

Synthetic n=20, m=8 stress inputs were derived from `tools/generate_stress.py`:

- Final official evaluator: the 128 MiB and unlimited cases pass, area 668 and
  delay 68; both independently verified with matching metrics. The 64 MiB case
  reports `MEMORY_LIMIT` despite exit code 0. The overall run is correctly FAIL:
  `results/stage2-memory-final-20261113/`. An earlier 24 MiB failure is retained
  in `results/stage2-memory-20261113/`.
- Direct execution under an OS-enforced **32 MiB address-space limit** passes all
  four variants (128 MiB, 64 MiB, unlimited and 12 MiB declared budgets), with
  peak RSS 11,776–11,904 KiB and full-table independent verification. The 12 MiB
  budget stops optional search after one candidate (area 978, delay 96); the
  others optimize to area 668, delay 68. Evidence and exact launch settings:
  `results/stage2-memory-direct-20261113/results.json` and
  `results/memory_direct_check_20261113.py`.

These observations distinguish scheduler behavior from the teacher evaluator's
memory verdict. The cause of the teacher/direct discrepancy is not established;
do not claim the 64 MiB official test passes or that RAM compliance is complete.

## Validation and next work

- 15 unit/integration tests pass, including four new resource-scheduling tests.
- 92 generated regression cases pass; all six checker corruption classes rejected.
  Evidence: `results/stage2-mapped-final-regression-20261113.json`.
- `make stage2` builds the experiment; default Stage 1 sources and frozen comparator
  remain unchanged. The program still takes exactly input and output paths and
  uses one CPU thread with only AND2/OR2/NOT output.

Next: investigate the evaluator's memory measurement discrepancy and the case 021
area regression, then compare both methods on additional released cases once
available. No submission or student review is claimed. All implementation and
experiments here were performed with AI assistance.
