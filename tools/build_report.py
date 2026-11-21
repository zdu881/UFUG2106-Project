#!/usr/bin/env python3
"""Build the technical report from retained measurements, not hand-entered scores."""
import argparse
import csv
import json
import html
import statistics
from report_theme import INK, ACCENT, TINT, PANEL, CAPTION, BORDER
from pathlib import Path

import markdown
from weasyprint import HTML
from pypdf import PdfReader

R = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--run', type=Path, default=R / 'results/final')
parser.add_argument('--baseline', type=Path, default=R / 'results/baseline.csv')
parser.add_argument('--regression', type=Path, default=R / 'results/final/regression.json')
parser.add_argument('--sanitizers', type=Path, default=R / 'results/final/sanitizers.json')
parser.add_argument('--stress', type=Path, default=R / 'results/final/stress_independent.json')
parser.add_argument('--output', type=Path, default=R / 'reports/UFUG2106_Stage1_Technical_Report.pdf')
parser.add_argument('--date-label', default='10 NOVEMBER 2026')
parser.add_argument('--study', type=Path, help='Verified two-repeat Stage 1 study summary JSON')
parser.add_argument('--margin-dir', type=Path, help='Directory of six florilegium margin SVGs')
args = parser.parse_args()

O = args.output.parent
O.mkdir(parents=True, exist_ok=True)
run = args.run.resolve()
optimized_path = run / 'official.csv'
if not optimized_path.exists():
    optimized_path = run / 'optimized.csv'
with args.baseline.open(newline='') as f:
    b = list(csv.DictReader(f))
with optimized_path.open(newline='') as f:
    frows = list(csv.DictReader(f))
ind = json.loads((run / 'independent.json').read_text())
reg = json.loads(args.regression.read_text())
san = json.loads(args.sanitizers.read_text())
stress = json.loads(args.stress.read_text())['cases'][0]
assert len(ind['cases']) == 25 and all(x['valid'] for x in ind['cases'])
assert reg['all_passed'] and san['all_passed']

summary_path = run / 'summary.json'
if summary_path.exists():
    s = json.loads(summary_path.read_text())
else:
    areas = [int(y['area']) / int(x['area']) for x, y in zip(b, frows)]
    times = [float(y['runtime_ms']) for y in frows]
    s = {
        'baseline_pass': sum(x['status'] == 'PASS' for x in b),
        'optimized_pass': sum(x['status'] == 'PASS' for x in frows),
        'baseline_target_met': sum(x['target_met'] == 'yes' for x in b),
        'optimized_target_met': sum(x['target_met'] == 'yes' for x in frows),
        'baseline_area_sum': sum(int(x['area']) for x in b),
        'optimized_area_sum': sum(int(x['area']) for x in frows),
        'area_lower_cases': sum(float(y['area']) < float(x['area']) for x, y in zip(b, frows)),
        'area_ratio_median': statistics.median(areas),
        'optimized_runtime_median_ms': statistics.median(times),
        'optimized_runtime_max_ms': max(times),
        'baseline_runtime_median_ms': statistics.median(float(x['runtime_ms']) for x in b),
        'total_assignments': sum(x['assignments'] for x in ind['cases']),
    }
    s['aggregate_area_reduction'] = 1 - s['optimized_area_sum'] / s['baseline_area_sum']

def md(t):
    return markdown.markdown(t, extensions=['tables', 'fenced_code'])

profiles = []
for name, path in [('Baseline', R / 'results/baseline.csv'), ('250 ms search cap', R / 'results/cap250.csv'),
                   ('500 ms search cap', R / 'results/cap500.csv'),
                   ('1500 ms cap (retained)', optimized_path)]:
    with path.open(newline='') as f:
        rows = list(csv.DictReader(f))
    profiles.append([name, sum(x['status'] == 'PASS' for x in rows),
                     sum(x['target_met'] == 'yes' for x in rows),
                     sum(int(x['area']) for x in rows),
                     sum(float(x['runtime_ms']) for x in rows)])

from report_figures import figures
fig = figures(O / (args.output.stem + "_figures"), profiles)

