# Stage 2 search update — 20 November 2026

Current measurements and the 64 MiB diagnosis are in the [case 006 follow-up](#case-006-follow-up).

The early influence-order candidate passes all 50 public cases and their
delay targets. Total area is 1,180,657, versus 1,206,033 for the submitted
Stage 1 method and 1,308,731 for the previous prototype.

## Change

On Stage 2 case 002, tracing showed that natural-order mapping consumed about
1.2 seconds of the 1.5-second search budget. Subsequent BDD screening and short
mapping slots failed to finish the better influence-order circuit.

The candidate retains the natural-order mappings, then maps the descending
influence order before screening. It tries weight 1 first; if that circuit
exceeds the delay target, it tries weight 16. The early probe stops at its
first feasible circuit. Completed candidates still compete by delay excess,
area and delay. The existing BDD shortlist and measured-cost refinement follow.

This changes search scheduling only. The gate mapper, budget, single-threaded
two-path CLI and optional-search RAM checks are unchanged.

## Measurements

| Case bank | Method | PASS / targets | Total area | Median runtime, ms |
|---|---|---:|---:|---:|
| Stage 1 | Submitted method | 25 / 25 | 622,257 | 603.236 |
| Stage 1 | Previous prototype | 25 / 25 | 616,538 | 606.259 |
| Stage 1 | Current candidate | 25 / 25 | 614,824 | 603.344 |
| Stage 2 | Submitted method | 25 / 25 | 583,776 | 602.859 |
| Stage 2 | Previous prototype | 25 / 25 | 692,193 | 380.847 |
| Stage 2 | Current candidate | 25 / 25 | 565,833 | 465.182 |

Against the submitted method, aggregate area falls 2.10% and total synthesis
time falls from 39.887 to 34.884 seconds. Fifteen cases have smaller area,
23 are equal and 12 are larger. Against the previous prototype, area falls
9.79%, while total synthesis time rises from 33.810 to 34.884 seconds.

| Case | Submitted area | Previous area | Current area / delay |
|---|---:|---:|---:|
| Stage 2 / 002 | 50,694 | 143,822 | 50,694 / 77 |
| Stage 2 / 005 | 142,405 | 164,968 | 142,405 / 40 |
| Stage 2 / 014 | 65,808 | 79,016 | 65,808 / 70 |
| Stage 1 / 015 | 33,836 | 32,193 | 32,193 / 78 |

The submitted and previous-method figures come from the retained
[20 November comparison](../results/stage2-public-20261120/comparison.json).
Current measurements are one sequential run per bank. Search is wall-clock
bounded, so completed candidates can vary between runs. Stage 1's aggregate
saving is concentrated in case 003: excluding it, the area change is +54.

## Validation and reproduction

- 50 official cases and independent full-table checks pass, with all targets met.
- 92 generated cases pass; all six checker corruption classes are rejected.
- 16 unit/integration tests pass, including the four RAM-scheduling checks.
- The frozen comparator matches the source ZIP submitted on 17 November.
- The 25 new input files match the released Stage 2 archive.

Evidence: [submitted-method comparison](../results/stage2-search-20261120/comparison-frozen.json),
[previous-prototype comparison](../results/stage2-search-20261120/comparison-prototype.json),
[Stage 1 run](../results/stage2-search-20261120/final-stage1/manifest.json),
[Stage 2 run](../results/stage2-search-20261120/final-stage2/manifest.json),
[generated regression](../results/stage2-search-20261120/regression.json), and
[unit tests](../results/stage2-search-20261120/unit-tests.log).
The evaluated snapshots retain the early influence-order version. See the follow-up below for the current source.

```bash
make stage2
python3 -m unittest discover -s tests -p 'test_*.py'
python3 tools/evaluate_release.py --source src/synth_stage2.cpp --cases official/stage1 --output results/new-stage2-on-stage1
python3 tools/evaluate_release.py --source src/synth_stage2.cpp --cases official/stage2 --output results/new-stage2-on-stage2
python3 tests/regression.py build/synth_stage2 results/new-stage2-regression.json
```

## Open items after the first run

Stage 2 case 006 still increases area from 5,196 to 8,208. The earlier 64 MiB
stress-case memory failure is unresolved. Final Stage 2 reporting must include
the selected method's comparison and actual individual contributions.
Existing report PDFs were not changed.

## Case 006 follow-up

A trace of the submitted Stage 1 method on Stage 2 case 006 found a shuffled
order with 656 BDD nodes and a mapped area of 5,196 at delay 48. The previous
prototype returned 8,208 at the same delay. The current search interleaves
shuffles with single-variable moves before screening. Within each BDD order,
the last two variables can describe only 16 Boolean functions. A four-bit truth
table indexes their shared literals, avoiding repeated construction of those
subgraphs. Candidate selection still uses mapped delay excess, area and delay.

| Run on both banks | Stage 1 area | Stage 2 area | Total synthesis time |
|---|---:|---:|---:|
| Submitted Stage 1 method | 622,257 | 583,776 | 39.887 s |
| Early influence-order candidate | 614,824 | 565,833 | 34.884 s |
| Current Stage 2 source | 611,427 | 565,769 | 33.397 s |

All 50 circuits passed both the official evaluator and full-table verification
and met their delay targets. Compared with the submitted method, 16 areas
decreased, 25 stayed equal and nine increased. Against the preceding prototype,
Stage 2 case 006 saved 3,012 area units while case 024 used 3,456 more.
Together the 25 new cases saved 64 units. Case 015 in the first bank retained
area 32,193 and delay 78. The preceding trial of the same algorithm measured
611,301 and 565,769 total area in the two banks. The deadline can change which
orders finish mapping, especially on larger inputs.

The [50-case comparison with the submitted source](../results/stage2-followup-20261120/vs-submitted.json)
and [comparison with the previous prototype](../results/stage2-followup-20261120/vs-previous.json)
contain the per-case area, delay and runtime. The exact production source and
commands are retained with the [first-bank run](../results/stage2-followup-20261120/release1/manifest.json)
and [second-bank run](../results/stage2-followup-20261120/release2/manifest.json).
All 16 unit tests passed. The generated regression passed 92 cases, including
all 16 two-input functions, and rejected six invalid circuit variants.
[Regression results](../results/stage2-followup-20261120/regression.json)

### 64 MiB stress result

The official evaluator still reports MEMORY_LIMIT on synthetic case 102.
The 128 MiB and unlimited versions both passed. On a traced single-case run
before the search change, the child exited successfully and the evaluator's
wait4 call returned 106,224 KiB. Sampled high-water marks during that run were
106,508 KiB for the evaluator worker and 12,000 KiB for the running synthesizer.
[Official rerun](../results/stage2-followup-20261120/memory-final.log) ·
[Process samples](../results/stage2-followup-20261120/memory-peaks.json) ·
[System-call trace](../results/stage2-followup-20261120/wait4.raw)

With the final source and the same input, increasing the parent's allocation
by 96 MiB changed wait4's child peak from 11,904 to 104,676 KiB. The sampled
peak after exec changed from 12,012 to 12,032 KiB. Both output circuits passed
all 1,048,576 assignments. This reproduces the inflated child peak across
fork and exec and explains the discrepancy with the observed solver footprint.
The official 64 MiB result remains a failure.
[Small parent](../results/stage2-followup-20261120/parent-0.json) ·
[Large parent](../results/stage2-followup-20261120/parent-96.json) ·
[Reproduction script](../results/stage2-followup-20261120/parent-rss.py)

### 21 November follow-up — separate the solver's RAM from the evaluator's RAM

Recreate synthetic case 102 and apply the 64 MiB cap to the solver's entire
address space on Linux. The independent checker runs after the solver exits.

```sh
make stage2
work=$(mktemp -d)
python3 tools/generate_stress.py "$work/case_102.txt" --ram-kib 65536
/usr/bin/time -f 'peak RSS %M KiB' prlimit --as=67108864 -- \
  ./build/synth_stage2 "$work/case_102.txt" "$work/out.txt"
python3 tools/check_circuit.py "$work/case_102.txt" "$work/out.txt"
official/evaluator ./build/synth_stage2 "$work/case_102.txt" --report "$work/official.csv"
```

The packed solver used 10,112 KiB peak RSS and passed all 1,048,576 assignments.
The unmodified evaluator returned `MEMORY_LIMIT` with solver exit code 0 and a
`wait4` peak of 106,836 KiB. In a separate control, adding 96 MiB to the
launching parent raised `wait4` from 10,116 to 104,540 KiB. The sampled solver
peaks after exec were 10,308 and 10,324 KiB. A course-side fix is to let a
fresh, lightweight process launch and wait for the solver, with truth-table
parsing and output checking in a separate process.
[Paired measurements and replay results](../reports/stage2-packed-20261121/resource-study.json)

The [input-renaming control](../results/input-permutation-20261121/README.md) replays both sources on 50 equivalent truth tables and independently verifies reference circuits under the new input names.
