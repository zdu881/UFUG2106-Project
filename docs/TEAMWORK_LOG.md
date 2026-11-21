# Teamwork record

Project group: GROUP 2. Members (updated by Zeyun DU): Zeyun DU (50025854) and Jincan Li (50032637).

No teammate code or agreed division of work was provided at workspace creation. Do not replace this statement with invented activities.

Record future real activities with: date, participants, decision, owner, resulting code/experiment/document, and review outcome. The Stage 2 report requires an accurate individual-contribution and coordination section.

Suggested division for discussion, not an existing agreement: one student explains and implements synthesis improvements; the other independently checks circuit validity and experiments. Both review the method, results and limits and prepare for the joint Q&A.

## 15 November 2026 — supplied technical review by Jincan Li

Evidence: the user supplied two page images in this conversation of a document
titled "Technical Review on Project Stage 1", bearing Jincan Li's name and the
date 15 November 2026. The original PDF/source file was not supplied in this
turn; this entry records the visible review rather than recreating an original.

- Reviewer credited by the supplied document: Jincan Li.
- Activity: technical review of Stage 1 source, documentation and retained
  experimental evidence. The review lists the project README, method document,
  synthesizer source and regression tests as consulted evidence.
- Findings: identifies the area/delay versus synthesis-runtime tradeoff; calls
  for component-wise ablations; notes the mandatory first candidate's lack of
  a search deadline, the unused Stage 1 RAM limit, exponential worst-case
  behavior and machine-dependent deadline truncation.
- Recommendations: controlled component ablations, stronger resource handling
  and a documented fallback, full comparison against the adopted Stage 1 method
  after additional cases are released, and accurate contribution/AI records.
- Scope: the supplied review does not claim that Jincan reran experiments,
  implemented an optimization, independently reproduced the results, approved
  the submission, or submitted anything. Those activities are not attributed.

Codex checked the numerical claims against `results/baseline.csv` and
`results/stage1-final-20261110/official.csv`: target successes 12/25 versus
25/25; area sums 1,103,455 versus 622,816 (43.56% lower); median runtimes
4.628 versus 603.945 ms. These are retained runs, not new measurements.
The frozen Stage 1 source confirms the resource observations. The separate
Stage 2 prototype now has current-RSS search admission; this does not invalidate
a criticism explicitly scoped to Stage 1. See
[the Stage 2 resource findings](STAGE2_MAPPED_SEARCH_20261113.md) for its limits.

Follow-up status: component ablations remain open; stronger Stage 2 resource
handling is partial and the official 64 MiB stress failure remains unresolved.
This is evidence of a concrete teammate review, not closure of all teamwork,
reproduction or joint Q&A requirements. No review meeting or agreed task owner
for the recommendations is inferred from the screenshots.

## 15 November 2026 — Zeyun DU's written explanation and review responses

The user supplied a written explanation of cofactors, Shannon expansion, BDD
sharing, legal-gate mapping and shared outputs. He responded to Jincan's review,
acknowledging synthesis-time costs, interactions between optimizations, the need
for controlled ablations and the first-candidate resource limitation. He also
acknowledged the unresolved Stage 2 64 MiB stress failure. This records supplied
written understanding and responses, not an inferred meeting or experiment.
See [the submission review record](STAGE1_TEAM_REVIEW_20261115.md). The user then
requested completion of the Stage 1 submission package; no Canvas upload was
requested in that instruction.

## 20 November — Jincan Li's Stage 2 materials update

Jincan Li committed the revised handout and 25 released Stage 2 inputs to
the research repository. On 21 November, Zeyun DU shared the repository
for comparison. Codex matched all 26 Git blobs to the local course files.
The archive also retains the group's separate Round 5.1 research source
and experiment records. The official Stage 1 comparator is the source
uploaded to Canvas on 17 November.
[Materials commit](https://github.com/Jincan-LI-HUB/Truth-Table-to-Circuit-Discrete-Math-Project/commit/866add1c0a94d863870ccdf00791f4617f8818b3)