pages = []
pages.append(md(f'''<div class="eyebrow">UFUG2106 · TRUTH TABLE TO CIRCUIT · {args.date_label}</div>
# Stage 1: Circuit synthesis
**Technical report · Group 2**<br>
Team: Zeyun DU (50025854) and Jincan Li (50032637).

<div class="note">AI-assisted development. AI use has been confirmed for this project; this report records actual AI-assisted work and measurements. Students remain responsible for understanding and presenting the work.</div>

## Main result
The optimized program passes **25/25** published Stage 1 cases and meets **25/25** circuit-delay targets. The natural-order baseline also passes all cases but meets **{s['baseline_target_met']}/25** targets. An independent checker confirms every output on all **{s['total_assignments']:,} input assignments**, and reproduces the official area and delay values.

| Measure | Baseline | Optimized |
|---|---:|---:|
| Correct cases | 25 / 25 | 25 / 25 |
| Delay targets met | {s['baseline_target_met']} / 25 | 25 / 25 |
| Sum of reported case areas | {s['baseline_area_sum']:,} | {s['optimized_area_sum']:,} |
| Median synthesis runtime | {s['baseline_runtime_median_ms']:.3f} ms | {s['optimized_runtime_median_ms']:.3f} ms |

Total area falls by **{s['aggregate_area_reduction']:.1%}**, and **{s['area_lower_cases']}/25** cases have smaller circuits. The median per-case area ratio is **{s['area_ratio_median']:.3f}**. Synthesis takes longer because the optimized program searches representations and mappings; all official runtime constraints are met.

## Problem and contract
For n inputs and m outputs, construct a circuit equivalent to all 2<sup>n</sup> truth-table rows using AND2, OR2 and NOT. Input x0 is the most significant bit. Signal 0 is constant zero; IDs 1…n are inputs. Gates have consecutive IDs and refer only to smaller IDs. The final line lists exactly m output signals.

Area is the sum of emitted gate costs; delay is the longest weighted path to an output. A delay-target violation affects quality but is not itself a correctness failure. Runtime and memory limits, when positive, are validity requirements. [Project handout, pp.1–3]
''') + fig['pipeline'])
pages.append(md('''# Method and correctness
Recursively split the complete truth table on a selected variable. The two cofactors describe the remaining function when that variable is 0 or 1. Equal children remove a decision; identical (variable, low, high) triples share a BDD node.
''') + fig['shannon'] + md('''
## From representation to an optimized circuit
- **Complement sharing:** an edge bit encodes negation, allowing a function and its complement to share a node.
- **Order search:** try natural, reverse, influence-based and seeded local/random orders. Influence samples only propose orders; construction uses every truth-table row.
- **Cost-aware mapping:** compare Shannon, product-of-sums and De Morgan forms using gate costs and signal arrival times. Verified cofactor implications simplify multiplexers; affine subfunctions map to balanced XOR trees using legal gates.
- **Depth reduction:** two-level expansion creates up to four path-conditioned terms. Balance single-fanout AND/OR cones by pairing early-arriving inputs; retain boundaries of shared cones.
- **Final accounting:** share identical gates, prune gates unreachable from outputs and renumber consecutively. Compare completed circuits by delay excess, then area, then delay.

## Correctness and resource boundaries
Shannon selection is exact for both values of x. Induction from the table terminals therefore establishes functional equivalence. Complement normalization negates both children and the selected result. Two-level expansion partitions assignments into exhaustive path conditions. Implication rewrites and affine recognition check their Boolean preconditions; sharing, associativity, idempotence and dead-gate removal preserve outputs.

A first valid candidate is completed before optional search. Later construction/mapping uses cooperative time checks with a budget of 60% of the declared runtime allowance, capped at 1500 ms (1500 ms when no limit is supplied). The mandatory first construction is not hard-bounded; Stage 1 does not actively schedule RAM.

A fixed-order construction visits O(m·2<sup>n</sup>) table positions; BDD size can remain exponential. The method is heuristic, not a proof of minimum area or delay. Figure 3 illustrates sharing and final accounting.
'''))


def result_table(start, end):
    out = ('<table class="results"><tr><th>Case</th><th>Target</th><th>Base area</th><th>Opt area</th>'
           '<th>Base delay</th><th>Opt delay</th><th>Base ms</th><th>Opt ms</th></tr>')
    for j in range(start, end):
        x, y = b[j], frows[j]
        v = ind['cases'][j]
        assert x['case'] == y['case'] == v['case']
        vals = [x['case'].replace('case_', '').replace('.txt', ''), v['target'], x['area'], y['area'],
                x['delay'], y['delay'], f"{float(x['runtime_ms']):.2f}", f"{float(y['runtime_ms']):.2f}"]
        out += '<tr>' + ''.join(f'<td>{html.escape(str(t))}</td>' for t in vals) + '</tr>'
    return out + '</table>'

