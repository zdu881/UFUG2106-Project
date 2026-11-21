# Questions for the joint project defence

## Trace one circuit

Use the handout circuit `y0 = x0 OR x1`, `y1 = y0 AND x2`. When the OR costs 8 area units and 3 delay units, and the AND costs 2 and 2, the circuit occupies **10 area units** and the last output arrives after **5 delay units**. The OR is shared. Count its area once and include its delay on each path that crosses it.

1. **Which row supplies the truth value?** With three inputs, rows 1, 2 and 4 encode 001, 010 and 100. The first input is the most significant bit. Trace rows 1 and 4 to catch a reversed input order.
2. **Why does Shannon expansion preserve the table?** Substitute 0 and 1 for the selector in `(NOT x AND f0) OR (x AND f1)`. Each substitution leaves its matching cofactor. Identical cofactors and gates can then share one node.
3. **When is the shorter expression valid?** If every assignment satisfying `f0` also satisfies `f1`, then `f0 OR (x AND f1)` has the required two cofactors. At x=1 the implication makes `f0 OR f1` equal to `f1`.
4. **How can more gates shorten a circuit?** Expanding two decision levels allows path selectors to run in parallel. A balanced OR tree combines the terms. Compute each arrival as the latest input arrival plus the gate delay, then take the latest output. Compare that gain with the extra area.
5. **How do we catch mapping errors?** Check every input assignment against the original table. Generated cases exercise constants, complementary outputs and input ordering. [Method and proof](METHOD.md)

## Interpret the measurements

- **What did PASS measure?** The engineering baseline passed all 25 public cases and met 12 delay targets. Report correctness, delay, area and synthesis time separately. The 43.6% aggregate area reduction uses case-specific gate costs. The official numerical grade formula is unpublished.
- **Which program was compared?** The submitted Stage 1 solver searches variable orders. Stage 2 first shortlists orders by BDD node count and then uses bounded mapping trials to prioritise refinement by actual delay and area. On the two original public banks it saved 28,837 area units; 16 cases improved, 25 tied and nine regressed. [Public comparison](STAGE2_SEARCH_20261120.md)
- **Why change just the input order?** A relabelled copy of the old circuit has identical gate costs and path lengths. The timed search may still find a different mapping. Across the 50 transformed functions the new solver saved 64,360 area units. Removing the largest gain leaves 22,392. The original public functions and their renamed copies are paired observations. [Independent replay](../results/input-permutation-20261121/README.md)
- **Where does it still fail?** The official evaluator passed the 128 MiB synthetic case and rejected the 64 MiB case. The circuit passed independent full-table checking. The grading evidence records the official memory failure. [Resource experiment](STAGE2_SEARCH_20261120.md#64-mib-stress-result)
- **What remains before the joint Q&A?** Each member should identify one actual Stage 2 contribution, show its source or measurement, and explain how the other member checked it. [Teamwork record](TEAMWORK_LOG.md)

## Defend the final Stage 2 choice

- **Can two correct circuits be mixed?** Each donor implements the same output
  function at the same index. Replacing that output preserves its value for
  every input. Removing a reference frees a gate only after its last live
  consumer is removed. In the report example, two six-gate candidates produce
  a three-gate circuit.
- **Why is the shallow construction correct?** Exactly one upper-input row
  selector is active. In sum-of-products form, only its column function can
  contribute. In product-of-clauses form, all other rows contribute one. Local
  four-entry lookup tables group column assignments with a shared selector.
- **What did we measure?** The last version saved 11.57% area and 23.56% total
  synthesis time against the previous packed solver across 50 public cases.
  Median circuit delay increased from 41.5 to 50.5, with all 50 targets met.
  Output selection saved 26,480 area units in the eight-case ablation. The final
  30 generated tests were held back from tuning. Official hidden scores and
  class rankings remain unknown.
