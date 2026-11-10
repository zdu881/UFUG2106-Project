#!/usr/bin/env python3
import csv,json,statistics,hashlib,platform,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1]
b=list(csv.DictReader((r/'results/baseline.csv').open()));o=list(csv.DictReader((r/'results/final/optimized.csv').open()))
assert [x['case'] for x in b]==[x['case'] for x in o]
areas=[int(y['area'])/int(x['area']) for x,y in zip(b,o)];times=[float(y['runtime_ms']) for y in o]
ind=json.loads((r/'results/final/independent.json').read_text())
s={'baseline_pass':sum(x['status']=='PASS' for x in b),'optimized_pass':sum(x['status']=='PASS' for x in o),'baseline_target_met':sum(x['target_met']=='yes' for x in b),'optimized_target_met':sum(x['target_met']=='yes' for x in o),'baseline_area_sum':sum(int(x['area']) for x in b),'optimized_area_sum':sum(int(x['area']) for x in o),'area_lower_cases':sum(float(y['area'])<float(x['area']) for x,y in zip(b,o)),'area_ratio_median':statistics.median(areas),'optimized_runtime_median_ms':statistics.median(times),'optimized_runtime_max_ms':max(times),'baseline_runtime_median_ms':statistics.median(float(x['runtime_ms']) for x in b),'total_assignments':sum(x['assignments'] for x in ind['cases']),'compiler':subprocess.check_output(['g++','--version'],text=True).splitlines()[0],'host':platform.platform(),'hashes':{str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [r/'src/synth.cpp',r/'src/baseline.cpp',r/'build/synth',r/'build/baseline']}}
s['aggregate_area_reduction']=1-s['optimized_area_sum']/s['baseline_area_sum']
(r/'results/final/summary.json').write_text(json.dumps(s,indent=2)+'\n')
print(json.dumps(s,indent=2))
