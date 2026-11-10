#!/usr/bin/env python3
import argparse, csv, json, platform, statistics, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline', type=Path, default=ROOT / 'results/baseline.csv')
parser.add_argument('--optimized', type=Path, default=ROOT / 'results/final/optimized.csv')
parser.add_argument('--independent', type=Path, default=ROOT / 'results/final/independent.json')
parser.add_argument('--output', type=Path, default=ROOT / 'results/final/summary.json')
args = parser.parse_args()

with args.baseline.open(newline='') as f:
    baseline = list(csv.DictReader(f))
with args.optimized.open(newline='') as f:
    optimized = list(csv.DictReader(f))
assert [x['case'] for x in baseline] == [x['case'] for x in optimized]
areas = [int(y['area']) / int(x['area']) for x, y in zip(baseline, optimized)]
times = [float(y['runtime_ms']) for y in optimized]
independent = json.loads(args.independent.read_text())
summary = {
    'baseline_pass': sum(x['status'] == 'PASS' for x in baseline),
    'optimized_pass': sum(x['status'] == 'PASS' for x in optimized),
    'baseline_target_met': sum(x['target_met'] == 'yes' for x in baseline),
    'optimized_target_met': sum(x['target_met'] == 'yes' for x in optimized),
    'baseline_area_sum': sum(int(x['area']) for x in baseline),
    'optimized_area_sum': sum(int(x['area']) for x in optimized),
    'area_lower_cases': sum(float(y['area']) < float(x['area']) for x, y in zip(baseline, optimized)),
    'area_ratio_median': statistics.median(areas),
    'optimized_runtime_median_ms': statistics.median(times),
    'optimized_runtime_max_ms': max(times),
    'baseline_runtime_median_ms': statistics.median(float(x['runtime_ms']) for x in baseline),
    'total_assignments': sum(x['assignments'] for x in independent['cases']),
    'compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
    'host': platform.platform(),
}
summary['aggregate_area_reduction'] = 1 - summary['optimized_area_sum'] / summary['baseline_area_sum']
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
