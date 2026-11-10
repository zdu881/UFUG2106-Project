#!/usr/bin/env python3
"""Build the technical report from retained measurements, not hand-entered scores."""
import csv,json,html
from pathlib import Path
import markdown
from weasyprint import HTML
from pypdf import PdfReader
R=Path(__file__).resolve().parents[1];O=R/'reports';O.mkdir(exist_ok=True)
s=json.loads((R/'results/final/summary.json').read_text())
b=list(csv.DictReader((R/'results/baseline.csv').open()));f=list(csv.DictReader((R/'results/final/optimized.csv').open()))
ind=json.loads((R/'results/final/independent.json').read_text());reg=json.loads((R/'results/final/regression.json').read_text());san=json.loads((R/'results/final/sanitizers.json').read_text());stress=json.loads((R/'results/final/stress_independent.json').read_text())['cases'][0]
assert len(ind['cases'])==25 and all(x['valid'] for x in ind['cases'])
assert reg['all_passed'] and san['all_passed']
def md(t):return markdown.markdown(t,extensions=['tables','fenced_code'])
pages=[]
pages.append(md(f'''<div class="eyebrow">UFUG2106 · TRUTH TABLE TO CIRCUIT · 08 NOVEMBER 2026</div>
# Stage 1: Circuit synthesis
**Technical development report · Group 2**  
Prepared for Zeyun DU (50025854). Canvas teammate: Xinchen HUANG.

<div class="note">AI-assisted development draft. Course Information p.16 prohibits GAI completing assessment tasks; no later project-specific exception has been confirmed. This report records actual AI-assisted work and measurements. Submission eligibility remains unresolved.</div>

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
'''))
pages.append(md('''# Method and correctness
## Baseline: shared Shannon decisions
Recursively split the table in input order x0,…,x(n−1). Each decision has a low and high cofactor. Equal children eliminate the decision; identical triples (variable, low, high) share one node. Map a decision to:

**f = (NOT x AND f0) OR (x AND f1).**

Terminals and literal functions map directly. A structural gate table shares repeated operations, and a final reachability traversal removes unused gates and assigns consecutive output IDs.

## Improved representation and mapping
- **Complement sharing:** one edge bit represents logical negation, reducing duplicated subfunctions.
- **Variable-order search:** compare natural, reverse, influence-based and seeded local/random orders. Influence samples propose orders; every candidate still uses the complete truth table.
- **Cost-aware logic:** compare sum-of-products, product-of-sums and De Morgan forms with the supplied gate costs and arrival times. Delay-oriented candidates can materialize both polarities instead of inserting an extra output inverter.
- **Two-level expansion:** replace two sequential decisions with up to four path-conditioned terms and a balanced OR tree. This trades some extra gates for fewer sequential multiplexer stages.
- **Algebraic structure:** verified cofactor implications simplify multiplexers; affine subfunctions become balanced XOR trees built from legal gates.
- **Associative balancing:** flatten single-fanout AND/OR cones and combine early-arriving signals first. Shared cones retain their boundaries.

Keep the completed baseline as a candidate. Rank complete, pruned circuits by delay excess, then area, then delay. Search uses at most 60% of the declared runtime allowance, capped at 1500 ms, with cooperative checks during BDD construction and mapping. A first valid candidate is built before optional search.

## Why the transformations preserve the function
Shannon expansion selects exactly f0 for x=0 and f1 for x=1, so induction from the truth-table terminals proves the baseline. Complement normalization negates both children and the selected result. A four-way cover partitions assignments by two decisions, then ORs the selected cofactors. If f0 implies f1, f0 OR (x AND f1) selects the same value in both cases. Affine recognition checks that cofactors differ by a constant one. Associativity, idempotence, structural sharing and removal of unreachable gates preserve outputs.

The method is heuristic, not a proof of minimum area or delay. Fixed-order construction visits O(m·2<sup>n</sup>) table positions; BDD size can still be exponential. The measured limits and tests are therefore essential evidence.
'''))
def result_table(start,end):
 out='<table class="results"><tr><th>Case</th><th>Target</th><th>Base area</th><th>Opt area</th><th>Base delay</th><th>Opt delay</th><th>Base ms</th><th>Opt ms</th></tr>'
 for j in range(start,end):
  x,y=b[j],f[j];v=ind['cases'][j]
  assert x['case']==y['case']==v['case']
  vals=[x['case'].replace('case_','').replace('.txt',''),v['target'],x['area'],y['area'],x['delay'],y['delay'],f"{float(x['runtime_ms']):.2f}",f"{float(y['runtime_ms']):.2f}"]
  out+='<tr>'+''.join(f'<td>{html.escape(str(t))}</td>' for t in vals)+'</tr>'
 return out+'</table>'
