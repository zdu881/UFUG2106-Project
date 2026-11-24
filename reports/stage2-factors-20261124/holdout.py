"""Generate unopened families with legal witnesses from their gate-level definitions."""
import csv
from pathlib import Path
import random
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'stage2-transfer-20261124'))
from generate import Witness


def signals(w, x, family):
    inv = lambda a: w.gate('NOT', a)
    both = lambda a, b: w.gate('AND2', a, b)
    either = lambda a, b: w.gate('OR2', a, b)
    if family == 'comparator':
        equal, greater = inv(0), 0
        for a, b in zip(x[:4], x[4:8]):
            greater = either(greater, both(equal, both(a, inv(b))))
            equal = both(equal, inv(w.xor(a, b)))
        return [equal, greater, either(equal, greater)]
    if family == 'priority':
        used, outputs = 0, [0, 0, 0]
        for i, pin in reversed(list(enumerate(x[:8]))):
            sel = both(pin, inv(used))
            for k in range(3):
                if (i >> k) & 1:
                    outputs[k] = either(outputs[k], sel)
            used = either(used, pin)
        return [*outputs, used]
    carry = x[7]
    bits = []
    for pin in x[:7]:
        bits.append(w.xor(pin, carry))
        carry = both(carry, pin)
    return [*bits, carry]


def generate(root):
    cases, witnesses = root/'cases', root/'witnesses'
    cases.mkdir(parents=True)
    witnesses.mkdir()
    with (root/'design.csv').open('w') as f:
        writer = csv.writer(f)
        writer.writerow(['case','family','permutation','profile','target'])
        number = 0
        for family in ('comparator','priority','counter'):
            for perm in range(2):
                x = list(range(1, 13))
                random.Random(391217 + perm).shuffle(x)
                for profile, costs in enumerate([((2,2),(8,3),(1,1)), ((8,3),(2,2),(1,1)), ((3,1),(3,1),(4,3))]):
                    w = Witness(12, dict(zip(('AND2','OR2','NOT'), costs)))
                    out = signals(w, x, family)
                    target = max(w.delay[i] for i in out)
                    number += 1
                    name = f'case_{number:03d}.txt'
                    table = ' '.join(''.join('1' if bit else '0' for bit in row)
                                     for row in zip(*(w.values[i] for i in out)))
                    prices = '\n'.join(f'{a} {d}' for a,d in costs)
                    (cases/name).write_text(f'12\n{len(out)}\n{table}\n{prices}\n{target}\n300\n65536\n')
                    (witnesses/name).write_text('\n'.join([*w.gates,' '.join(map(str,out))])+'\n')
                    writer.writerow([name,family,perm,profile,target])


if __name__ == '__main__':
    generate(Path(sys.argv[1]))
