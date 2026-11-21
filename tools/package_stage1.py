#!/usr/bin/env python3
"""Package Stage 1 source and retained evaluation evidence."""
import argparse
import json
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = '''CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wpedantic
.PHONY: all baseline clean
all: build/synth
baseline: build/baseline
build/synth: src/synth.cpp
\tmkdir -p build
\t$(CXX) $(CXXFLAGS) $< -o $@
build/baseline: src/baseline.cpp
\tmkdir -p build
\t$(CXX) $(CXXFLAGS) $< -o $@
clean:
\trm -f build/synth build/baseline
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--study', type=Path, default=ROOT/'results/stage1-study-summary-20261116/summary.json', help='Include verified study summary and its referenced CSVs')
    args = parser.parse_args()
    source = ROOT / 'src/synth.cpp'
    if source.read_bytes() != (ROOT / 'releases/stage1_candidate/synth.cpp').read_bytes():
        raise ValueError('Stage 1 source differs from the frozen candidate')
    files = {name: ROOT / name for name in [
        'src/synth.cpp', 'src/baseline.cpp', 'docs/METHOD.md',
        'releases/stage1_candidate/synth.cpp',
        'examples/shared_outputs.txt',
        'tools/check_circuit.py', 'tools/verify_all.py', 'tools/evaluate_release.py',
        'tools/compare_stage_results.py', 'tests/regression.py',
        'tests/test_comparison.py', 'tests/test_release.py']}
    files['README.md'] = ROOT / 'docs/STAGE1_SUBMISSION_README.md'
    files['docs/TEAM_REVIEW.md'] = ROOT / 'docs/STAGE1_TEAM_REVIEW_20261115.md'
    files['docs/AI_WORK_LOG.md'] = ROOT / 'docs/STAGE1_AI_PROVENANCE.md'
    for name in ['baseline.csv', 'cap250.csv', 'cap500.csv',
                 'final/regression.json', 'final/sanitizers.json',
                 'final/stress_independent.json', 'final/baseline_independent.json',
                 'stage1-final-20261110/official.csv',
                 'stage1-final-20261110/independent.json',
                 'stage1-final-20261110/manifest.json']:
        files['evidence/' + name] = ROOT / 'results' / name
    payload = {name: path.read_bytes() for name, path in files.items()}
    study = json.loads(args.study.read_text())
    for name, rows in study['variants'].items():
        study_dir = Path(rows[0]['csv']).parent.parent
        for row in rows:
            original = Path(row['csv'])
            archived = f"evidence/stage1-study/{name}-r{row['repeat']}.csv"
            payload[archived] = original.read_bytes()
            row['csv'] = archived
        regression = study_dir / f'{name}-regression.json'
        payload[f'evidence/stage1-study/{name}-regression.json'] = regression.read_bytes()
    payload['evidence/stage1-study/summary.json'] = (json.dumps(study,indent=2)+'\n').encode()
    payload['evidence/stage1-study/paired.csv'] = args.study.with_name('paired.csv').read_bytes()
    for name in ['tools/stage1_ablation.py','tools/summarize_stage1_study.py','docs/STAGE1_STUDY_20261116.md']:
        payload[name] = (ROOT/name).read_bytes()
    payload['Makefile'] = MAKEFILE.encode()
    # The method document's final availability observation is historical.
    payload['docs/METHOD.md'] = payload['docs/METHOD.md'].replace(
        b'Stage 2 data and hidden cases are not currently available.',
        b'At the 13 November release check, no Stage 2 cases were found; hidden cases are unavailable.')
    args.output.mkdir(parents=True, exist_ok=False)
    archive = args.output / 'UFUG2106_Group2_Stage1_Source.zip'
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED) as z:
        for name, content in sorted(payload.items()):
            z.writestr('UFUG2106_Stage1/' + name, content)
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:
            raise ValueError('ZIP integrity failure')
    print(archive)


if __name__ == '__main__':
    main()
