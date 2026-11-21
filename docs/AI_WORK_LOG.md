# AI-assisted work log

2026-11-08: The user requested completion and iterative review of UFUG2106 Project. Codex read the official project PDF, syllabus and Stage 1 inputs; copied official files; invoked the evaluator help interface. No student solution or submission was present when this workspace was created.

AI assistance is confirmed for this project. Artifacts generated in this workspace are transparently AI-assisted; students remain responsible for understanding, reviewing and presenting the work accurately.

Project team (collaborator updated by Zeyun DU): Zeyun DU and Jincan Li, GROUP 2. At workspace creation, contributions by Jincan Li had not been established. A technical review bearing his name was supplied on 15 November; see [the teamwork record](TEAMWORK_LOG.md). This file does not assign unreported activities retrospectively.

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

## 13 November 2026 experimental search follow-up

Codex reproduced the frozen Stage 1 source and previous Stage 2 source in fresh,
sequential snapshot runs. It enabled the previously zero-budget direct-mapping
portfolio with 25% of remaining search time and added `make stage2`. The modified
prototype passed all 25 official cases, independent full-table checks and delay
targets, plus 92 generated cases and 6 checker corruption checks. All 11 tool
unit tests pass. Area sum was 616,606, versus 622,257 for the frozen Stage 1 rerun
and 617,943 for the previous prototype. Median runtime rose from 464.071 ms for
the previous prototype to 588.341 ms; one case regressed in area. See
`docs/PROGRESS_20261113.md` for the full tradeoff and evidence paths.
No frozen Stage 1 source, historical results or Stage 1 report was replaced.
No live course status check, submission, teammate communication or student
review occurred in this session. These were AI-performed changes and checks.

## 13 November mapped-order and memory follow-up

Codex checked Canvas assignments, visible files, modules, announcements and linked
Stage 1 attachment metadata using read-only requests; no additional case release
was found. It instrumented the case 015 regression and replaced the pre-screen
portfolio with bounded mapping probes and measured area/delay refinement. Three
repetitions recovered case 015 to area 32,193 / delay 78. It added current-RSS
search admission and four integration tests, including a large-launcher regression.
Final verification passed all 25 official cases and delay targets, independent
full tables, 92 generated cases, six checker corruption classes and 15 tests.
The final official-set area is 615,861 and median runtime 604.336 ms. Synthetic
64 MiB official evaluation still reports MEMORY_LIMIT; direct execution of four
variants under a 32 MiB address-space cap passes. Both passing and failing
measurements are retained. See `docs/STAGE2_MAPPED_SEARCH_20261113.md` for the
scope and remaining limitations. No frozen Stage 1 edit, submission or student
contribution is claimed.

## 15 November 2026 — recording the supplied teammate review

The user supplied two page images of a Stage 1 technical review bearing Jincan
Li's name. Codex compared its area, target-delay and runtime figures with the
retained CSVs and checked its resource observations against the frozen Stage 1
source. It recorded the supplied review and its recommendations in TEAMWORK_LOG.md,
without attributing experiment execution or implementation to the reviewer.
Only documentation changed; no experiment rerun, submission or teammate message
was performed. The original review's English wording was assessed in the reply,
but its original file was not available or edited.

## 15 November 2026 — final Stage 1 package

At the user's explicit request, Codex prepared a focused Stage 1 source ZIP and
updated six-page PDF with the supplied student review contributions. The frozen
synthesizer was unchanged. It verified source identity, clean extraction/build,
25 official and independent full-table checks with all delay targets met,
92 generated regression cases, six checker corruption classes and 11 packaged
tool tests. Fresh reproduction area was 622,257 and runtime median 612.066 ms;
the report retains its explicitly identified 10 November measurements. The
final archive adds only the frozen-source copy and AI provenance to the initially
evaluated package; all pre-existing files and final rebuilt executable were
compared byte-for-byte. PDF pages were rendered and inspected. Final paths and
verification evidence are recorded in STAGE1_DELIVERY_20261115.md. No Canvas
upload was performed, and historical results/packages were retained.

