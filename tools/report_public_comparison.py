"""Render the two-bank public comparison from the four verified evaluator runs."""
import argparse
import json
from pathlib import Path
from statistics import median
from compare_stage_results import read_csv

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet
from reportlab.lib.units import mm
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak


def output_choice_figure():
    from reportlab.graphics.shapes import Drawing, Line, Polygon, Rect, String
    d = Drawing(470, 105)
    ink = colors.HexColor('#224657')
    for y, title, detail in [(57, 'Circuit A  |  6 units', 'Direct AND, OR via De Morgan'),
                             (3, 'Circuit B  |  6 units', 'Direct OR, AND via De Morgan')]:
        d.add(Rect(1,y,142,43,fillColor=colors.HexColor('#eef3f4'),strokeColor=ink))
        d.add(String(10,y+26,title,fontName='Helvetica-Bold',fontSize=10,fillColor=ink))
        d.add(String(10,y+10,detail,fontSize=9))
    for y in (78,24):
        d.add(Line(143,y,244,y,strokeColor=ink))
        d.add(Line(244,y,244,51,strokeColor=ink))
    d.add(String(153,83,'Keep AND',fontSize=9))
    d.add(String(153,29,'Keep OR',fontSize=9))
    d.add(Line(244,51,284,51,strokeColor=ink))
    d.add(Polygon([284,51,277,54,277,48],fillColor=ink,strokeColor=ink))
    d.add(Rect(286,28,177,47,fillColor=ink,strokeColor=ink))
    d.add(String(295,56,'Selected circuit  |  3 units',fontName='Helvetica-Bold',fontSize=10,fillColor=colors.white))
    d.add(String(295,39,'AND, OR, shared NOT(0)',fontSize=9,fillColor=colors.white))
    return d