pages.append(md('''# Published results / 001–013
Every case below is **PASS** for both methods. Every optimized delay is at or below its target. Area and delay use the case-specific units; runtime is wall-clock program time measured by the official evaluator, excluding its correctness checks.
''') + result_table(0, 13) + md('''## Reading the tradeoff
The selection rule first removes delay violations, then reduces area. It can choose a larger circuit when this is needed to meet the target. Individual case results, rather than only a total, show that tradeoff.

The area sum is a descriptive aggregate, not an official project score. Gate costs differ across cases, and no numerical grading formula or excellence threshold has been published in the supplied handout.

## Measurement environment
Ubuntu 22.04.5 LTS, x86-64; GCC 11.4.0, `-O2 -std=c++17`. The course target is Ubuntu 22.04.3 with an AMD EPYC 7542 CPU. Measurements here are local, not claimed to be timings from that target server.
'''))
pages.append(md('''# Published results / 014–025
The submission tables use the retained 10 November optimized run and the historical baseline CSV, not a new packaging-time benchmark. All values come from those evaluator CSVs. The independent checker recalculates area and delay from the actual emitted files.
''') + result_table(13, 25) + md(f'''## Timing and reproducibility
The optimized maximum reported runtime is **{s['optimized_runtime_max_ms']:.3f} ms**. All 25 runs satisfy their case runtime limits, including the 100 ms cases. A fixed random seed makes the search proposal sequence reproducible, but a wall-clock deadline can change how many proposals finish on another machine. Reproductions must retain their own emitted circuits and actual measurements.

The baseline is preserved as a separate executable and source file. Both programs are single-threaded C++17 and require no external synthesis package or network access. The source-only archive contains code, build instructions, checking scripts and retained numerical evidence; use the instructor's cases and evaluator for official reruns.
''') + fig['sharing'])
pages.append(md(f'''# Review through experiments
## Does a shorter search suffice?
{fig['tradeoff']}
The 250 ms and 500 ms variants both miss case 024's delay target. The retained 1500 ms cap reaches all targets and produces smaller total area, at higher synthesis cost. This comparison supports the retained engineering choice; an unpublished runtime-weighted score could rank tradeoffs differently.

## Independent correctness evidence
- **Published inputs:** all {s['total_assignments']:,} assignments, every output. Independent results agree with official area and delay for both methods.
- **{reg['generated_cases']} generated cases:** all Boolean functions on 1, 2 and 3 inputs under three gate-cost settings; 80 seeded random multi-output cases; structured cases on 4, 8 and 12 inputs. All pass full-table checking.
- **Checker self-tests:** six corrupt outputs are rejected, covering wrong behavior, nonconsecutive IDs, forward/self references, an illegal gate, output-count mismatch and invalid arity.
- **Sanitizers:** the generated regression suite also passes AddressSanitizer and UndefinedBehaviorSanitizer with leak detection enabled.
- **Largest input dimension:** a generated 20-input, 8-output case passes the official evaluator under a 10,000 ms runtime limit and 128 MiB memory cap. All 1,048,576 rows independently match; area {stress['area']}, delay {stress['delay']} ≤ {stress['target']}.

## Known limits
The tests do not prove optimality or predict hidden-case grades. BDD ordering can be costly for unstructured functions. The first construction and final output writing are not guaranteed to fit arbitrarily tiny supplied limits. The current synthesizer does not actively schedule against a positive RAM limit; the explicit memory-cap test adds evidence for one large structured instance, not a universal bound.
'''))
if args.study:
    study = json.loads(args.study.read_text())
    variants = study['variants']
    def span(name, key, digits=0):
        values=[row[key] for row in variants[name]]
        lo,hi=min(values),max(values)
        fmt=lambda value: f'{value:,.{digits}f}'
        return fmt(lo) if lo==hi else fmt(lo)+'–'+fmt(hi)
    def study_table(names):
        lines=['| Configuration | Area range | Median ms | Targets |',
               '|---|---:|---:|---:|']
        labels={'control':'Full frozen Stage 1','no_order_search':'No order search',
                'no_balance':'No balancing','no_affine_mapping':'No affine mapping',
                'cap_250':'250 ms cap','cap_500':'500 ms cap','cap_750':'750 ms cap'}
        for name in names:
            lines.append(f'| {labels[name]} | {span(name,"area")} | {span(name,"median_ms",3)} | {span(name,"targets")}/25 |')
        return '\n'.join(lines)+'\n\n'
    pages.append(md('''# Component evidence and selection
## Controlled follow-up / 16 November
Seven configurations × two sequential repetitions × 25 cases: **350 official and independent full-table passes**. Configuration order was reversed for the second repetition on the same workstation. Each configuration also passes **92 generated cases**; all six checker corruption classes are rejected. These new measurements are separate from the 10 November tables.

## Which components help under the current policy?
'''+study_table(['control','no_order_search','no_balance','no_affine_mapping'])+'''
One change per ablation: retain only the natural order and its mapping portfolio; bypass net balancing; or bypass affine mapping while retaining detection. Sharing, complemented edges and other rewrites remain enabled.

Relative to the full run, removing order search raises aggregate area by **16.1%**, removing balancing by about **9.8%**, and bypassing affine mapping by **0.42%**. Effects are conditional and non-additive. Wall-clock stopping changes completed candidates; two repetitions cannot establish statistical significance or hidden-case robustness.

## Can a shorter budget preserve all delay targets?
'''+study_table(['cap_250','cap_500','cap_750'])+'''
Only the search ceiling changes. Each cap misses **case 024** in both repetitions; correctness and runtime checks pass. Full Stage 1 reaches 25/25 targets twice. The unbounded first construction means these caps do not bound total runtime.

## Submission decision and remaining uncertainty
Retain full Stage 1: it has the lowest aggregate area tested and meets every public target. **Highest official score is not established**: no-order search also meets all targets and takes roughly 87 ms median, at higher area. An unknown runtime-weighted score could prefer it; case-specific gate costs also limit aggregate interpretation.

This addresses the critique with three ablations and a same-source budget study. Complement-edge ablation, interactions, tight RAM and cross-machine timing remain open. Codex ran these experiments; no student rerun is inferred. Patches, CSVs and paired case records are retained.
'''))

