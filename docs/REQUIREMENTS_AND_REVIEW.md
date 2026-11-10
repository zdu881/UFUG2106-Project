# UFUG2106 Project: requirements and review ledger

Source: official/869384-UFUG2106_Final_Project.pdf (Canvas file 869384), downloaded 2026-11-08. Course Information (Canvas 785116), pp.12 and 16.

## Scope and acceptance evidence

| Requirement | Evidence needed | Current state |
|---|---|---|
| Truth table to equivalent AND2/OR2/NOT circuit | Official evaluator and independent full-table evaluation | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| n=1..20; multiple outputs; MSB-first input ordering | Generated edge cases, exhaustive small functions, largest inputs | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| Consecutive IDs; only smaller fanins; m output signals | Independent parser validation and mutation checks | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| CPU single-thread; executable takes input/output paths | Source review and clean build/run | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| Runtime and memory limits | Official per-case measurements, stress tests | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| Area, delay, runtime optimization | Baseline comparison, per-case tables, held-out generated inputs | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| Stage 1: 25 cases, source ZIP, build README, PDF method/results | Extract/build/reproduce package | Verified for published Stage 1 inputs; see FINAL_REVIEW.md |
| Stage 2: both methods on 50 published cases, contribution/teamwork record | Actual released data and authentic collaboration records | 25 additional cases not yet released |
| Both students attend live Q&A | Actual attendance after date announced | Future requirement |
| Final hidden evaluation (50 cases) | Teacher results | Not available locally |
| AI-use eligibility | Updated project-specific allowance, if any | Course Information p.16 currently prohibits GAI completing assessment tasks; clarification requested |

The syllabus assigns 15% of the course grade to the project. The project specification says validity and area/delay/runtime affect scores, but provides no numerical scoring formula, reference optimum, or excellence cutoff. We will report technical evidence, not invent a predicted official score.

## Engineering review targets (self-imposed, not official grade boundaries)

- All 25 released cases pass both the official evaluator and an independent checker.
- No hard-coded answers, benchmark-name dispatch, or detection of the official evaluator.
- Validity holds on exhaustive small functions, random multi-output tables, constants, projections, complements, shared outputs, cost variants, and n=20 stress inputs.
- Compare an unchanged baseline with optimization, report tradeoffs per case rather than only averages.
- Report all delay violations and resource failures, and retain reproducible commands and raw logs.
- Submission source builds using standard C++17 on the target Linux environment without network or third-party synthesis tools.
- Preserve real design history and AI involvement. Do not invent teammate contributions or claim independent authorship.

## Immediate work order

1. Establish a standalone baseline and independent evaluator.
2. Run official cases and identify dominant cost/delay bottlenecks.
3. Add general algorithmic optimizations; use regression and held-out tests after each substantive change.
4. Produce traceable results and method documentation, then audit all gates above.
5. Keep final project completion open until Stage 2, Q&A, and submission eligibility are resolved.

## Current review status

See `FINAL_REVIEW.md` for the evidence audit. Stage 1 technical artifacts are built and checked; this does not close the academic-policy, Stage 2, teamwork or live Q&A gates.
