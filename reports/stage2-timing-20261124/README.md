# Stage 2 timing rescue

The [report](UFUG2106_Group2_Stage2_Report.pdf) compares the frozen Stage 1 submission and the current Stage 2 delivery on both official banks. [Public measurements](public-runs.csv) include three complete repeats. [Stress measurements](stress.csv) record per-case validity, delay target, achieved delay, timing slack, runtime and memory budget. [Memory controls](memory.csv) compare the official verdict and independently capped runs.

## Reproduce a public run

From the project root on Linux with GCC, Python and NumPy:

```sh
make stage2-source
run=$(mktemp -d)
unzip -q artifacts/UFUG2106_Group2_Stage2_Source.zip -d "$run/src"
g++ -O2 -std=c++17 "$run/src/synth_stage2.cpp" -o "$run/synth"
for bank in stage1 stage2; do
  official/evaluator "$run/synth" "official/$bank" --report "$run/$bank.csv" --save-outputs "$run/$bank-out"
  python3 tools/verify_all.py "official/$bank" "$run/$bank-out" "$run/$bank.csv" "$run/$bank.json"
done
```

## Timing experiments

Each limited-resource bank uses 12 seeded gate networks and the same two exact-delay controls. The generator writes a legal circuit establishing each target as feasible. To reproduce a seed, run the following and substitute its cases directory for the official bank above:

```sh
python3 reports/stage2-headroom-20261124/limits_bank.py "$run/seed" 521021
```

In the CSV, `limits` and `fresh` identify the original seeds 261124701 and 31874271. Subsequent numeric labels identify the actual generator seed. We used the first two banks to compare cofactor decomposition, Boolean covers and grouped Shannon mapping. Further experiments exposed temporary gate buildup between outputs, the previous 12-input support limit and a late path that needed a cut rewrite after decomposition. All these banks became development data. The final freeze record identifies which later seeds were held back. The other controls use structured logic, tightened public targets and independently constructed comparator, encoder and counter circuits.

The [64 MiB reproducer](../stage2-rss-accounting-20261121/README.md) documents the official memory verdict on zero-gate projection controls. The public specification and evaluator remain unchanged. Both the original and the final Stage 2 solver versions run single-threaded.

One held-back circuit (seed 711413, case 003) still exceeds its target by one delay unit. Six further experimental variants changed logic sharing, input order or cofactor mapping; all six also reached 28. Its generating witness yields delay 27 and area 550, verified on all 16,384 rows. [Per-variant measurements](residual.csv).

## Frozen delivery

[Development and holdout history](development.csv) · [Public run that exceeded the ceiling under the old cap](earlier-budget.csv) · [Public per-process peak RSS](public-rss.csv) · [Earlier area-reserve release](../stage2-headroom-20261124/README.md).

The development banks met 168/168 timing targets. Four seeds reserved for timing validation (112431, 357139, 711413, 338111) and two additional seeds generated after the final ZIP freeze (829543, 526633) met 83/84. The 84 observations comprise 72 generated networks and twelve runs of the two common controls. Across the three public repeats, areas were 884,234, 876,694, 876,694; maximum runtime was 49.7% and peak direct RSS was 44.9 MiB. The supplementary small-table sanitizer harness exercised 2, 3, 4, 5, 7 and 10 inputs, including all 256 three-input functions. The independent checker matched every output.

Local raw evidence, the predeclared seed list, isolated builds, saved circuits and replay logs are in `results/decompose-20261124-B9wOYU/budget-vRGB`. The instructor's evaluator returned MEMORY_LIMIT on the eight-output zero-gate projection and PASS on the single-output control. The original adjudication is retained in `results/decompose-20261124-B9wOYU/wire8.csv` and `wire1.csv`.

On 22 November the teaching team confirmed that Stage 1, Stage 2 and hidden inputs omit `ram_limit`; the grading server has 48 GB total RAM. Replaying seed 711413 case 003 with the ninth line omitted and again with the original 48 MiB limit gave the same area (2,105) and delay (28). Both checked all 16,384 assignments. Additional cover and residual-factor alternatives did not shorten the critical output.
