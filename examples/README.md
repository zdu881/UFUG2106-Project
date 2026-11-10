# Five-minute circuit walkthrough

This is a learning example, not an official benchmark or claimed teammate contribution.
Run from the project root:

```bash
make
./build/synth examples/shared_outputs.txt /tmp/ufug2106-demo-output.txt
python3 tools/check_circuit.py examples/shared_outputs.txt /tmp/ufug2106-demo-output.txt
```

Inputs are x0, x1, x2, in MSB-first row order. The outputs are
`y0 = x0 OR x1` and `y1 = (x0 OR x1) AND x2`.

| Row | x0 x1 x2 | y0 y1 |
|---:|---|---|
| 0 | 000 | 00 |
| 1 | 001 | 00 |
| 2 | 010 | 10 |
| 3 | 011 | 11 |
| 4 | 100 | 10 |
| 5 | 101 | 11 |
| 6 | 110 | 10 |
| 7 | 111 | 11 |

One valid mapping shares a single OR gate between the two outputs:

```text
4 OR2 1 2
5 AND2 4 3
4 5
```

Signal 0 is constant zero. Signals 1–3 are the inputs; new gates start at 4.
Each gate only refers to earlier signals, so the graph is acyclic.
The final line is the list of output signal IDs, not another gate.

For the costs in this example, the shared circuit has area `8 + 2 = 10`,
output arrivals `3` and `3 + 2 = 5`, and circuit delay `5`.
Do not add output delays together. The OR gate's area is counted once.
A synthesizer may emit a different equivalent circuit; inspect its actual output.

Try these explanations together before the live Q&A:

1. Substitute x0=0 and x0=1 to derive Shannon expansion.
2. Explain why row 1 is 001, not 100.
3. Explain how the checker tests all eight rows independently.
4. Change the target delay to 4: truth-table correctness and meeting the target are separate.
5. Compare the area/delay tradeoff if OR is replaced by NOT/AND/NOT.

The complete reproducible Stage 1 experiment is documented in
`../docs/PROGRESS_20261110.md`. Use its actual measurements for the report.
