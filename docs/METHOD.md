# Method and correctness arguments

## Baseline

Read the complete truth table in the specified MSB-first row order. Recursively split on x0, x1, ..., x(n-1). If two cofactors are equal, eliminate the decision. Otherwise intern the tuple (variable, low child, high child) so equal subfunctions share a node. Map each decision using Shannon expansion:

f = (NOT x AND f0) OR (x AND f1).

Constant cofactors and literal functions reduce directly. A global gate hash shares identical gates. Remove gates that cannot reach any output and renumber the remaining graph in topological order.

Proof: terminals agree with their table entries. The Shannon expression selects f0 when x=0 and f1 when x=1. Induction on the number of remaining variables proves equality on every assignment. Hash sharing changes representation, not the function. Dead gate elimination changes no output.

## Optimizations

1. Complemented BDD edges normalize a node so its high edge is regular. A low bit records logical negation. Flipping both children and complementing the result preserves the selected cofactor.
2. Try natural, influence-based, reverse and seeded mutated variable orders. Influence is estimated from at most 4096 input rows solely to propose orders; all truth-table rows are still used to construct every candidate. No correctness decision is based on sampling.
3. Map both sum-of-products and product-of-sums multiplexer forms. Use De Morgan transformations to account for the supplied AND/OR/NOT area and delay. Evaluate alternatives with an area-plus-arrival heuristic; discard unused speculative gates.
4. Expand two decision levels into a balanced four-way Shannon cover for delay-oriented candidates. Each leaf is ANDed with its path condition and all terms are ORed. This can reduce sequential multiplexer depth at the cost of additional gates.
5. Check cofactor implications. If f0 implies f1, replace a multiplexer by f0 OR (x AND f1), or f1 AND (x OR f0). Use dual forms if f1 implies f0. Implication checks descend through the BDD; a work cutoff conservatively declines an optimization and cannot justify an invalid rewrite.
6. Detect affine subfunctions by induction: if f1 = NOT f0 and f0 is affine, f is f0 XOR x. Build balanced XOR trees using only legal AND2/OR2/NOT gates. This is a recognized algebraic structure, not a benchmark-specific answer.
7. Reassociate same-operator AND/OR cones only through single-fanout nodes. Pair earliest-arriving leaves first. Associativity and idempotence preserve the truth function; existing shared cones are retained.
8. Keep the original natural-order mapped candidate available. Compare final reachable circuits lexicographically by delay excess, then total area, then delay. This is an engineering choice, not an inferred official score formula.

The search deadline uses 60% of the case runtime limit, capped at 1500 ms, reserving time for finalization. A first baseline is always constructed. Later BDD/mapping work is interruptible and leaves the best completed candidate intact. The first construction and finalization are not a hard real-time guarantee under arbitrarily small limits. Official measurements and separate constrained tests are required.

## Area and delay

Area counts each emitted gate once, including a NOT gate if constant 1 is needed. No charge is assigned to inputs, constant 0 or wires. Gate arrival = max(fanin arrival) + gate delay. Circuit delay = max(output arrival). A shared gate is charged once even if it feeds several outputs. Optimization comparisons use the pruned output circuit, not the number of speculative search gates.

## Complexity and limits

A fixed-order construction visits O(m*2^n) table positions. BDD sharing often reduces representation size but its worst case remains exponential. The input itself already has m*2^n bits. Variable ordering, mapping and sharing are heuristic; global area/delay optimality is not claimed. Time-limited search can complete different numbers of candidates on different machines even with a fixed seed.

Stage 2 data and hidden cases are not currently available. Generated tests provide additional evidence but do not replace those evaluations.
