#!/usr/bin/env python3
"""Build and verify a source snapshot in a fresh evidence directory."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys
import time

from compare_stage_results import read_csv

ROOT = Path(__file__).resolve().parents[1]


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_cases(report, cases):
    rows = read_csv(report)
    expected = {p.name for p in cases}
    if rows.keys() != expected:
        raise ValueError('Evaluator case set differs from the input snapshot')
    if not all(row['passed'] for row in rows.values()):
        raise ValueError('One or more official cases failed; retain the CSV and logs')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'src/synth.cpp')
    parser.add_argument('--cases', type=Path, default=ROOT / 'official/stage1')
    parser.add_argument('--evaluator', type=Path, default=ROOT / 'official/evaluator')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=300, help='Wall timeout per pipeline command')
    args = parser.parse_args()
    source, cases_dir, evaluator = (p.resolve(strict=True) for p in
                                     (args.source, args.cases, args.evaluator))
    cases = sorted(cases_dir.glob('*.txt'))
    if not cases or args.timeout <= 0:
        parser.error('Nonempty case directory and positive timeout required')
    out = args.output.resolve()
    # A new directory prevents accidental use of reports from an earlier run.
    out.mkdir(parents=True, exist_ok=False)
    (out / 'cases').mkdir()
    (out / 'verifier').mkdir()
    shutil.copy2(source, out / 'synth.cpp')
    shutil.copy2(evaluator, out / 'evaluator')
    for case in cases:
        shutil.copy2(case, out / 'cases' / case.name)
    for name in ('verify_all.py', 'check_circuit.py', 'evaluate_release.py', 'compare_stage_results.py'):
        shutil.copy2(ROOT / 'tools' / name, out / 'verifier' / name)
    snapshot_paths = [out / 'synth.cpp', out / 'evaluator'] + list((out / 'cases').iterdir()) + list((out / 'verifier').iterdir())
    hashes = {str(p.relative_to(out)): sha256(p) for p in sorted(snapshot_paths)}
    compiler = shutil.which(os.environ.get('CXX', 'g++'))
    manifest = {'started_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'status': 'RUNNING', 'source_path': str(source), 'case_count': len(cases),
                'platform': platform.platform(), 'python': sys.version, 'compiler': compiler,
                'inputs_sha256': hashes, 'commands': [],
                'scope': 'Named local case snapshot; not hidden-case or Stage 2 certification'}
    env = os.environ.copy()
    env.pop('PYTHONOPTIMIZE', None)
    env['PYTHONDONTWRITEBYTECODE'] = '1'

    def run(name, command):
        started = time.monotonic()
        record = {'name': name, 'argv': [str(x) for x in command]}
        manifest['commands'].append(record)
        print(name, flush=True)
        with (out / (name + '.log')).open('w') as log:
            try:
                result = subprocess.run(record['argv'], stdout=log, stderr=subprocess.STDOUT,
                                        env=env, timeout=args.timeout, check=False)
                record['exit_code'] = result.returncode
            except subprocess.TimeoutExpired:
                record['exit_code'] = 124
            record['wall_seconds'] = round(time.monotonic() - started, 3)
        if record['exit_code']:
            raise RuntimeError(f'{name} failed (exit {record["exit_code"]}); inspect {name}.log')

    try:
        if not compiler:
            raise RuntimeError('C++ compiler not found')
        run('python-dependencies', [sys.executable, '-c', 'import numpy; print(numpy.__version__)'])
        run('compiler-version', [compiler, '--version'])
        run('build', [compiler, '-O2', '-std=c++17', '-Wall', '-Wextra', '-Wpedantic',
                      out / 'synth.cpp', '-o', out / 'synth'])
        manifest['binary_sha256'] = sha256(out / 'synth')
        run('official', [out / 'evaluator', out / 'synth', out / 'cases',
                         '--report', out / 'official.csv', '--save-outputs', out / 'outputs'])
        rows = validate_cases(out / 'official.csv', list((out / 'cases').glob('*.txt')))
        run('independent', [sys.executable, out / 'verifier/verify_all.py', out / 'cases',
                            out / 'outputs', out / 'official.csv', out / 'independent.json'])
        if any(sha256(out / name) != digest for name, digest in hashes.items()):
            raise RuntimeError('An input snapshot changed during evaluation')
        if sha256(out / 'synth') != manifest['binary_sha256']:
            raise RuntimeError('The evaluated binary changed during evaluation')
        manifest['summary'] = {'passed': len(rows),
            'target_met': sum(r['target_met'] for r in rows.values()),
            'area_sum': sum(r['area'] for r in rows.values()),
            'runtime_median_ms': statistics.median(r['runtime_ms'] for r in rows.values())}
        manifest['status'] = 'PASS'
    except Exception as error:
        manifest['status'] = 'FAIL'
        manifest['error'] = str(error)
    finally:
        manifest['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        manifest['artifacts_sha256'] = {str(p.relative_to(out)): sha256(p) for p in sorted(out.rglob('*'))
                                      if p.is_file() and p.name != 'manifest.json'}
        (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({k: manifest[k] for k in ('status', 'summary', 'error') if k in manifest}, indent=2))
    return 0 if manifest['status'] == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
