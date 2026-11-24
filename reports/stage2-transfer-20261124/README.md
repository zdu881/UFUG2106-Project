# How well does the synthesis method transfer?

## Design principle

Shannon expansion writes a function as a choice between two smaller functions. When two outputs need the same subfunction, one gate network can serve both. The delay of the whole circuit comes from its longest path, while its area comes from every distinct gate. These two facts guide the search: shorten paths that block the target, then look for computations that several outputs can share. Accept changes on the measured whole circuit, including the cost of shared gates.

The implementation ranks candidates by delay excess, then weighted area, then delay. Once it meets the timing constraint, even a small area decrease can consume remaining slack. The portfolio order, time splits and 160-root cap are empirical choices. The public cases have influenced development; they measure development performance. The first candidate has no time deadline and cases with a positive RAM limit bypass the finishing pass.

## What the checks showed

| Test | Inputs | Integrated | Standalone R10.0 |
|---|---:|---:|---:|
| Original public bank | 50 | 50 valid, 50 on time | 50 valid, 50 on time |
| Public functions with a tighter delay target | 26 | 26 valid, 26 on time | 26 valid, 26 on time |
| New structured truth tables with a known feasible circuit | 24 | 24 valid, 24 on time | 24 valid, 24 on time |

“On time” here means meeting the circuit delay target. On the 50 public cases the integrated solver landed exactly at target 13 times and within 5% of target (inclusive) 24 times. Median relative delay slack was 6.62%. Maximum synthesis wall time was 49.28% of the respective case limit. None of the 50 specified a positive RAM limit. A separate million-row input has an unresolved official 64 MiB memory-limit result; see the [memory measurements](../stage2-rss-accounting-20261121/README.md).

The 26 tightened cases each had a previously verified circuit that met the new target, while the original integrated output missed it. Regenerating the circuits met all 26 new targets. The integrated circuits were smaller than R10.0 on 14, tied on 3 and larger on 9.

The 24 synthetic cases cover four base designs: two multiplexers with shared selectors, shared parity reductions, a four-bit adder with carry, and a small shared gate network. Each is relabeled twice and tested under three gate-cost profiles. The integrated solver used less area than R10.0 on 12 cases and tied on 12. All 24 witnesses and both solvers’ outputs were independently replayed on all 4,096 assignments per case. The easy shared-network instances simplify heavily. These data test four structures and one machine; they cannot estimate a hidden grade.

## The informative miss

On the first adder under cost profile 0, the solver emitted area 961 and delay 20. The generating circuit had area 152 and delay 25, exactly the permitted 25. Both implement the same table. Relabeling the inputs gave areas 934 and 152 respectively. Even beating R10.0 on both cases left a large feasible reduction unexplored. A known circuit supplies an upper bound on the smallest feasible area; its optimality is unknown.

## Next experiment

Test a general multi-output factoring step that considers a repeated Boolean subfunction once and reuses it in several outputs. Inspect whether it recovers shared propagate or carry terms in the adder without encoding an adder recognizer. Keep the ordinary solver as a paired control. Compare complete-circuit area and delay, and charge the time spent proposing unsuccessful factors. A saving on this bank would be a development result. Freeze the candidate before testing it on fresh, structurally distinct functions such as comparators, priority encoders and counter updates, including tighter resource limits.

[Protocol](protocol.md) · [Synthetic generator](generate.py) · [Synthetic case measurements](structured.csv) · [Tightened case measurements](tightened.csv) · [Original public measurements](../stage2-cuts-20261124/casewise.csv)
