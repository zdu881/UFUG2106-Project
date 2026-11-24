# Transfer check, 24 November 2026

Freeze the current source ZIP and standalone R10.0 from the four-version comparison before generation. Compare both on 24 synthetic cases: four circuit families, two input relabelings and three gate-cost profiles. Use seed 112406. All functions have 12 available inputs, a 2 s synthesis limit and a delay target set exactly to an independently supplied legal witness circuit. Each solver sees only the truth table, gate costs and limits.

Measure official correctness, number meeting target, area on paired feasible cases, synthesis time and results by family. Independently replay all witnessed and synthesized circuits. Two permutations and three cost profiles exercise each underlying function; they do not provide six independent function samples. Timings are a single local observation. This is a diagnostic bank, not a model of the hidden-case distribution. Once reviewed, it becomes development data.

## Public delay sensitivity

For each released input set the new target to floor(0.9 × original target), keeping everything else fixed. Select every case where the frozen integrated circuit would now miss, and a previously saved circuit from one of the four versions already meets the tighter target. Rerun both solvers with only the changed input. This probes the ability to respond to timing pressure on solvable instances. It deliberately selects difficult cases and reuses public functions.
