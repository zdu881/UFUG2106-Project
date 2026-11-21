# Stage 1 component and budget study — 16 November 2026

This is a new AI-executed study of the frozen Stage 1 method, not a change to
its implementation or a claim of a higher official score. All runs use the
same workstation, official 25 cases and evaluator. Two sequential repetitions
reverse configuration order in the second repetition. There is no concurrent
benchmark execution. Wall-clock completion and hardware variability still
limit attribution; two repetitions are not a statistical significance test.

| Configuration | Aggregate area range | Median runtime range (ms) | Targets each run |
|---|---:|---:|---:|
| Full Stage 1 | 622,257 | 603.380–603.443 | 25/25 |
| No order search | 722,588 | 85.954–88.046 | 25/25 |
| No balancing | 683,116–683,144 | 603.217–603.930 | 25/25 |
| No affine mapping | 624,890 | 605.523–606.684 | 25/25 |
| 250 ms cap | 1,020,097–1,021,703 | 253.086–253.280 | 24/25 |
| 500 ms cap | 817,788 | 503.238–503.408 | 24/25 |
| 750 ms cap | 713,843–714,363 | 603.687–604.132 | 24/25 |

All 350 official case-runs pass, including runtime/resource checks. Independent
verification exhaustively checks every output and agrees with evaluator area
and delay. Each of seven configurations passes 92 generated cases; the six
checker corruption classes are rejected in each regression run. The three
short-cap configurations miss case 024's delay target in both repetitions.
Functional validity and delay-target coverage are deliberately separate.

## Exact scope of each switch

- No order search limits the trial loop to the natural order; all mapping weights,
  initial construction, influence preprocessing and the original budget remain.
- No balancing returns the input net unchanged from the balancing function.
- No affine mapping bypasses that mapping path, but retains affine detection,
  complemented edges, sharing, implication rewrites and other mapping logic.
- Short caps change only the search ceiling from 1500 to 250, 500 or 750 ms.
  The first candidate is still unbounded by that deadline.
- The control source is byte-identical to the frozen Stage 1 source.

Order search, balancing and affine mapping have conditional area benefits in
these runs: removing them increases total area by 16.1%, about 9.8% and 0.42%,
respectively. These effects are not additive. Disabling work can permit other
candidates to complete before the deadline; improvements on individual cases
can occur even if aggregate area worsens. The paired CSV retains these details.

Keep the full frozen method for this submission: it has the smallest aggregate
area among the tested configurations and covers all public delay targets.
No-order search is a genuine alternative with much shorter runtime and larger
area, also covering every public target. Without the official scoring formula,
we cannot establish that the full method scores higher. Case-specific costs
also mean aggregate area is descriptive, not an official score.

Complement-edge ablation, component interactions, tighter RAM limits and
independent-machine timing remain open. No new student experiment or approval
is inferred; these experiments were performed by Codex.

## Reproduction and retained evidence

Run from the source root with system Python 3 + NumPy and instructor materials:

```bash
python3 tools/stage1_ablation.py --output results/my-components
python3 tools/stage1_ablation.py --output results/my-budgets --variants cap_250 cap_500 cap_750
python3 tools/summarize_stage1_study.py --runs results/my-components results/my-budgets --output results/my-summary
```

Every output directory must be new. Source transformations are guarded by exact
single-occurrence anchors. Each run retains source, compiler commands, official
CSV, emitted circuits, independent verification and logs; failures are retained.
The submission includes compact evidence under `evidence/stage1-study/`:
14 official CSVs, the per-configuration regression results, summary and paired
case records. Raw snapshots/binaries remain outside the source ZIP. In the
workspace the original runs are `results/stage1-ablation-20261116/` and
`results/stage1-budget-study-20261116/`; the summary is in
`results/stage1-study-summary-20261116/`.