## 15 November — illustrated report revision

At the user's request for more diagrams, Codex added four vector figures:
synthesis pipeline, Shannon gate mapping, a shared-output example, and a
Matplotlib plot of retained budget/area/runtime data. It shortened the method
prose and adjusted result-table spacing to retain six pages. Final renders were
inspected, all 25 result-table rows compared against the prior report, plot data
checked against the retained CSVs, and the depicted example independently
verified on all eight assignments. The source ZIP is unchanged. The prior report
and packages are retained; no new synthesis performance claim or Canvas upload
was made. Current delivery links are in STAGE1_DELIVERY_20261115.md.

## 15 November — florilegium and unified report colors

At the user's request, Codex used Kevin Raymond's florilegium SVG generator
(commit 678a7505162c83b79edaf76a69fadea8d48e4812, MIT) to add botanical page
margins, and unified report headings, tables, diagrams and artwork with a blue
palette. A Python wrapper invokes the installed Lua 5.3 shared library; artwork
parameters and the upstream license are retained beside the generated SVGs.
The six PDF pages were rendered for visual inspection, both result tables were
compared with the prior illustrated edition, and the source ZIP was copied
unchanged. No algorithm edit, benchmark rerun or Canvas upload was performed.

## 15 November — botanical clipping correction

After the user reported obstructed artwork, Codex removed the page-level masks
that cut petals, omitted the separate horizontal footer flourish, and uniformly
scaled complete flower branches into the side margin. All six pages were
rendered and inspected; transparent artwork bounds, unchanged PDF text and
identical source ZIP bytes were checked. The previous report remains available.
No algorithm change, new benchmark or submission was performed.

## 15 November — confirmed student ID

The user supplied Jincan Li's student ID, 50032637. Codex added it to the
report and team identification records, regenerated the delivery, and verified
that the six-page report text only adds the ID. Only the ZIP README and team
review changed; code and experimental evidence remain byte-identical.

## 15 November — fuller botanical report margins

The user requested larger, denser decoration resembling the upstream gallery.
Codex increased florilegium density to 1.0, restricted generation to the side
column, and fitted full gardens into a 39 mm wide area. It reflowed the report
with a 48 mm left margin, corrected the asymmetric PDF background offset, and
retained six pages. All rendered pages were inspected; the actual rendered gap
was checked for overlap, report text matched after whitespace normalization,
and per-case tables and source ZIP remained unchanged. No benchmark or
submission was performed.

## 15 November — enclosing botanical frame

At the user's request for a surrounding rather than single-column composition,
Codex arranged native florilegium gardens along all four sides, using rotated
and mirrored branches with fine corner connections. It adjusted page margins
and footer placement, retained editable SVGs, and embedded 300 dpi backgrounds
to control PDF size. All six pages were rendered for inspection; content and
source-package comparisons were retained. No algorithm, benchmark or submission
was involved.

## 15 November — continuous half-frame

The user preferred a half-enclosure and stronger corner continuity. Codex
restored florilegium's native root-to-footer connection, extended the footer
planting region, and placed each complete garden as a single left/bottom
composition. It removed the assembled four-sided frame and its rectangular
connector. All six final pages were inspected; artwork bounds, open top/right
edges, text/footer clearance and unchanged report text/source ZIP were checked.
No algorithm, benchmark or submission change was made.

## 16 November — worked Shannon/BDD diagram

At the user's request to implement the proposed diagram improvement, Codex
replaced the abstract Shannon mapping figure with a single worked example
across a truth table, decision tree, reduced BDD and legal gates. The SVG is
programmatically drawn; no image-generation model was used. Diagram graph
data and the independent checker verified all eight input assignments. The
report remains six pages, with retained measurements, source ZIP and all
other page text unchanged except the cover date. No synthesis algorithm
change, benchmark rerun or submission was performed.