pages.append(md(r'''# Reproduction and project gates
## Commands
Run from the source root, with the teacher's evaluator and Stage 1 inputs in `official/`:

```bash
make
python3 tools/evaluate_release.py --output results/a-new-run
python3 tests/regression.py ./build/synth results/regression.json
```

The run directory must be new. Python verification uses NumPy; the synthesizer itself only needs the C++ standard library. `docs/METHOD.md` explains the transformations, and raw CSV/JSON logs retain the numerical evidence.

## Completion review
| Gate | Evidence / status |
|---|---|
| Stage 1 format, source and compile instructions | Standalone C++ sources and Makefile; README documents the interface |
| Method and all 25 case results | This report and retained official / independent outputs |
| Technical correctness and resource tests | Official, independent, generated, sanitizer and constrained stress checks |
| AI assistance | Confirmed for this project; students remain responsible for the work |
| Student review, 15 November | Jincan Li: supplied technical review; Zeyun DU: written method explanation and responses. No student experiment rerun is claimed. |
| Stage 2 comparison | No additional release found in the 13 November check; compare both methods on all 50 when available |
| Live Q&A | Date TBD; both students must attend |
| Hidden-case grading | Teacher evaluation, unavailable locally |

## Next-stage plan
Compare the actual adopted Stage 1 method with the Stage 2 improvement on all 50 public cases. The current optimized candidate is frozen in releases/stage1_candidate; the simple baseline remains an engineering ablation. The new component study measures three conditional effects; broader interactions and complemented-edge ablation remain open. Resource handling also remains open; the Stage 2 prototype has memory monitoring but an unresolved 64 MiB stress failure. The source ZIP records the supplied student reviews and AI assistance. Prepare to explain correctness and the area/delay/runtime tradeoff in the joint Q&A.

**Sources:** UFUG2106 Final Project, Canvas file 869384, pp.1–4; Course Information, Canvas file 785116. The syllabus assigns 15% of the course to the project; it does not provide an excellence cutoff.
'''))
css = '''figure{margin:12pt 0 8pt;break-inside:avoid}figure img{display:block;width:100%;height:auto}figcaption{font-size:8pt;line-height:1.3;color:#526070;margin-top:5pt}@page{size:A4;margin:17mm 17mm 18mm;@bottom-left{content:"UFUG2106 · STAGE 1 TECHNICAL REPORT";font-size:7.5pt;color:#777}@bottom-right{content:counter(page);font-size:9pt;color:#852d42}}body{font-family:"Liberation Sans",sans-serif;color:#272932;font-size:10.2pt;line-height:1.43}h1{color:#852d42;font-size:24pt;line-height:1.2;margin:0 0 13pt}h2{color:#852d42;font-size:13pt;margin:15pt 0 6pt}p{margin:7pt 0}li{margin:5pt 0}table{width:100%;border-collapse:collapse;font-size:9.3pt;margin:12pt 0}th{background:#f0e7eb;text-align:left}td,th{padding:7pt 5pt;border-bottom:1px solid #ddd}.page:nth-child(4) .results td,.page:nth-child(4) .results th{padding:6pt 4pt}.results{font-size:8.6pt}.results td,.results th{text-align:right;padding:9pt 4pt}.results td:first-child,.results th:first-child{text-align:left}.page{break-before:page}.page:first-child{break-before:auto}.eyebrow{font-size:8.5pt;color:#777;margin-bottom:12pt}.note{padding:9pt;background:#f5f0f2;border-left:3px solid #852d42;font-size:9.2pt}pre{white-space:pre-wrap;background:#f5f5f5;padding:10pt;font-size:9pt}code{font-family:"DejaVu Sans Mono",monospace;font-size:8.7pt}tr{break-inside:avoid}.page:last-child td,.page:last-child th{padding:4pt 5pt}'''
# One palette drives the headings, panels, tables, diagrams and floral ink.
for before, after in {'#852d42':ACCENT, '#272932':INK, '#f0e7eb':TINT,
                      '#f5f0f2':PANEL, '#f5f5f5':PANEL, '#526070':CAPTION,
                      '#777':CAPTION, '#ddd':BORDER}.items():
    css = css.replace(before, after)
