#!/usr/bin/env python3
"""Summarize retained official/independent ablation runs without rerunning them."""
import argparse,csv,json,statistics
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runs',type=Path,nargs='+',required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    groups={};data={};regressions={}
    for directory in a.runs:
        experiment=json.loads((directory/'ablation.json').read_text())
        for entry in experiment['runs']:
            run=directory/entry['evaluation'];m=json.loads((run/'manifest.json').read_text())
            assert m['status']=='PASS',run
            independent=json.loads((run/'independent.json').read_text())
            assert len(independent['cases'])==25 and all(c['valid'] for c in independent['cases'])
            rows=list(csv.DictReader((run/'official.csv').open()))
            assert len(rows)==25 and all(r['status']=='PASS' for r in rows)
            name=entry['variant'];repeat=entry['repeat'];data[name,repeat]={r['case']:r for r in rows}
            groups.setdefault(name,[]).append({'repeat':repeat,'area':sum(int(r['area']) for r in rows),
                'median_ms':statistics.median(float(r['runtime_ms']) for r in rows),
                'total_ms':sum(float(r['runtime_ms']) for r in rows),
                'targets':sum(r['target_met']=='yes' for r in rows),
                'misses':[r['case'] for r in rows if r['target_met']!='yes'],
                'csv':str(run/'official.csv')})
            reg=directory/f'{name}-regression.json'
            if name not in regressions:
                r=json.loads(reg.read_text());assert r['all_passed'] and r['generated_cases']==92
                regressions[name]={k:r[k] for k in ['all_passed','generated_cases','checker_mutations_rejected']}
    assert all(len(v)==2 for v in groups.values())
    summary={'scope':'Two sequential alternating-order repetitions per variant; same workstation and 25 public cases. Wall-clock completion and component interactions remain confounders. Not an official score.',
             'variants':groups,'regressions':regressions}
    (a.output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    with (a.output/'paired.csv').open('w') as f:
        w=csv.writer(f);w.writerow(['variant','repeat','case','control_area','variant_area','control_delay','variant_delay','control_runtime_ms','variant_runtime_ms','target_met'])
        for (name,repeat),rows in data.items():
            for case,r in rows.items():
                c=data['control',repeat][case]
                w.writerow([name,repeat,case,c['area'],r['area'],c['delay'],r['delay'],c['runtime_ms'],r['runtime_ms'],r['target_met']])
    for name,rows in groups.items():
        print(name,[(r['area'],r['median_ms'],r['targets']) for r in rows])
if __name__=='__main__':main()
