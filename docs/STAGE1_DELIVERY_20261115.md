# Stage 1 delivery — resubmitted 17 November 2026

Canvas confirms **attempt 2 submitted** on 17 November at 16:57:58 Beijing time,
with both the source ZIP and unchanged seven-page report. Both uploaded files
were downloaded and matched against the local copies.
See the [submission receipt](../artifacts/stage1-resubmission-20261117/submission_receipt.json).
The preparation records below describe the earlier deliveries.

Submitted files:

- [Source ZIP](../artifacts/stage1-resubmission-20261117/UFUG2106_Group2_Stage1_Source.zip)
- [Seven-page PDF report](../artifacts/stage1-resubmission-20261117/UFUG2106_Group2_Stage1_Report.pdf)

The current source package contains 52 files (including the new study evidence): frozen Stage 1 source, optional
engineering baseline, concise build README, a Stage 1 Makefile, independent
verification tools/tests, retained numerical evidence and truthful review/AI
provenance. It excludes Stage 2 implementation, instructor materials, binaries
and raw outputs. `make` builds `build/synth`; `make baseline` is optional.

The report includes all 25 case results, method/correctness, optimization
tradeoffs, known resource limits and the actual supplied student reviews.
Performance tables retain the 10 November measurement (area 622,816, runtime
median 603.945 ms); the new reproduction below is separate evidence.

## Completed checks

- ZIP integrity and file inventory; source equals the frozen Stage 1 candidate.
- Fresh extraction and `make`; final rebuilt executable is byte-identical to the
  executable used by the fresh snapshot evaluator.
- Official evaluator: 25/25 PASS, 25/25 delay targets.
- Independent verifier: every truth-table output checked, all 25 cases valid.
- Fresh reproduction area sum: 622,257; median runtime: 612.066 ms.
- Final extracted executable: 92 generated cases PASS, 6 checker corruption
  classes rejected. Packaged runner/comparison tests: 11 PASS.
- PDF: six pages, all result rows generated from retained CSVs; rendered pages
  inspected for clipping/overlap. Package Markdown links checked.

Evidence: `results/stage1-delivery-check-20261115/delivery_check.json`,
`evaluation/`, `regression.json` and `report-render/` below that directory.
The final ZIP adds a frozen-source copy and provenance document to the first
package extracted for official evaluation; every existing file is unchanged.
No historical measurement, original archive or frozen source was overwritten.
Sanitizer and structured-stress records in the report remain historical evidence;
those experiments were not rerun during this packaging task.

The user's explanation/responses and the supplied review bearing Jincan Li's
name are recorded in [the team review](STAGE1_TEAM_REVIEW_20261115.md).
No meeting, student experiment execution or independent student implementation
is inferred. Broader component interactions and additional resource work remain follow-up
items; Stage 2/Q&A are not declared complete.

## Rebuild the source ZIP

```bash
python3 tools/package_stage1.py --output artifacts/a-new-stage1-delivery
```

The destination must not exist. Rebuild the companion report separately with
`tools/build_report.py --run results/stage1-final-20261110 --date-label "15 NOVEMBER 2026" --output artifacts/a-new-stage1-delivery/UFUG2106_Group2_Stage1_Report.pdf`.

This task prepared the upload files. It did not upload them to Canvas.

## Illustrated report revision — 15 November

The illustrated edition introduced four vector
figures now explain the Stage 1 search pipeline, Shannon decision-to-gate mapping,
shared-output area/delay accounting and the measured search-budget tradeoff.
The PDF remains six pages. Both per-case result tables (all 25 rows) are unchanged;
the tradeoff plot is generated from the retained evaluator CSVs. The synthetic
circuit pictured in Figure 3 passes all eight assignments in the independent
checker, with area 10 and delay 5.

The source ZIP is byte-for-byte identical to the previously verified delivery.
No synthesizer change or repeated benchmark was needed. The previous PDF remains
in `artifacts/stage1-submission-20261115-final/`. Verification for this visual
revision is in `results/stage1-illustrated-report-20261115/verification.json`.
Editable SVG figures are alongside the new PDF in its `_figures/` directory;
`tools/report_figures.py` and `tools/build_report.py` regenerate them. Report
rebuilding additionally requires Matplotlib for the measured tradeoff plot.
Only the PDF and source ZIP are intended for Canvas upload.

## Florilegium report styling — 15 November

