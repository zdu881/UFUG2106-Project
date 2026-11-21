"""Generate the three 64-bit Linux memory controls used in the report."""
import argparse
from pathlib import Path
import random

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
dest = parser.parse_args().output
dest.mkdir(parents=True, exist_ok=False)

for n, m, seed, ram_mib, and_cost, or_cost in [
    (17, 5, 261117, 32, 3, 1),
    (18, 6, 261118, 48, 2, 2),
    (19, 4, 210698, 64, 2, 3),
]:
    rng = random.Random(seed)
    tokens = [format(i, f'0{m}b') for i in range(1 << m)]
    with (dest/f'dense{n}.txt').open('w') as output:
        output.write(f'{n}\n{m}\n')
        for _ in range(1 << (n - 10)):
            output.write(' '.join(tokens[rng.getrandbits(m)] for _ in range(1024)) + ' ')
        output.write(f'\n{and_cost} 1\n{or_cost} 2\n1 1\n100\n10000\n{ram_mib*1024}\n')
