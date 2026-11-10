# AI-assisted work log

2026-11-08: The user requested completion and iterative review of UFUG2106 Project. Codex read the official project PDF, syllabus and Stage 1 inputs; copied official files; invoked the evaluator help interface. No student solution or submission was present when this workspace was created.

AI assistance is confirmed for this project. Artifacts generated in this workspace are transparently AI-assisted; students remain responsible for understanding, reviewing and presenting the work accurately.

Team membership from Canvas: Zeyun DU and Xinchen HUANG, GROUP 2. Contributions by Xinchen HUANG have not been established. This file does not assign them work retrospectively.

## Actual development sequence, 2026-11-08

Codex wrote the natural-order BDD baseline, the optimized C++ implementation and an independent Python bit-lane checker. It ran the teacher evaluator, retained raw per-case measurements, and made seven successive optimization iterations. Initial complemented mapping reached 19 delay targets; balancing reached 21; subsequent ordering/affine/implication changes improved area; two-level Shannon expansion reached all 25 targets. All versions stayed semantically valid on the official cases.

Codex generated regression tests (including every Boolean function of up to three inputs), ran ASan/UBSan, tested the checker with corrupted circuits, compared search budgets, built the six-page technical report, and rebuilt/evaluated an extracted source archive. These are AI-performed activities. No claim is made that either student independently implemented, ran, reviewed or approved them.

No teammate contribution record has been supplied in this work session. Nothing has been uploaded to Canvas.

Follow-up review: Codex corrected the Stage 2 comparator definition, froze the current optimized Stage 1 candidate, wrote a failure-preserving CSV comparison tool, and verified it with six targeted tests and the existing baseline/optimized measurements. No new Stage 2 data or student contribution was claimed.

## 10 November 2026 follow-up

At the user's request to advance the project, Codex read the existing artifacts and
queried the current Canvas Stage 1 assignment, linked file metadata and announcements.
It added a snapshot-based reproduction runner, five failure-detection tests and a
small runnable teaching example. The first run passed all official cases but failed
independent verification because the Canvas-agent virtualenv lacked NumPy. That
failure was retained; dependency preflight was added and the full workflow was rerun
with the existing system Python. All 25 cases passed official and independent checks,
all delay targets were met, area summed to 622,257 and median runtime was 603.201 ms.
No synthesis algorithm, frozen Stage 1 source or historical measurement was replaced.
The proposed student division is not claimed as work performed by either teammate.

## 10 November 2026 cleanup and latest run

Removed SHA-256 checks as workflow gates, refreshed the course-material references,
and rebuilt the six-page technical report from a new snapshot run
(`results/stage1-final-20261110/`): 25/25 official PASS, 25/25 delay targets, area
sum 622,816, median runtime 603.945 ms. A clean extraction of the source archive
also rebuilt and passed 25/25 official cases and delay targets. No synthesis
algorithm change was made.
