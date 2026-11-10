# Stage 1 technical review — 2026-11-08

## Outcome

Technical Stage 1 gates are supported by actual results: both programs pass 25/25 official cases; the optimized program meets 25/25 delay targets, versus 12/25 for the baseline. The optimized sum of case areas is 622,257 versus 1,103,455 (43.6083% lower), with smaller area on 23/25 cases. Median runtime rises from 4.628 ms to 604.023 ms. These are measured engineering results, not an official grade or proof of minimum area.

## Requirement-by-requirement audit

| Requirement | Authoritative evidence | Finding |
|---|---|---|
| Legal circuit syntax and full truth-table agreement | `results/final/official.log`, `optimized.csv`, `independent.json` | 25/25 PASS; every output checked on 3,395,584 assignments |
| Gate area and longest-path delay | Official CSV versus independent parser/evaluator | Exact agreement for all 25; baseline independently checked too |
| Correct input bit order and multiple outputs | Generated exhaustive Boolean-function families, projection/constant tests, official cases | PASS |
| Correct constant-one handling and output IDs | Generated tests include both constants and mixed outputs; independent syntax checks | PASS |
| Single-threaded executable, exact two-path CLI | Source inspection, Makefile, clean extracted rebuild | PASS; no subprocess or worker threads in synthesizer |
| Runtime constraints | Official evaluator per-case results | All released cases pass; timing is local Ubuntu 22.04.5/GCC 11.4 |
| Positive memory constraint | Generated n=20, m=8 case, 10,000 ms and 131,072 KB limits | Official PASS; independently verified |
| Optimization quality | Baseline, versions 1–7, 250/500/1500 ms budget comparisons | All target delays met only by retained budget among tested capped profiles; area/runtime tradeoff disclosed |
| Memory safety / undefined behavior | `results/final/sanitizers.json` and `.log` | 92 generated cases pass ASan/UBSan/leak checks |
| Verifier sensitivity | `results/final/regression.json` | Six corrupt-circuit classes rejected |
| Source archive integrity and clean build | `artifacts/ufug2106-project-20261108/clean_package.json`, clean build/evaluator logs | Rebuilt extracted sources; 25/25 official PASS and 25/25 delay targets |
| Report completeness | Six-page technical PDF | Method, proof outline, all 25 rows, tradeoffs, tests, reproduction and remaining gates |
| Academic submission eligibility | Official Course Information p.16 | NOT CLEARED: published policy prohibits GAI completing assessments; later project exception unconfirmed |
| Team contributions | Canvas membership plus actual work log | Membership confirmed; student/teammate activities not invented |
| Stage 2 | Official handout p.4 | INCOMPLETE: additional 25 cases and deadline not published |
| Joint live Q&A | Official handout p.4 | FUTURE: both students must attend; date TBD |
| Hidden-case grade | Official teacher evaluation | UNAVAILABLE; cannot infer it from local tests |

## Review findings resolved

1. Removed redundant literal gates in the baseline and preserved that baseline separately.
2. Kept a completed baseline candidate so optional search cannot replace it with a worse candidate under the declared delay/area selection rule.
3. Added candidate rollback, gate pruning and independent final-circuit accounting.
4. Added cooperative search time checks; sanitizer and mutation checks cover risks from temporary gate IDs and complementary literals.
5. Added four-way Shannon mapping after all earlier variants left four delay targets unmet; this reached all 25.
6. Evaluated faster search caps instead of hiding runtime costs. Both 250 and 500 ms missed case 024's target, so the 1500 ms cap remains.
7. Verified the source archive after extraction; the newly built executable also passed all 25 official cases and delay targets.

## Remaining risks and decisions

The exact official scoring formula and excellence cutoff are absent from the supplied material. A numerical forecast such as “95/100” would be unsupported. Aggregate area is not the official score. Time-limited search can produce different candidate counts on other CPUs; the retained CSVs report one actual run and the clean-extraction rerun is separate evidence. Arbitrarily tight future time/memory limits can still defeat the first construction.

Do not mark the overall project complete: Stage 2 data, genuine contributions, live Q&A and assessment eligibility remain unresolved. No Canvas submission or message to the teammate/teacher has been sent.

## Follow-up review: Stage 2 comparator corrected

The official Stage 2 requirement compares the Stage 1 method with the later improvement. The natural-order baseline is not automatically that comparator. The current optimized candidate has been frozen in `releases/stage1_candidate/` with a SHA-256 manifest. The actually adopted/submitted method still needs to be identified when eligible.

Added `tools/compare_stage_results.py`: it preserves failures, namespaces reused filenames by stage, rejects incomplete or duplicated case sets, and reports metrics only on valid paired results. Six targeted tests passed, including validity regressions/recoveries, zero-area cases, identical filenames across stages and invalid numeric data. On existing engineering-ablation CSVs it independently reproduces 25 paired passes, 12→25 target successes and 23 cases with smaller area. This run is explicitly an engineering ablation, not Stage 2 evidence.