## 16 November — remaining technical figures

Codex redesigned Figures 1, 3 and 4 as requested: a search/retention flow, a
verified before/after sharing example, and aligned area/runtime/target panels.
It checked the pipeline against the frozen source, independently verified both
example circuits on eight assignments, and compared every plotted aggregate
with retained evaluator CSVs. Six-page renders were inspected. Figure 2, body
prose, numerical tables and the source archive remain unchanged. All new
figures are code-generated vectors; no image-generation model was used.

## 16 November — Stage 1 score-oriented evidence work

At the user's request to strengthen Stage 1, Codex performed three component
ablations and three search-cap experiments against the byte-identical frozen
control, with two sequential reversed-order repetitions per configuration.
All 350 official/independent case-runs pass; each configuration also passes
92 generated cases and six checker-rejection classes. Full control has the
smallest aggregate area tested; short caps miss case 024 and no-order search
trades larger area for much lower runtime. No scoring-formula superiority is
claimed. The frozen method was retained, and a seventh report page, portable
experiment tools and compact evidence were added to the source delivery.
No student experiments, approval, algorithm promotion or Canvas submission are
inferred. A fresh archive build/example and source identity were checked; the
primary 10 November report results were preserved.

## 17 November — code cleanup

Codex consolidated the Makefile build rules and Stage 2 mapping exception
handling, removed redundant candidate state and unused screening roots, and
flattened an unconditional packaging branch. The frozen Stage 1 source and
report PDFs were preserved. Validation: 15 tests, 25 official and independent
case checks (all delay targets met), 92 generated cases and six checker
rejections pass. Repackaged contents match all 52 existing delivery entries.
Evidence: `results/code-cleanup-20261117/`.

## 17 November — Canvas resubmission

At the user's request, Codex submitted the Stage 1 source ZIP and unchanged
report PDF through the logged-in Canvas Dashboard. Canvas confirmed attempt 2
at 08:57:58 UTC. Both attachments were downloaded and matched byte-for-byte
against the local files. Receipt and submitted copies:
`artifacts/stage1-resubmission-20261117/`.

## 20 November — released-case search improvement

Codex fetched origin (no incoming commits), checked the released Stage 2
archive against its 25 local inputs, and traced the case 002 regression.
It tested early influence-order mappings and retained a probe that tries
weight 1, then weight 16 only if that probe misses the delay target.
All 50 official/independent case checks and delay targets pass; total area is
1,180,657. The candidate also passes 92 generated cases, six checker rejection
classes and 16 unit tests. Rejected experiments and source snapshots remain in
`results/stage2-search-20261120/`; results and reproduction commands are in
`docs/STAGE2_SEARCH_20261120.md`. Frozen Stage 1 source and report PDFs were
preserved. No new submission or student contribution was recorded.

## 20 November — case 006 and the 64 MiB memory verdict

