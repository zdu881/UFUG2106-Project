# Stage 2 · Area and timing headroom

The acceptance check uses all 50 released inputs together. For each fresh run sum the weighted areas, require a total below 900,000, check official correctness and delay for every input, and independently replay every output. For this release we also require at least one integer unit of delay slack on every public input. This reserve is an engineering choice. The official delay target stays unchanged.

- [Current PDF and the comparison with submitted Stage 1](UFUG2106_Group2_Stage2_Report.pdf)
- [Three full public runs and the previous release](public-runs.csv)
- [Timed, memory-limited and structured controls](controls.csv)
- [Memory controls and direct capped runs](memory.csv)
- [Policy trials on the development bank](development.csv)

Build with `make stage2-source`, extract the ZIP and compile with the included README. Use a fresh output directory for every evaluator run. For example, from the repository root:

```bash
run=$(mktemp -d)
unzip -q artifacts/UFUG2106_Group2_Stage2_Source.zip -d "$run/src"
g++ -O2 -std=c++17 "$run/src/synth_stage2.cpp" -o "$run/synth"
for bank in stage1 stage2; do
  official/evaluator "$run/synth" "official/$bank" --report "$run/$bank.csv" --save-outputs "$run/$bank-out"
  python3 tools/verify_all.py "official/$bank" "$run/$bank-out" "$run/$bank.csv" "$run/$bank.json"
done
```

The limited-resource development and second-seed banks can be regenerated with
`python3 reports/stage2-headroom-20261124/limits_bank.py NEW_DIRECTORY SEED`.
Seeds were 261124701 and 31874271; the second was evaluated after the final package was frozen. Each has 12 generated circuits and two exact-delay controls. On some generated circuits synthesis returns a valid circuit above a delay already attainable by the reference. Both controls are expected to meet their specified delay exactly. Raw circuits, official logs, replay outputs and the extracted source are local under `results/headroom-20261124-Sbc3dy/`. The official structured 64 MiB control still fails; the [earlier grader-side memory study](../stage2-rss-accounting-20261121/README.md) isolates that behavior.

The new timing repair uses the R10 cut truth-table evaluator and SOP/Shannon builders credited in [ORIGIN.md](../../src/round51/ORIGIN.md). The local code propagates required times, enumerates frontiers on late paths and ranks completed circuits. [Public direct-process RSS readings](public-rss.csv).
