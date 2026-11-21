#!/usr/bin/env python3
"""Sequential policy ablations of the frozen Stage 1 source, with fresh evidence."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VARIANTS = {
    'control': None,
    'no_order_search': ('for (int trial = 0; trial < 32; ++trial)',
                        'for (int trial = 0; trial < 1; ++trial)'),
    'no_balance': ('Net balance(const Net &in, const Case &c) {',
                   'Net balance(const Net &in, const Case &c) {\n    return in; // experimental ablation'),
    'no_affine_mapping': ('if (!plain && affine[lit] != UINT32_MAX)',
                          'if (false && !plain && affine[lit] != UINT32_MAX)'),
}

DEFAULT_VARIANTS = list(VARIANTS)
for cap in (250, 500, 750):
    VARIANTS[f'cap_{cap}'] = (
        'double budget_ms = c.limit > 0 ? std::min(1500.0, 0.60 * double(c.limit)) : 1500.0;',
        f'double budget_ms = c.limit > 0 ? std::min({cap}.0, 0.60 * double(c.limit)) : {cap}.0;')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--repeats', type=int, default=2)
    p.add_argument('--variants', nargs='+', choices=list(VARIANTS), default=DEFAULT_VARIANTS)
    args = p.parse_args()
    if args.repeats < 1:
        p.error('repeats must be positive')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    source = (ROOT/'releases/stage1_candidate/synth.cpp').read_text()
    entries = []
    for name in args.variants:
        edit = VARIANTS[name]
        text = source
        if edit:
            assert text.count(edit[0]) == 1, (name, 'source anchor changed')
            text = text.replace(*edit)
        (out/(name+'.cpp')).write_text(text)
    for repeat in range(1,args.repeats+1):
        order = list(args.variants)
        if repeat % 2 == 0:
            order.reverse()
        for name in order:
            run = out/f'{name}-r{repeat}'
            print(f'{name} repeat {repeat}', flush=True)
            command = [sys.executable,str(ROOT/'tools/evaluate_release.py'),
                       '--source',str(out/(name+'.cpp')),'--output',str(run)]
            with (out/f'{name}-r{repeat}.log').open('w') as log:
                status = subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
            entry = {'variant':name,'repeat':repeat,'exit_code':status,
                     'evaluation':str(run.relative_to(out)),
                     'manifest':json.loads((run/'manifest.json').read_text())}
            if repeat == 1 and (run/'synth').exists():
                with (out/f'{name}-regression.log').open('w') as log:
                    result = subprocess.run([sys.executable,str(ROOT/'tests/regression.py'),
                        str(run/'synth'),str(out/f'{name}-regression.json')],
                        stdout=log,stderr=subprocess.STDOUT,timeout=300)
                entry['regression_exit_code'] = result.returncode
            entries.append(entry)
            (out/'ablation.json').write_text(json.dumps({'scope':
                'Sequential repetitions with alternating order; same hardware/cases. Component variants retain the budget policy; cap variants change only its ceiling. Effects include changed search completion, not isolated intrinsic contributions. Affine detection remains enabled.',
                'variants':{name:VARIANTS[name] for name in args.variants},'runs':entries},indent=2)+'\n')
            print(json.dumps({'variant':name,'repeat':repeat,'status':entry['manifest']['status'],
                  'summary':entry['manifest'].get('summary'),'regression':entry.get('regression_exit_code')}),flush=True)
    return int(any(e['exit_code'] or e.get('regression_exit_code',0) for e in entries))


if __name__ == '__main__':
    sys.exit(main())
