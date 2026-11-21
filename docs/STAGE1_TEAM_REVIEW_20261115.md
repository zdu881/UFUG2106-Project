# Stage 1 team review and provenance — 15 November 2026

Group 2: Zeyun DU (50025854) and Jincan Li (50032637).

## Recorded student contributions

Jincan Li: the user supplied two pages of a technical review bearing his name and
the date 15 November. It reviews Stage 1 source, documentation and retained
experimental evidence. It identifies the area/delay versus synthesis-time
tradeoff, missing component-wise ablations, mandatory-first-candidate resource
limits and reproducibility limits. It recommends controlled ablations and stronger
resource handling. The supplied review does not claim new experiment execution
or implementation. The original review PDF was not provided with the screenshots.

Zeyun DU: supplied a written explanation of truth-table splitting into cofactors,
Shannon expansion, BDD subfunction sharing, mapping to AND/OR/NOT and sharing
intermediate signals between outputs. He supplied written responses to the review:

- The slower quantity is synthesis runtime, not the generated circuit's delay.
  Runtime affects evaluation, so 43.56% lower area alone does not prove a better
  overall official score.
- Aggregate gains do not identify individual optimization contributions. Variable
  ordering and rewriting may interact; controlled component ablations remain
  future work, not completed experiments.
- Completing a first candidate preserves an output if optional search stops, but
  does not guarantee that construction fits arbitrary time or memory limits.
  Stage 1 must disclose this limitation. Stage 2 memory monitoring is partial;
  its 64 MiB synthetic official-evaluator failure remains unresolved.

These are supplied written contributions. No joint meeting, independent rerun,
implementation contribution or completed live Q&A is inferred from them.

## AI-assisted work

AI assistance is confirmed for this project. Codex performed the recorded code
implementation, optimization experiments, automated checks, report generation
and packaging. The students' review and explanations above do not imply that the
code was independently student-written. Students remain responsible for
understanding, reviewing and presenting the work.

## Submission scope

The Stage 1 source remains the frozen optimized implementation. Natural-order
baseline results are an engineering comparison; this baseline is not the Stage 2
comparator. Component ablations and improved resource handling are follow-up
work. The submitted Stage 1 method must be retained for the later comparison.

## Follow-up executed by Codex — 16 November

The earlier statements about future ablations describe the supplied student
review at that time. Codex has since completed three conditional component
ablations and a same-source budget study, each with two repetitions, 25 official
and independent checks, plus generated regression checks. See
[the study](STAGE1_STUDY_20261116.md). Broader interactions and complement-edge
ablation remain open. No new student execution or approval is implied.