pages.append(md('''# Published results / 001–013
Every case below is **PASS** for both methods. Every optimized delay is at or below its target. Area and delay use the case-specific units; runtime is wall-clock program time measured by the official evaluator, excluding its correctness checks.
''')+result_table(0,13)+md('''## Reading the tradeoff
The selection rule first removes delay violations, then reduces area. It can choose a larger circuit when this is needed to meet the target. Individual case results, rather than only a total, show that tradeoff.

The area sum is a descriptive aggregate, not an official project score. Gate costs differ across cases, and no numerical grading formula or excellence threshold has been published in the supplied handout.

## Measurement environment
Ubuntu 22.04.5 LTS, x86-64; GCC 11.4.0, `-O2 -std=c++17`. The course target is Ubuntu 22.04.3 with an AMD EPYC 7542 CPU. Measurements here are local, not claimed to be timings from that target server.
'''))
pages.append(md('''# Published results / 014–025
All values come from `results/baseline.csv` and `results/final/optimized.csv`. The independent checker recalculates area and delay from the actual emitted files.
''')+result_table(13,25)+md(f'''## Timing and reproducibility
The optimized maximum reported runtime is **{s['optimized_runtime_max_ms']:.3f} ms**. All 25 runs satisfy their case runtime limits, including the 100 ms cases. A fixed random seed makes the search proposal sequence reproducible, but a wall-clock deadline can change how many proposals finish on another machine. Reproductions must retain their own emitted circuits and actual measurements.

The baseline is preserved as a separate executable and source file. Both programs are single-threaded C++17 and require no external synthesis package or network access. The source-only archive contains code, build instructions, checking scripts and retained numerical evidence; use the instructor's cases and evaluator for official reruns.
'''))
profiles=[]
for name,path in [('Baseline','results/baseline.csv'),('250 ms search cap','results/cap250.csv'),('500 ms search cap','results/cap500.csv'),('1500 ms cap (retained)','results/final/optimized.csv')]:
 rows=list(csv.DictReader((R/path).open()));profiles.append([name,sum(x['status']=='PASS' for x in rows),sum(x['target_met']=='yes' for x in rows),sum(int(x['area']) for x in rows),sum(float(x['runtime_ms']) for x in rows)])
