#!/usr/bin/env python3
import argparse,csv,json
from pathlib import Path
from check_circuit import verify
p=argparse.ArgumentParser();p.add_argument('cases',type=Path);p.add_argument('outputs',type=Path);p.add_argument('official_csv',type=Path);p.add_argument('json',type=Path);a=p.parse_args()
official={r['case']:r for r in csv.DictReader(a.official_csv.open())};results=[]
for case in sorted(a.cases.glob('*.txt')):
    result=verify(case,a.outputs/(case.stem+'.output.txt'))
    row=official[case.name]
    assert row['status']=='PASS'
    assert result['area']==int(row['area']) and result['delay']==int(row['delay'])
    assert result['target_met']==(row['target_met']=='yes')
    results.append(result);print(case.name,'PASS',flush=True)
a.json.write_text(json.dumps({'independent':True,'all_rows_checked':True,'cases':results},indent=2)+'\n')
