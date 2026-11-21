# UFUG2106 evaluator memory accounting

The Stage 2 assignment linked Canvas file 869388 on 21 November 2026. The download
matched the local evaluator byte for byte. All three runs below used the same binary
and a 65,536 KiB limit. All three solvers exited 0, and independent truth-table
replay matched all 1,048,576 assignments in each case.

| Synthetic input | Official result | Child peak in evaluator wait4, KiB | Solver peak in separate capped run, KiB |
|---|---|---:|---:|
| Submitted-source candidate, structured 20-input/8-output function | MEMORY_LIMIT | 106,680 | 10,112 |
| Reference solver, 1 copy of an input wire | PASS | 28,680 | 14,540 |
| Same reference solver, 8 copies of the same wire | MEMORY_LIMIT | 107,616 | 28,928 |

The wire example needs zero gates. The reference program reads the table, identifies
the shared input wire, verifies every row and writes the output IDs. The included
fixture and program can be shared without the group's synthesis source or course data.

## Reproduce on Linux

Use the unmodified course evaluator and a fresh output directory. Run these commands
from the unpacked attachment with Python 3, GNU time and util-linux installed.

```sh
chmod +x projection_case.py
work=$(mktemp -d)
./projection_case.py --generate "$work/eight.txt" 8
/usr/bin/time -f 'peak %M KiB' prlimit --as=67108864 -- \
  ./projection_case.py "$work/eight.txt" "$work/circuit.txt"
cat "$work/circuit.txt"
```

The expected circuit is the single line `20 20 20 20 20 20 20 20`. Set
`EVALUATOR` to the path of the course binary and run:

```sh
"$EVALUATOR" ./projection_case.py "$work/eight.txt" --report "$work/eight.csv"
./projection_case.py --generate "$work/one.txt" 1
"$EVALUATOR" ./projection_case.py "$work/one.txt" --report "$work/one.csv"
```

## Grader-side correction and acceptance check

Use a lightweight evaluator-owned worker to launch and wait for the student process.
Keep the parsed truth table in the checker process. Read the student's peak from
the worker, preserving the existing time, exit-status, output and truth-table checks.
On Linux, resource-usage statistics persist across exec. A child forked from a large
checker can therefore carry a high peak into a small executable. [Linux getrusage(2)](https://man7.org/linux/man-pages/man2/getrusage.2.html)

The following control compares both launch arrangements. GNU time serves as the
lightweight worker in this experiment. The loaded parent holds the source text and
parsed tokens. Both arrangements run an additional process that actually touches
80 MiB.

```sh
python3 compare_accounting.py ./projection_case.py "$work/eight.txt" "$work/control"
cat "$work/control/comparison.json"
```

With the same loaded parent, the eight-wire program measured **73,380 KiB** with
direct accounting and **28,880 KiB** through the worker. The genuine over-limit
control measured **91,216 KiB** through the worker. The corresponding values for
the group's solver were 73,468, 10,112 and 91,212 KiB. Under both launch
arrangements each solver produced byte-identical circuit outputs in the paired runs.

The course-side acceptance check is a PASS for both wire cases, a memory rejection
for the genuine over-limit control, and ordinary functional verification on the
structured case. The distributed evaluator still reports the failures above.
The final grading server has not been tested. [Recorded measurements](observed.json)