table='| Configuration | Pass | Target met | Area sum | Total runtime |\n|---|---:|---:|---:|---:|\n'+''.join(f'| {a} | {b}/25 | {c}/25 | {d:,} | {e/1000:.2f} s |\n' for a,b,c,d,e in profiles)
pages.append(md(f'''# Review through experiments
## Does a shorter search suffice?
{table}
The 250 ms and 500 ms variants both miss case 024's delay target. The retained 1500 ms cap reaches all targets and produces smaller total area, at higher synthesis cost. This comparison supports the retained engineering choice; an unpublished runtime-weighted score could rank tradeoffs differently.

## Independent correctness evidence
- **Published inputs:** all {s['total_assignments']:,} assignments, every output. Independent results agree with official area and delay for both methods.
- **{reg['generated_cases']} generated cases:** all Boolean functions on 1, 2 and 3 inputs under three gate-cost settings; 80 seeded random multi-output cases; structured cases on 4, 8 and 12 inputs. All pass full-table checking.
- **Checker self-tests:** six corrupt outputs are rejected, covering wrong behavior, nonconsecutive IDs, forward/self references, an illegal gate, output-count mismatch and invalid arity.
- **Sanitizers:** the generated regression suite also passes AddressSanitizer and UndefinedBehaviorSanitizer with leak detection enabled.
- **Largest input dimension:** a generated 20-input, 8-output case passes the official evaluator under a 10,000 ms runtime limit and 128 MiB memory cap. All 1,048,576 rows independently match; area {stress['area']}, delay {stress['delay']} ≤ {stress['target']}.

## Known limits
The tests do not prove optimality or predict hidden-case grades. BDD ordering can be costly for unstructured functions. The first construction and final output writing are not guaranteed to fit arbitrarily tiny supplied limits. All released cases omit a positive RAM limit; the explicit memory-cap test adds evidence for one large structured instance, not a universal bound.
'''))
pages.append(md(r'''# Reproduction and project gates
## Commands
Run from the source root, with the teacher's evaluator and Stage 1 inputs in `official/`:

```bash
make
./official/evaluator ./build/synth ./official/stage1 \
  --report results/reproduction.csv \
  --save-outputs results/reproduction_outputs
python3 tools/verify_all.py official/stage1 \
  results/reproduction_outputs results/reproduction.csv \
  results/reproduction_independent.json
python3 tests/regression.py ./build/synth
```

The saved-output directory must be new or empty. Python verification uses NumPy; the synthesizer itself only needs the C++ standard library. `docs/METHOD.md` explains the transformations, `SHA256SUMS.json` identifies source-archive contents, and raw CSV/JSON logs retain the numerical evidence.

## Completion review
| Gate | Evidence / status |
|---|---|
| Stage 1 format, source and compile instructions | Standalone C++ sources and Makefile; README documents the interface |
| Method and all 25 case results | This report and retained official / independent outputs |
| Technical correctness and resource tests | Official, independent, generated, sanitizer and constrained stress checks |
| AI-use eligibility | Pending clarification against Course Information p.16 |
| Actual student teamwork | Canvas team known; contributions and division of work must reflect real activities |
| Stage 2 comparison | Additional 25 cases not yet released; both methods must be compared on all 50 |
| Live Q&A | Date TBD; both students must attend |
| Hidden-case grading | Teacher evaluation, unavailable locally |

## Next-stage plan
Compare the actual adopted Stage 1 method with the Stage 2 improvement on all 50 public cases. The current optimized candidate is frozen in releases/stage1_candidate; the simple baseline remains an engineering ablation. Retain real teamwork records and prepare to explain the method, correctness, delay calculation and area/runtime tradeoff in the joint Q&A.

**Sources:** UFUG2106 Final Project, Canvas file 869384, pp.1–4; Course Information, Canvas file 785116, pp.12 and 16. The syllabus assigns 15% of the course to the project; it does not provide an excellence cutoff.
'''))
css='''@page{size:A4;margin:17mm 17mm 18mm;@bottom-left{content:"UFUG2106 · STAGE 1 / AI-ASSISTED TECHNICAL DRAFT";font-size:7.5pt;color:#777}@bottom-right{content:counter(page);font-size:9pt;color:#852d42}}body{font-family:"Liberation Sans",sans-serif;color:#272932;font-size:10.2pt;line-height:1.43}h1{color:#852d42;font-size:24pt;line-height:1.2;margin:0 0 13pt}h2{color:#852d42;font-size:13pt;margin:15pt 0 6pt}p{margin:7pt 0}li{margin:5pt 0}table{width:100%;border-collapse:collapse;font-size:9.3pt;margin:12pt 0}th{background:#f0e7eb;text-align:left}td,th{padding:7pt 5pt;border-bottom:1px solid #ddd}.results{font-size:8.6pt}.results td,.results th{text-align:right;padding:9pt 4pt}.results td:first-child,.results th:first-child{text-align:left}.page{break-before:page}.page:first-child{break-before:auto}.eyebrow{font-size:8.5pt;color:#777;margin-bottom:12pt}.note{padding:9pt;background:#f5f0f2;border-left:3px solid #852d42;font-size:9.2pt}pre{white-space:pre-wrap;background:#f5f5f5;padding:10pt;font-size:9pt}code{font-family:"DejaVu Sans Mono",monospace;font-size:8.7pt}tr{break-inside:avoid}.page:last-child td,.page:last-child th{padding:4pt 5pt}'''
content='<html lang="en"><meta charset="utf-8"><style>'+css+'</style><body>'+''.join('<section class="page">'+x+'</section>' for x in pages)+'</body></html>'
target=O/'UFUG2106_Stage1_Technical_Report_AI_Assisted.pdf';target.with_suffix('.html').write_text(content)
HTML(string=content,base_url=str(R)).write_pdf(target)
reader=PdfReader(target);assert len(reader.pages)==6,len(reader.pages)
print(target,len(reader.pages),'pages')
