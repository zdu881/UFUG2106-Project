"""Generate 12 constrained DAGs and two exact-delay controls from legal gates."""
import csv
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'stage2-transfer-20261124'))
from generate import Witness


def generate(root, seed):
    cases = root / 'cases'
    witnesses = root / 'witnesses'
    cases.mkdir(parents=True)
    witnesses.mkdir()
    with (root/'design.csv').open('w') as f:
        writer = csv.writer(f)
        writer.writerow(['case','inputs','outputs','target','limit_ms','ram_kib'])
        for i in range(14):
            rng = random.Random(seed+i)
            n = [10,12,14][i % 3] if i < 12 else 2
            ops = ('AND2','OR2','NOT')
            costs = {op:(rng.randint(1,7),rng.randint(1,3)) for op in ops}
            if i == 12:
                costs['AND2'] = (1,1)
            if i == 13:
                costs['NOT'] = (1,3)
            w = Witness(n, costs)
            layer = list(range(1,n+1))
            if i < 12:
                for depth in range(3+i%4):
                    next_layer = []
                    for j in range(n):
                        a,b = rng.sample(layer,2)
                        next_layer.append(w.xor(a,b) if j%3 == 0 else w.gate(rng.choice(ops[:2]),a,b))
                    layer = next_layer
                outputs = layer[:4+i%4]
            else:
                outputs = [w.gate('AND2',1,2)] if i == 12 else [w.gate('NOT',1)]
            target = max(w.delay[o] for o in outputs)
            # The generator size and release CPU/RAM budgets vary independently.
            limit = [250,400,650,850][i%4]
            ram = 1024*[32,64,48,96][i%4]
            name = f'case_{i+1:03d}.txt'
            table = ' '.join(''.join('1' if bit else '0' for bit in row)
                             for row in zip(*(w.values[o] for o in outputs)))
            prices = '\n'.join(f'{costs[op][0]} {costs[op][1]}' for op in ops)
            (cases/name).write_text(f'{n}\n{len(outputs)}\n{table}\n{prices}\n{target}\n{limit}\n{ram}\n')
            (witnesses/name).write_text('\n'.join([*w.gates,' '.join(map(str,outputs))])+'\n')
            writer.writerow([name,n,len(outputs),target,limit,ram])


if __name__ == '__main__':
    generate(Path(sys.argv[1]), int(sys.argv[2]) if len(sys.argv) > 2 else 261124701)
