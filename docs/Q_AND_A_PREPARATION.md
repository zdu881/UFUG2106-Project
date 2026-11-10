# Learning and Q&A preparation

Use these as study prompts. Both teammates should be able to derive the answers and trace a small circuit themselves.

1. **Why does Shannon expansion work?** Substitute x=0 and x=1. The unselected AND term becomes zero, leaving the matching cofactor.
2. **How are truth-table rows numbered?** x0 is the most significant bit. With n=3, row 1 is (0,0,1), row 2 is (0,1,0), and row 4 is (1,0,0). Rows 1 and 4 clearly distinguish MSB-first from reversed input ordering.
3. **What is shared?** Identical BDD subfunctions and identical gates. A shared gate contributes area once, but its delay participates in every output path using it.
4. **How is circuit delay computed?** In topological ID order: arrival(v)=max(arrival(fanins))+delay(type). Take the maximum output arrival, not the sum of all gate delays.
5. **Why can a PASS miss the target?** Correctness and resource limits determine validity; exceeding target circuit delay incurs a quality penalty. The baseline has 25 PASS cases but only 12 target successes.
6. **Why does four-way expansion help?** Two sequential multiplexers put gates in series. Expanding their path conditions can move selector computations in parallel and combine terms in a balanced tree, sometimes at an area cost.
7. **What is the implication rewrite?** If f0 implies f1, f0 OR (x AND f1) equals f0 at x=0 and f1 at x=1. Without the implication, this rewrite is invalid.
8. **Why check generated functions?** Published cases alone could miss constant handling, input-order errors, complementary outputs or a mapping bug. Independent exhaustive small-function testing exercises these mechanisms.
9. **Why is 43.6% lower aggregate area not a grade?** Gate costs differ by case, the scoring formula is unpublished, and runtime increases. Report all axes rather than inventing a score.
10. **What remains for Stage 2?** Compare the actual adopted Stage 1 method with the Stage 2 improvement on all 50 public cases, explain modifications and actual team contributions, and attend the joint Q&A. Hidden cases remain unknown.
11. **What are the known resource risks?** The current synthesizer does not actively schedule against `ram_limit`, and its first construction is not a hard real-time fallback. Stage 2 work should address both without changing the frozen Stage 1 comparator.

Small exercise from the handout: y0=x0 OR x1; y1=y0 AND x2. With OR area/delay=8/3 and AND=2/2, the shared circuit has area 10 and delay 5. Replacing OR by NOT/AND/NOT can lower area under the given NOT costs, but increases the critical path; derive that tradeoff before using the evaluator.