if args.study:
    css += '.page:nth-child(6) td,.page:nth-child(6) th{padding:3pt 4pt}'
if args.margin_dir:
    css += '''@page{margin:20mm 12mm 30mm 48mm;@bottom-left{vertical-align:top;padding-top:2mm}@bottom-right{vertical-align:top;padding-top:2mm}}
body{font-size:9.5pt;line-height:1.34}h1{font-size:22pt}h2{font-size:12pt;margin:12pt 0 5pt}
table{font-size:8.7pt}td,th{padding:6pt 4pt}.page:first-child td,.page:first-child th{padding:4pt 4pt}.page:first-child table{margin:10pt 0}.results{font-size:8pt}
.results td,.results th{padding:8pt 3pt}.page:nth-child(4) .results td,.page:nth-child(4) .results th{padding:6pt 3pt}
figure{margin:10pt 0 7pt}figcaption{font-size:7.5pt}'''
    if args.study:
        css += '.page:nth-child(6) td,.page:nth-child(6) th{padding:3pt 4pt}'
    for page in range(1, len(pages)+1):
        artwork_page = (page-1)%6+1
        margin_path = args.margin_dir / f'margin-{artwork_page}.png'
        if not margin_path.exists():
            margin_path = args.margin_dir / f'margin-{artwork_page}.svg'
        margin = margin_path.resolve(strict=True).as_uri()
        css += f'@page:nth({page}){{background-image:url("{margin}");background-size:210mm 297mm;background-position:-48mm -20mm;background-repeat:no-repeat}}'
    pages[-1] += '<p style="font-size:7.5pt;color:'+CAPTION+'">Margin artwork: florilegium by Kevin Raymond (MIT), generated from seed 2106.</p>'
content = ('<html lang="en"><meta charset="utf-8"><style>' + css + '</style><body>' +
           ''.join('<section class="page">' + x + '</section>' for x in pages) + '</body></html>')
target = args.output
target.with_suffix('.html').write_text(content)
HTML(string=content, base_url=str(R)).write_pdf(target)
reader = PdfReader(target)
assert len(reader.pages) == len(pages), (len(reader.pages), len(pages))
print(target, len(reader.pages), 'pages')
