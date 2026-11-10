# Stage 2 prototype — order screening, 10 November 2026

## Status

Stage 2 cases have not been released. This is an engineering prototype against the
25 published Stage 1 cases, not a Stage 2 submission or a claim about hidden cases.
The adopted Stage 1 comparator remains frozen in `releases/stage1_candidate/`.

Files:

- New method: `src/synth_stage2.cpp`
- Stage 1 result: `results/stage1-final-20261110/`
- Stage 2 result on the same cases: `results/stage2-final-20261110/`
- Paired comparison: `results/stage2-comparison-20261110.json`
- Generated regression: `results/stage2-regression-20261110.json`

## Method change

The Stage 1 search maps every trial order with four weights until its budget ends.
The prototype keeps a first valid candidate, then builds a wider pool of natural,
reverse, random and local-mutation variable orders. It ranks the pool cheaply by
BDD node count and maps only the most promising orders with the existing
area/delay mapping. The search still uses only legal AND2/OR2/NOT gates and the
same final candidate ranking.

The 25-case run used deterministic order generation and a fresh snapshot runner.

## Measured result on the 25 Stage 1 cases

| Measure | Stage 1 | Stage 2 prototype |
|---|---:|---:|
| Official cases passed | 25/25 | 25/25 |
| Delay targets met | 25/25 | 25/25 |
| Area sum | 622,816 | 617,943 |
| Median runtime | 603.945 ms | 462.188 ms |

Aggregate area falls by **0.78%**; median runtime falls by about **23%** on this
workstation. Per case, 7 cases have smaller area, 10 are equal and 8 are larger.
All 25 remain valid and meet their target delays. The largest area reduction is
case 003 (26,059 → 18,572). The largest area increase is case 006 (5,218 → 7,989);
its delay still meets the target. These are measured tradeoffs, not a claim of
uniform improvement.

## Verification

- Official evaluator: 25/25 PASS, 25/25 target delays.
- Independent full-table verification: 25/25 valid.
- Generated regression: 92 cases pass; 6 corrupt-circuit classes rejected.
- No Stage 1 source or historical result was replaced.

## Next work

1. When Stage 2 cases are released, snapshot them and run both methods on all 50.
2. Compare with `tools/compare_stage_results.py`; report every case, not only totals.
3. Improve the mapping side (Pareto area/delay choices) and add RAM-limit scheduling.
4. Record actual student review and contribution; do not infer it from this file.