Codex traced case 006, interleaved search orders and cached the last two BDD
levels. It verified the two public banks, 92 generated cases and 16 unit tests.
It traced the evaluator's wait4 memory reading and varied the launcher's
memory while independently verifying both circuits. The 64 MiB synthetic case
still fails official evaluation. [Results and evidence](STAGE2_SEARCH_20261120.md#case-006-follow-up)

## 21 November — teammate repository comparison

Zeyun shared Jincan's research repository. Codex matched commit 866add1's handout and input files to the course release, updated the teamwork paragraph, compiled Round 5.1 from the archived source and evaluated it on both banks. Official and independent results are in `results/jincan-reference-20261121/`. The measured Stage 2 deliverable retains the team's current solver.

## 21 November — integrated solver

Zeyun requested the project merger. Codex adapted the Round 5.1 candidate
generator and graph passes from the team research archive, shared the Stage 2
input and scheduling budget, compared successive implementations, ran both
official banks with independent verification, and packaged the measured solver.
All 92 generated checks and 16 tests passed. The synthetic 64 MiB case still
failed official evaluation. [Source provenance](../src/round51/ORIGIN.md) ·
[Measured results](../results/stage2-merged-release-20261121/method-summary.json).

## 21 November — review fixes

At Zeyun's request, Codex opened [issue 2](https://github.com/Jincan-LI-HUB/Truth-Table-to-Circuit-Discrete-Math-Project/issues/2), fixed memory admission and BDD time allocation, and added a source ZIP target. In the 9,000 KiB wire and inverter tests, peak RSS fell from 13,952 to 4,608 KiB. The relaxed-target case 005 used 93,786 area at delay 74. Both public banks passed official and independent checks. Bank 1 case 024 used 159,633 area in the full run and 124,830 on both isolated reruns. The 64 MiB synthetic case still failed the official memory check. [Public measurements](../reports/stage2-review-fixes-20261121/comparison.json)

## 21 November — packed cofactors and candidate order

At Zeyun's request, Codex traced the case 024 area regression to a late reverse-order candidate. Codex implemented bit-packed truth columns, exact per-output support, support-only BDD construction and early mapped order comparisons. Both public banks passed official and independent verification with total area 1,001,957. On the three original case 024 repeats the new solver returned area 124,475 and delay 70. The 64 MiB stress circuit passed exhaustive verification under a hard process cap. The official evaluator still reported a memory failure. Codex reproduced the parent-dependent peak and added a regression test. [Paired controls](../reports/stage2-packed-20261121/resource-study.json)

## 21 November — output selection and depth recovery

At Zeyun's request, Codex reviewed the mapper, added per-output circuit selection,
ordinary and complemented BDD candidates, shared two-input decoding and a shallow
row/column construction. Codex replaced intermediate text conversions with typed
graphs, handled buffered output errors, tested individual candidates, and prepared
the report and source ZIP. A first generated suite exposed two dense functions
above target and informed the shallow mapper. A new 30-case suite was generated
after freezing the source. All 30 circuits passed exhaustive checks and met
their targets. Public results and the eight-case ablation are recorded in the
[report](../reports/stage2-selection-20261121/UFUG2106_Group2_Stage2_Report.pdf).

## 21 November — variable-order scheduling

Codex traced the area regression on Stage 2 case 011 to the initial mapping
sweep. It tried preserving late low-area output cones and a three-variable lookup
cache before selecting the scheduling change. In three paired 011 runs
the revised order produced 39,886 area and delay 78, versus 52,463 and 62.
The 30-case fresh-seed holdout yielded the same total area for both schedules.
The report generator now checks the independent area, delay, target and assignment
count against the retained measurements. Codex rebuilt the package, repeated both
public banks and the ablation, and assembled the [report](../reports/stage2-schedule-20261121/UFUG2106_Group2_Stage2_Report.pdf).
The final code passed 21 unit tests and all 92 generated regression cases.
The checker rejected six deliberately invalid circuits.

## 21 November — constructing a circuit under a memory limit

Codex reproduced a missing-output failure during the initial circuit construction
on a 19-input dense truth table. It added memory checks on that path, released the
mapper before gate balancing, implemented streaming Shannon expansion with bounded
gate reuse, and ran the packed source through the course evaluator and independent
checker. Three dense inputs switched from MEMORY_LIMIT to PASS. The original
20-input structured control still fails the course evaluator at 64 MiB.
[Paired measurements](../reports/stage2-memory-20261121/memory-controls.json).
The final ZIP also passed the preceding 30-function generated set. All 22 unit tests
and 92 generated regression cases passed. The UBSan build repeated the 92 cases
and the 17-input streaming control.

## 21 November — grader memory accounting

Codex checked the live assignment attachment, reproduced the official result with
the extracted submission source, and built a separate zero-gate example. Paired
launcher tests included a genuine 80 MiB control. An English teaching-team note
and reproducible evidence are in the [diagnostic directory](../reports/stage2-rss-accounting-20261121/README.md).
