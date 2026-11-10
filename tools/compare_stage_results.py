#!/usr/bin/env python3
"""Compare official CSV pairs without dropping failures or conflating case names.
Example: --pair stage1 old-stage1.csv new-stage1.csv --pair stage2 old-stage2.csv new-stage2.csv
The caller must preserve evaluator provenance: CSV contents alone cannot prove which executable ran.
"""
import argparse,csv,json,math
from pathlib import Path

def read_csv(path):
    rows={}
    with Path(path).open(newline='') as f:
        reader=csv.DictReader(f)
        required={'case','status','area','delay','runtime_ms','target_met'}
        if not required.issubset(reader.fieldnames or []):raise ValueError(f'{path}: missing columns')
        for row in reader:
            name=row['case']
            if not name or name in rows:raise ValueError(f'{path}: empty or duplicated case')
            parsed={'case':name,'status':row['status'],'passed':row['status']=='PASS'}
            for key in ('area','delay','runtime_ms'):
                if parsed['passed']:
                    value=float(row[key])
                    if not math.isfinite(value) or value<0:raise ValueError(f'{name}: invalid {key}')
                    if key!='runtime_ms' and not value.is_integer():raise ValueError(f'{name}: non-integer {key}')
                    parsed[key]=value if key=='runtime_ms' else int(value)
                else:parsed[key]=None  # retain failure, never treat an empty metric as zero
            if parsed['passed'] and row['target_met'] not in ('yes','no'):
                raise ValueError(f'{name}: invalid target flag')
            parsed['target_met']=parsed['passed'] and row['target_met']=='yes'
            rows[name]=parsed
    if not rows:raise ValueError(f'{path}: no cases')
    return rows

def compare(pairs):
    all_rows=[];labels=set()
    for label,old_path,new_path in pairs:
        if not label or label in labels:raise ValueError('stage labels must be nonempty and unique')
        labels.add(label);old,new=read_csv(old_path),read_csv(new_path)
        if old.keys()!=new.keys():raise ValueError(f'{label}: case sets differ; refusing an incomplete comparison')
        for name in sorted(old):
            a,b=old[name],new[name];both=a['passed'] and b['passed']
            row={'stage':label,'case':name,'stage1_method':a,'improved_method':b}
            row['both_pass']=both
            row['area_delta']=b['area']-a['area'] if both else None
            row['delay_delta']=b['delay']-a['delay'] if both else None
            row['runtime_delta_ms']=b['runtime_ms']-a['runtime_ms'] if both else None
            all_rows.append(row)
    both=[r for r in all_rows if r['both_pass']]
    summary={'case_count':len(all_rows),'both_pass_count':len(both),
             'stage1_method_pass':sum(r['stage1_method']['passed'] for r in all_rows),
             'improved_method_pass':sum(r['improved_method']['passed'] for r in all_rows),
             'regressed_validity':sum(r['stage1_method']['passed'] and not r['improved_method']['passed'] for r in all_rows),
             'recovered_validity':sum(not r['stage1_method']['passed'] and r['improved_method']['passed'] for r in all_rows),
             'stage1_method_target_met':sum(r['stage1_method']['target_met'] for r in all_rows),
             'improved_method_target_met':sum(r['improved_method']['target_met'] for r in all_rows),
             'area_lower_on_both_pass':sum(r['area_delta']<0 for r in both),
             'area_equal_on_both_pass':sum(r['area_delta']==0 for r in both),
             'area_higher_on_both_pass':sum(r['area_delta']>0 for r in both)}
    return {'scope':'CSV comparison; actual release coverage and executable provenance must be verified separately',
            'stages':sorted(labels),'summary':summary,'cases':all_rows}

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pair',nargs=3,action='append',required=True,metavar=('STAGE','STAGE1_METHOD_CSV','IMPROVED_METHOD_CSV'));p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();result=compare(a.pair);a.output.parent.mkdir(parents=True,exist_ok=True)
    a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['summary'],indent=2))
if __name__=='__main__':main()
