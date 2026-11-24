# UFUG2106 Group 2 · Stage 2

From the extracted directory on Linux, compile and run the single-threaded solver:

```sh
g++ -O2 -std=c++17 synth_stage2.cpp -o synth
./synth INPUT_PATH OUTPUT_PATH
```

The Round 5.1 source attribution is in `round51/ORIGIN.md`.

On eligible inputs, the final pass rewrites four-input cuts in sum-of-products
form. The original circuit is retained unless the whole replacement meets
the target and has strictly smaller weighted area. Inputs specifying a
positive memory limit bypass this pass.


A final multi-output pass tests whether the truth table factors through a
shared one-bit signal. The circuit is chosen by delay excess, area and delay.

The timing search reserves one integer delay unit when the target exceeds one.
It rewrites small cones on late paths and can reuse cheaper outputs from a second, functionally equivalent circuit
if the complete result ranks better under the reserved target. Positive RAM
limits disable storage of the second circuit.

When the best circuit misses the stated delay, the solver can resynthesize
outputs with up to 14 relevant inputs from their exact truth tables. It tries
Boolean decomposition, implicant covers and grouped cofactors, then keeps a
replacement only if the whole circuit's maximum delay decreases.
If the reconstructed circuit remains late, the same budget can rebalance it
and remap small cuts on its critical paths.

The main search uses up to 45% of a positive time allowance, capped at 3.3 s.
Without an explicit limit, its original 1.8 s budget applies.