def render(root, output, frozen_root=None, permutation_control=None, merged=False):
    retained = merged and frozen_root is None
    run_root = root / 'evidence' if retained else root
    if retained:
        frozen_root = run_root
    comparison_file = 'vs-submitted.json' if frozen_root and not retained else 'comparison.json'
    comparison = json.loads((root / comparison_file).read_text())
    rows = comparison['cases']
    control_path = root / 'control-comparison.json'
    controls = json.loads(control_path.read_text()) if merged and control_path.exists() else None
    schedule = controls.get('schedule') if controls else None
    styles = getSampleStyleSheet()
    story = []

    def para(text, style='BodyText'):
        story.append(Paragraph(text, styles[style]))

    def table(data):
        t = Table(data, repeatRows=1, hAlign='LEFT')
        t.setStyle(TableStyle([
            ('BACKGROUND', (0, 0), (-1, 0), colors.HexColor('#224657')),
            ('TEXTCOLOR', (0, 0), (-1, 0), colors.white),
            ('ROWBACKGROUNDS', (0, 1), (-1, -1), [colors.white, colors.HexColor('#eef3f4')]),
            ('FONTNAME', (0, 0), (-1, 0), 'Helvetica-Bold'),
            ('FONTSIZE', (0, 0), (-1, -1), 9),
            ('ALIGN', (1, 0), (-1, -1), 'RIGHT'),
        ]))
        story.append(t)

    summary = [['Bank', 'Method', 'PASS', 'Delay met', 'Total area', 'Median ms']]
    for stage in ('stage1', 'stage2'):
        for method, key in [('frozen', 'stage1_method'), ('prototype', 'improved_method')]:
            folder = root / f'{stage}-{method}'
            if frozen_root:
                folder = frozen_root / f'{stage}-frozen' if method == 'frozen' else run_root / (f'bank{stage[-1]}' if merged else f'release{stage[-1]}')
            manifest = json.loads((folder/'manifest.json').read_text())
            independent = json.loads((folder/'independent.json').read_text())
            group = [r for r in rows if r['stage'] == stage]
            assert manifest['status'] == 'PASS' and independent['all_rows_checked']
            assert len(group) == len(independent['cases']) == 25
            assert {r['case'] for r in group} == {r['case'] for r in independent['cases'] if r['valid']}
            official = read_csv(folder/'official.csv')
            if {r['case']: r[key] for r in group} != official:
                raise ValueError(f'{stage} {method} comparison differs from its official CSV')
            for replay in independent['cases']:
                measured = official[replay['case']]
                if replay['assignments'] != 1 << replay['n'] or any(
                    replay[field] != measured[field] for field in ('area', 'delay', 'target_met')
                ):
                    raise ValueError(f"{stage} {method} {replay['case']} independent replay differs")
            metrics = [r[key] for r in group]
            summary.append([stage, method, sum(r['passed'] for r in metrics), sum(r['target_met'] for r in metrics),
                            f"{sum(r['area'] for r in metrics):,}", f"{median(r['runtime_ms'] for r in metrics):.1f}"])
    if merged:
        study_path = root / 'resource-study.json'
        study = json.loads(study_path.read_text()) if study_path.exists() else None
        para('UFUG2106 | Stage 2 method improvement', 'Title')
        para('Group 2 | Zeyun DU (50025854) and Jincan Li (50032637)')
        heading = 'Selecting a circuit for each output' if controls else 'Two searches share one time budget'
        para('Comparing variable orders before refining their mappings' if schedule else heading, 'Heading2')
        para('The submitted Stage 1 method splits a truth table into cofactors, shares repeated functions in a reduced ordered BDD, then maps candidate variable orders into the permitted gates. Variable order and gate polarity both affect the area of the resulting circuit.')
        if controls:
            para('The Stage 2 solver builds a natural-order circuit from packed truth columns. It measures which inputs affect each output and tries shared and output-specific variable orders, with and without complemented edges. A two-input decoder shares selectors across four cofactors. If the first search misses the target, a row/column decomposition offers a shallower path using balanced gate trees.')
            if schedule:
                para('The solver first maps the natural order with one timing weight. It compares other orders before varying that weight on the best measured mappings. It can select individual outputs across circuits, accounting for shared gates. It ranks by delay above target, area and delay. Optional search uses up to 1.8 seconds or 45% of a positive limit, whichever is smaller. A graph pass can finish later.')
            else:
                para('Completed candidates are joined by matching gate type and inputs. The solver can replace one output while retaining the gates used by others. It ranks moves by excess delay, reachable area and delay. The optional search budget is capped at 1.8 seconds and 45% of a positive case limit. Building the initial circuit and completing a graph pass can extend beyond that window.')
        elif study:
            para('Each output occupies a packed bit column. XOR and population counts identify which inputs change that output. Both BDD builders skip independent inputs. The first search compares exact and sampled influence orders on the full table. Round 5.1 tests the reverse of natural order first and refines the winner using measured gate area and delay.')
        else:
            para('The merged program builds its first valid circuit in natural order. Its first search tests an influence-based order, screens other orders by BDD size and refines candidates using reachable gate area and delay. The second search uses the Round 5.1 mapper, which also considers two-level covers for outputs with small support. It tries rebalancing, area recovery within the delay target, algebraic rewriting and polarity changes.')
        if not controls:
            para('Both searches rank circuits by excess delay, area and delay. The nominal budget is capped at three seconds and at 60% of a positive case limit. BDD receives half when Round 5.1 supports the gate costs and target, and the full budget otherwise. Round 5.1 checks available memory before allocating its row buffers. A zero-area, zero-delay first circuit ends the search. The first circuit and some graph passes can extend beyond their scheduled window.')
        para('Released cases', 'Heading2')
        table(summary)
        saved = sum(-r['area_delta'] for r in rows)
        smaller = sum(r['area_delta'] < 0 for r in rows)
        larger = sum(r['area_delta'] > 0 for r in rows)
        para(f'Against the submitted method, total area fell by {saved:,} units across the two banks. {smaller} cases used less area, {larger} used more and {50-smaller-larger} tied. The following pages give the official measurements for each case. Search time is wall-clock time on the local workstation.')
        totals = json.loads((root / 'method-summary.json').read_text())
        table([['Both public banks', 'Targets met', 'Total area', 'Total runtime (s)']] +
              [[name, f"{v['targets']} / 50", f"{v['area']:,}", f"{v['total_ms']/1000:.2f}"]
               for name, v in totals.items()])
        smallest = min(totals, key=lambda name: totals[name]['area'])
        fastest = min(totals, key=lambda name: totals[name]['total_ms'])
        if study:
            memory = study['memory']
            para(f"{smallest} had the smallest total area. {fastest} had the shortest total synthesis time. On the 64 MiB stress case, the solver peaked at {memory['new_peak_rss_kib']:,} KiB under an enforced address-space cap and matched every row. The official evaluator reported MEMORY_LIMIT. Its wait4 reading was {memory['official_wait4_kib']:,} KiB.")
        else:
            para(f'{smallest} had the smallest total area. {fastest} had the shortest total synthesis time. In a separate synthetic stress test, the evaluator passed the 128 MiB case and returned MEMORY_LIMIT at 64 MiB.')
        para('Individual contributions and coordination', 'Heading2')
        para("Jincan Li reviewed Stage 1, identifying resource risks and the need to separate synthesis runtime from circuit delay. He archived the revised specification and released Stage 2 cases on 20 November. Zeyun DU responded to the review in writing, requested tests of mapped area and delay, shared Jincan's research repository for comparison and requested the merger. The team research archive provided the Round 5.1 source. Codex adapted the two implementations, ran the experiments and prepared this report.")
    else:
        para('UFUG2106 | Stage 2 method improvement' if frozen_root else 'UFUG2106 | Stage 2 working report', 'Title')
        if frozen_root:
            para('Group 2 &nbsp; | &nbsp; Zeyun DU (50025854) and Jincan Li (50032637)')
        para('Public comparison of two variable-order searches', 'Heading2')
        para('The comparator is the Stage 1 source delivered on 17 November. It constructs a reduced ordered binary decision diagram (BDD) from the truth table, maps it to AND2, OR2 and NOT gates, and tries further variable orders within a time budget. The order of input variables changes how much of the diagram can be shared.')
        para('The candidate first constructs a valid circuit in natural order. It uses BDD node counts to shortlist additional orders, gives each a bounded mapping trial, then refines orders with promising mapped delay and area. This tests whether inexpensive screening leaves more time for good circuits. Our search ranks completed circuits by delay above target, then area, then delay.')
        if frozen_root:
            para('An early descending-influence trial gives a promising order time to finish before screening. Later trials interleave shuffled orders and single-variable moves. For the last two variables of an order there are just 16 Boolean functions: caching their BDD literals avoids reconstructing the same subgraphs.')
        para('Official evaluator and independent truth-table replay', 'Heading2')
        table(summary)
        for stage in ('stage1','stage2'):
            group = [r for r in rows if r['stage']==stage]
            peak = max(group,key=lambda r:r['area_delta'])
            delta = sum(r['area_delta'] for r in group)
            better = sum(r['area_delta'] < 0 for r in group)
            para(f"{stage.capitalize()}: the candidate saves area on {better} of 25 cases. Its total area change is {delta:+,}; its largest increase is {peak['area_delta']:+,} on {peak['case']}.")
        para('Results and limitations','Heading2')
        if frozen_root:
            para('Across 50 cases, 16 circuits used less area, nine used more and 25 tied. Case 006 in the second bank returned to area 5,196 at delay 48. Relative to the preceding search it saved 3,012 units on that case, while case 024 required 3,456 more. The wall-clock budget can change which candidates finish.')
            para('Separate 64 MiB stress test: the evaluator rejected case 102. Its output passed all 1,048,576 assignments independently. Under a larger parent process, wait4 reported roughly 102 MiB while sampled post-exec solver usage stayed near 12 MiB. The official failure remains in the record.')
        else:
            para('The second bank exposes a costly regression that the first bank alone would miss. Correctness and meeting the delay target do not imply a lower area. The reported times come from one sequential workstation run per method and bank; the course score and hidden cases remain unknown.')
        if frozen_root:
            para('Teamwork and provenance', 'Heading2')
            para('Jincan Li reviewed the Stage 1 implementation and measurements. He called for a comparison on the expanded test set and identified the risk of building the first circuit without a resource bound. On 20 November he archived the revised handout and all 25 new public cases in the <link href="https://github.com/Jincan-LI-HUB/Truth-Table-to-Circuit-Discrete-Math-Project/commit/866add1c0a94d863870ccdf00791f4617f8818b3" color="#224657">research repository (866add1)</link>. Zeyun DU responded to the review in writing, explaining how outputs share BDD subfunctions and why synthesis runtime and circuit delay must be reported separately. For Stage 2, Zeyun requested evaluation on the new cases and investigation of mapped area and delay when selecting variable orders. This report follows those questions through the 50-case comparison and memory experiment. Codex implemented the search changes, ran the evaluations and helped prepare the report and source package.')
        else:
            para('AI-assisted work included running the comparison, checking archived sources and preparing these tables. Zeyun DU and Jincan Li still need to record their actual individual contributions and coordination before final submission.')
    for stage in ('stage1', 'stage2'):
        story.append(PageBreak());para(f'{stage.capitalize()} | 25 public cases','Title')
        para('F = submitted Stage 1 method. P = Stage 2 prototype. Runtime is in milliseconds. Both methods used the same released cases and evaluator. Each circuit was independently evaluated on the full truth table.')
        story.append(Spacer(1,4*mm))
        data=[['Case','F / P status','F area','P area','F delay','P delay','F ms','P ms']]
        for r in rows:
            if r['stage']!=stage: continue
            a,b=r['stage1_method'],r['improved_method']
            data.append([r['case'][5:8],a['status']+' / '+b['status'],a['area'],b['area'],a['delay'],b['delay'],
                         f"{a['runtime_ms']:.3f}",f"{b['runtime_ms']:.3f}"])
        table(data)
    if controls:
        story.append(PageBreak())
        para('Output selection and independent tests', 'Title')
        para('When mixing two circuits saves area', 'Heading2')
        para('Both circuits implement AND(x,y), OR(x,y), 1 and 1. Gates cost one unit. Each circuit implements one binary output with De Morgan inversion. Selecting the direct gate for each output leaves three live gates, including the single NOT(0) driving both constant outputs.')
        story.append(output_choice_figure())
        para('Fixed-seed held-out functions', 'Heading2')
        para('After fixing the method, we generated 30 functions across six families: dense random tables, addition, small lookup tables with selection logic, shared products, parity with comparison, and thresholds. The two programs ran on identical inputs in alternating order. Every passing circuit was checked on its full truth table.')
        group = controls['holdout']
        common = [r for r in group if all(r[k].get('target_met') for k in ('old','new'))]
        grid = [['Method', 'Valid', 'Delay met', 'Area on paired targets', 'Total ms']]
        labels = ('Previous selection', 'Revised schedule') if schedule else ('Previous packed', 'Output selection')
        for k, name in zip(('old', 'new'), labels):
            valid = [r[k] for r in group if r[k]['valid']]
            grid.append([name, f'{len(valid)} / {len(group)}',
                         f"{sum(r['target_met'] for r in valid)} / {len(group)}",
                         f"{sum(r[k]['area'] for r in common):,}",
                         f"{sum(r['ms'] for r in valid):,.0f}"])
        table(grid)
        para(f'The area column uses the {len(common)} functions where both methods reached the delay target. The benchmark covers generated functions with 10 to 16 inputs. The course also grades 50 hidden cases.')
        previous_file = 'vs-packed.json' if retained and not schedule else 'vs-previous.json'
        prior = json.loads((root / previous_file).read_text())['cases']
        old_delay = median(r['stage1_method']['delay'] for r in prior)
        new_delay = median(r['improved_method']['delay'] for r in prior)
        if schedule:
            replicates = schedule['repeat011']
            def span(tag, field):
                values = [r[tag][field] for r in replicates]
                a, b = min(values), max(values)
                return f'{a:,}' if a == b else f'{a:,} to {b:,}'
            savings = [-r['area_delta'] for r in prior]
            case_saving = next(-r['area_delta'] for r in prior if r['stage'] == 'stage2' and r['case'] == 'case_011.txt')
            para(f"Three paired repeats of Stage 2 case 011 gave areas of {span('old','area')} previously and {span('new','area')} with the revised schedule. Its target is {replicates[0]['new']['target']}. Delay was {span('old','delay')} previously and {span('new','delay')} after the change. This case accounts for {case_saving:,} of the {sum(savings):,} public area units saved.")
        timing_note = ('The two schedules used the same time budget.' if schedule else
                       'The new solver uses some of that timing slack to save gates.')
        para(f'On the original public cases the median circuit delay went from {old_delay:g} to {new_delay:g}. Both solvers reached every public target. {timing_note}')
        para('What does output selection contribute?', 'Heading2')
        pairs = controls['ablation']
        gains = [r['whole']['area'] - r['new']['area'] for r in pairs]
        met = [sum(r[k]['target_met'] for r in pairs) for k in ('whole','new')]
        para(f"On eight selected public cases we disabled only output replacement, keeping the candidate generators, compiler and wall-clock budget fixed. Whole-circuit selection reached {met[0]}/8 delay targets and output replacement reached {met[1]}/8. Replacement saved {sum(gains):,} area units in total. Area fell on {sum(x>0 for x in gains)}, tied on {gains.count(0)} and rose on {sum(x<0 for x in gains)}. Wall-clock cutoffs can change which candidates finish.")
    if permutation_control:
        banks = json.loads(permutation_control.read_text())
        story.append(PageBreak())
        para('Would a different input order change the result?', 'Title')
        para('The public truth tables specify an order for their primary inputs. Reordering the inputs of an existing circuit gives a circuit with the same gates, gate costs and path delays. A search that tries different orders within a deadline can nevertheless return a different circuit.')
        para('Paired input-renaming experiment', 'Heading2')
        para('For each of the 50 public functions we fixed one nonidentity input permutation, reordered the truth-table rows accordingly, and kept the gate costs, output bits and delay target fixed. We alternated which solver ran first. Both solvers received exactly the same transformed table for each case.')
        data = [['Bank', 'Targets met (F / P)', 'F area', 'P area', 'Smaller / tied / larger']]
        total_frozen = total_candidate = assignments = 0
        all_gains = []
        for bank in ('1', '2'):
            group = banks[bank]
            old, new = (group['arms'][k] for k in ('stage1', 'stage2'))
            gains = group['gains']
            count = group['cases']
            assert len(gains) == count and old['status'] == new['status'] == {'PASS': count}
            assert sum(gains.values()) == old['paired_area'] - new['paired_area']
            data.append([bank, f"{old['targets_met']} / {new['targets_met']}", f"{old['paired_area']:,}",
                         f"{new['paired_area']:,}", f"{group['fewer']} / {group['equal']} / {group['more']}"])
            total_frozen += old['paired_area']
            total_candidate += new['paired_area']
            assignments += group['reference_assignments']
            all_gains.extend((gain, bank, case) for case, gain in gains.items())
        story.append(Spacer(1, 3*mm))
        table(data)
        para('What changed?', 'Heading2')
        gain = total_frozen - total_candidate
        largest, bank, case = max(all_gains)
        regressions = sum(change < 0 for change, _, _ in all_gains)
        para(f"Stage 2 saved {gain:,} area units ({100*gain/total_frozen:.2f}%). Bank {bank}, case {case[5:8]}, supplied {largest:,} units of that gain. Removing it leaves a saving of {gain-largest:,}. {regressions} transformed cases used more area. The overall saving therefore coexists with substantial variation across cases.")
        para('Why the comparison is valid', 'Heading2')
        para(f"We renamed the input wires in all 50 archived baseline circuits and checked them on all {assignments:,} transformed assignments. Every circuit retained its original area and delay. We separately replayed the truth tables for the 100 newly synthesised circuits and matched the official measurements.")
        para('What can transfer?', 'Heading2')
        para('Both the original and transformed orderings favour Stage 2 in total area. The transformed inputs reuse the same underlying functions. Performance on previously unseen functions remains for the course evaluation.')
    SimpleDocTemplate(str(output),pagesize=A4,leftMargin=16*mm,rightMargin=16*mm,
                      topMargin=16*mm,bottomMargin=14*mm,title='UFUG2106 Stage 2 public comparison').build(story)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('results',type=Path)
    parser.add_argument('pdf',type=Path)
    parser.add_argument('--frozen-root', type=Path, help='Original two frozen Stage 1 evaluations for the case-006 follow-up report')
    parser.add_argument('--permutation-control', type=Path, help='Verified control-summary.json for the input-renaming experiment')
    parser.add_argument('--merged', action='store_true', help='Report the integrated Round 5.1 and Stage 2 run')
    args = parser.parse_args()
    if args.merged and args.permutation_control:
        parser.error('--merged uses the original public banks')
    render(args.results,args.pdf,args.frozen_root,args.permutation_control,args.merged)