The current delivery adds procedural botanical margins generated with
[florilegium](https://github.com/kevinraymond/florilegium), pinned to commit
`678a7505162c83b79edaf76a69fadea8d48e4812`. Its MIT license and generation
parameters are retained in the delivery's `margins/` directory. The report,
four technical figures and botanical artwork share `tools/report_theme.py`:
deep blue `#234B80`, secondary blue `#547DA6`, and pale panels `#F3F7FB`.
The initial edition clipped decoration to the leftmost 14 mm; this cut some
petals and was corrected in the follow-up below. Body margins remain 17 mm.

The PDF remains six pages, the per-case result tables are unchanged, and the
source ZIP is byte-identical to the verified illustrated delivery. No algorithm
or performance measurement changed. Checks and page renders are retained in
`results/stage1-florilegium-report-20261115/`; previous editions remain available.

To reproduce in a new output directory (Python report dependencies plus the
system Lua 5.3 shared library, CairoSVG and Pillow; TeX is not required):

```bash
git clone https://github.com/kevinraymond/florilegium artifacts/florilegium-upstream
git -C artifacts/florilegium-upstream checkout 678a7505162c83b79edaf76a69fadea8d48e4812
python3 tools/florilegium_margins.py --upstream artifacts/florilegium-upstream --output artifacts/new-report/margins
python3 tools/build_report.py --run results/stage1-final-20261110 --date-label "15 NOVEMBER 2026" --margin-dir artifacts/new-report/margins --output artifacts/new-report/UFUG2106_Group2_Stage1_Report.pdf
```

Reuse the existing upstream checkout if already present. Omitting `--margin-dir`
builds the blue report with its original narrower margins and no botanical artwork.

## Botanical clipping correction — 15 November

The current links point to the corrected edition. The generator removes the
outer page safety mask, retains internal petal masks, and omits the separate
horizontal footer ornament. It measures the complete flower branches and fits
all six with one uniform scale into x = 2–14 mm. Petals and the bottom curl are
fully visible, with at least 3 mm clearance from the body. Final SVGs and their
license/provenance are in the corrected delivery's `margins-v2/` directory.

All six rendered pages were inspected. Transparent-image bounds confirm the
decoration stays inside its allotted space; extracted PDF text on all six pages
and source ZIP bytes equal the preceding edition. Verification and renders:
`results/stage1-florilegium-layout-fix-20261115/`.

## Student ID completion — 15 November

The user confirmed Jincan Li's student ID as **50032637**. The current PDF
credits Zeyun DU (50025854) and Jincan Li (50032637). The source ZIP's README
and team review also include this ID. These two documentation files are the
only archive changes from the corrected botanical edition; all code, tests
and retained evidence are byte-identical. The six-page PDF text changes only
by adding the ID, and the complete botanical margins are preserved. Current
artwork and its provenance/license are in the ready delivery's `margins/`.
Checks are recorded in `results/stage1-student-id-update-20261115/`.

## Expanded botanical garden — 15 November

At the user's request, the current report replaces the sparse stem with full
flower-and-leaf gardens: density 1.0, intensity 0.8, generated for a 48 mm
left margin. Growth is constrained to the side column before rendering; each
complete garden is fitted without distortion into x = 3–42 mm. Internal petal
masks remain, while the outer page mask is removed to avoid cutting flowers.
The final SVGs, MIT license and parameters are in `margins-final/`.

The report uses a 48 mm left margin, 12 mm right margin and 9.5 pt body type,
retaining six A4 pages and all four technical figures. PDF background placement
is explicitly offset for these asymmetric margins. All six page renders were
inspected, and the actual PDF's 43–47 mm gap was checked to be clear. Extracted
text matches the previous report after whitespace normalization; both result
tables and source ZIP are unchanged. Verification and renders are in
`results/stage1-garden-report-20261115/`.

## Four-sided botanical frame — 15 November

The user requested an enclosing composition. The current report arranges
complete florilegium gardens along all four edges, rotating and mirroring
slender branches and connecting the corners with a fine rounded border.
The body margins are left 34 mm, right 26 mm, top 25 mm and bottom 26 mm;
footer text sits above the bottom floral band, within the frame. The report remains six pages
with the same text, four technical figures, per-case tables and student IDs.

Editable SVGs, print-resolution 300 dpi PNG backgrounds, placement metadata
and the MIT license are retained in `margins-final/`. PNG backgrounds keep
the PDF practical in size while preserving the SVG originals for editing.
The source ZIP is unchanged. Final page renders and content/artwork checks
are in `results/stage1-frame-report-20261115/`. Earlier editions remain available.

## Connected half-frame — 15 November

The user requested a half-enclosure with a continuous corner. The current
report uses one native florilegium garden per page, retaining its original
root-to-footer branch rather than assembling rotated side strips. The planting
region extends along the bottom, so leaves continue through the lower-left
turn. Top and right edges remain open, with no rectangular connecting border.

Complete gardens are uniformly scaled and positioned against measured painted
bounds, preserving flowers without an outer crop. The left margin is 48 mm;
right 12 mm, top 20 mm and bottom 30 mm. Editable SVGs, 300 dpi PNGs, license
and parameters are in `margins-final/`. Six final page renders and clearance
checks are in `results/stage1-half-frame-report-20261115/`. All report text
(after whitespace normalization) and the source ZIP are unchanged.

## Worked algorithm diagram — 16 November

The current report replaces Figure 2 with a complete synthetic example:
f = (a OR b) AND c, shown as all eight truth-table rows, a seven-node
decision tree, a three-node reduced BDD, and a two-gate circuit. Matching
c subtrees share a blue highlight; dashed/solid edges distinguish 0/1.
Counts exclude terminals and the caption distinguishes BDD size from mapped
area/delay. This figure is editable SVG, not an image-model-generated bitmap.

All eight assignments were checked against the plotted tree and BDD data;
the independent circuit checker also verified the depicted OR2/AND2 netlist.
The report still has six pages, with the half-frame decoration retained.
Other page text is unchanged except the cover date; all per-case tables and
the source ZIP are unchanged. Evidence and a large diagram preview are in
`results/stage1-diagram-report-20261116/`. Editable figures and inherited
botanical assets are alongside the current PDF.

## Remaining technical figures redesigned — 16 November

The latest report updates Figure 1 with initialization, optional order/BDD and
mapping search, mapped-cost comparison, retention and output. Its ranking and
first-candidate limit note were checked against the frozen Stage 1 source.
Figure 3 now compares duplicate versus shared OR gates: independently verified
on all eight assignments, area 18 versus 10 and delay 5 for both. Figure 4
uses aligned panels for total circuit area, total synthesis time and delay
targets met; all values are generated from retained CSVs, with chart data
retained beside the SVG. Figure 2 remains unchanged.

The report is still six pages after adjusting the first summary table's
spacing. All prose and tables outside the figures and the source ZIP are
unchanged. Editable SVGs and the half-frame assets are included alongside
the PDF. Checks, figure previews and a six-page contact sheet are retained in
`results/stage1-all-diagrams-20261116/`. No new benchmark or submission occurred.

## Evidence strengthened for submission — 16 November

The current PDF has seven pages: the prior six-page material plus a component
and budget study. The official handout specifies report content, not a six-page
limit. The frozen production method remains unchanged. See
[the study](STAGE1_STUDY_20261116.md) for scope, results and limitations.

Seven configurations were each evaluated twice, in sequential reversed order:
350/350 official and independent case checks pass. All seven pass 92 generated
cases each and reject six checker corruption classes. Full control reaches
25/25 targets with area 622,257 in both repetitions; each shorter cap misses
case 024. No-order search is much faster but has larger area, so no official
score superiority is asserted. The primary per-case tables retain 10 November
measurements; this new evidence is separately dated.

The 52-entry ZIP adds experiment tools, 14 CSVs, seven regression records,
summary/paired data and study documentation. Its production source and
previous retained evidence are unchanged. A fresh extraction builds and the
small shared-output example passes independent checking. The executable matches
the newly fully evaluated control. The new report page and final page were
rendered and inspected; all seven final pages were rendered. Verification is
in `results/stage1-evidence-delivery-20261116/`. No Canvas upload occurred.

Rebuild the illustrated study report with:

```bash
python3 tools/build_report.py --run results/stage1-final-20261110 --date-label "16 NOVEMBER 2026" --study results/stage1-study-summary-20261116/summary.json --margin-dir artifacts/stage1-submission-20261116-illustrated/margins --output artifacts/a-new-report/UFUG2106_Group2_Stage1_Report.pdf
```

`tools/package_stage1.py` now includes the study summary by default. Its output
directory must still be new.
