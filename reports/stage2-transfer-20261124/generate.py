"""Generate a fixed diagnostic bank and legal witness circuits in a fresh directory."""
import argparse
import csv
import random
from pathlib import Path

import numpy as np


class Witness:
    def __init__(self, n, costs):
        self.n = n
        rows = np.arange(1 << n)
        self.values = [np.zeros(len(rows), dtype=bool)]
        self.values.extend((rows & (1 << (n - i))) != 0 for i in range(1, n + 1))
        self.delay = [0] * (n + 1)
        self.costs = costs
        self.gates = []

    def gate(self, op, *inputs):
        op_fn = {'AND2': np.logical_and, 'OR2': np.logical_or, 'NOT': np.logical_not}
        i = len(self.values)
        self.values.append(op_fn[op](*(self.values[x] for x in inputs)))
        self.delay.append(max(self.delay[x] for x in inputs) + self.costs[op][1])
        self.gates.append(' '.join(map(str, (i, op, *inputs))))
        return i

    def xor(self, a, b):
        either = self.gate('OR2', a, b)
        both = self.gate('AND2', a, b)
        return self.gate('AND2', either, self.gate('NOT', both))

    def mux(self, s, a, b):
        return self.gate('OR2', self.gate('AND2', self.gate('NOT', s), a), self.gate('AND2', s, b))


def outputs(w, x, family, seed):
    if family == 'mux':
        return [w.mux(x[1], w.mux(x[0], *x[i:i+2]), w.mux(x[0], *x[i+2:i+4]))
                for i in (2, 6)]
    if family == 'parity':
        pairs = [w.xor(*x[i:i+2]) for i in range(0, 8, 2)]
        halves = [w.xor(*pairs[i:i+2]) for i in (0, 2)]
        return [*halves, w.xor(*halves)]
    if family == 'carry':
        carry = x[8]
        result = []
        for a, b in zip(x[:4], x[4:8]):
            propagate = w.xor(a, b)
            result.append(w.xor(propagate, carry))
            carry = w.gate('OR2', w.gate('AND2', a, b), w.gate('AND2', propagate, carry))
        return [*result, carry]
    rng = random.Random(seed)
    pool = x.copy()
    for _ in range(18):
        a, b = rng.sample(pool[-8:], 2)
        pool.append(w.gate(rng.choice(['AND2', 'OR2']), a, b))
        if len(pool) % 3 == 0:
            pool.append(w.gate('NOT', pool[-1]))
    return pool[-4:]


def generate(root, seed):
    cases = root / 'cases'
    cases.mkdir(parents=True)
    circuits = root / 'witnesses'
    circuits.mkdir()
    with (root / 'design.csv').open('w') as f:
        writer = csv.writer(f)
        writer.writerow(['case', 'family', 'permutation', 'cost_profile', 'target'])
        case_id = 0
        for family in ['mux', 'parity', 'carry', 'shared_dag']:
            for permutation in range(2):
                x = list(range(1, 13))
                random.Random(seed + permutation).shuffle(x)
                for profile, prices in enumerate([((2,2),(8,3),(1,1)), ((8,3),(2,2),(1,1)), ((3,1),(3,1),(4,3))]):
                    costs = dict(zip(['AND2', 'OR2', 'NOT'], prices))
                    w = Witness(len(x), costs)
                    out = outputs(w, x, family, seed)
                    target = max(w.delay[o] for o in out)
                    case_id += 1
                    name = f'case_{case_id:03d}.txt'
                    table = ' '.join(''.join('1' if v else '0' for v in row)
                                     for row in zip(*(w.values[i] for i in out)))
                    gates = '\n'.join(f'{a} {d}' for a, d in prices)
                    (cases / name).write_text(f'{w.n}\n{len(out)}\n{table}\n{gates}\n{target}\n2000\n0\n')
                    (circuits / name).write_text('\n'.join([*w.gates,' '.join(map(str,out))])+'\n')
                    writer.writerow([name, family, permutation, profile, target])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('new_directory', type=Path)
    parser.add_argument('--seed', type=int, default=112406)
    args = parser.parse_args()
    generate(args.new_directory, args.seed)
