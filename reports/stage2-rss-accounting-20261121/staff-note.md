Subject — UFUG2106 Stage 2: reproducible 64 MiB memory-accounting discrepancy

Dear UFUG2106 teaching team,

Could you check how the final grader measures the student program’s peak memory?
With the evaluator linked from the Stage 2 assignment (Canvas file 869388), we
can reproduce a MEMORY_LIMIT on a valid circuit with zero gates. The input has
20 inputs and eight identical outputs, each wired to the last input. The reference
program checks every row. It exits 0 and peaks at 28,928 KiB under a hard
65,536 KiB address-space limit. The evaluator records 107,616 KiB for the child.
With one output wire, the same reference program passes.

The attached small reproducer contains synthetic inputs generated on demand and
a standalone reference program. A separate launcher measured 28,880 KiB for the
eight-wire program and correctly flagged a process that actually touched 80 MiB.
Our Stage 2 solver shows the same discrepancy on a structured synthetic case.

Would you verify these controls on the grading server and confirm how affected
cases will be assessed? Please let us know if a revised evaluator is available.

Thank you,
UFUG2106 Group 2
