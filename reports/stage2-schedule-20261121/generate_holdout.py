"""Recreate the final paired benchmark. Run with a new output directory."""
import argparse
from pathlib import Path
import random

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
destination = parser.parse_args().output
destination.mkdir(parents=True, exist_ok=False)
rng = random.Random(81952463)
for i in range(30):
    family, replicate = i % 6, i // 6
    n = 12 + replicate if family == 0 else 10 + (replicate + family) % 7
    m = 3 + (replicate + family) % 4
    permutation = list(range(n))
    rng.shuffle(permutation)
    masks = [rng.getrandbits(n) for _ in range(m)]
    cubes = [rng.sample(range(n), 3) for _ in range(9)]
    lut = [[rng.getrandbits(1) for _ in range(64)] for _ in range(m)]
    width = n // 2
    half = (1 << width) - 1
    rows = []
    for row in range(1 << n):
        x = sum(((row >> wire) & 1) << j for j, wire in enumerate(permutation))
        a, b = x & half, (x >> width) & half
        if family == 0:
            out = [rng.getrandbits(1) for _ in range(m)]
        elif family == 1:
            out = [((a + b) >> ((j + replicate) % (width + 1))) & 1 for j in range(m)]
        elif family == 2:
            out = [lut[j][(x >> j) & 63] ^ ((b >> (a % width)) & 1) for j in range(m)]
        elif family == 3:
            terms = [all((x >> v) & 1 for v in cube) for cube in cubes]
            out = [terms[j] or (terms[7] and terms[(j + replicate + 1) % 9]) for j in range(m)]
        elif family == 4:
            out = [((x & mask).bit_count() & 1) ^ int(a > b + j) for j, mask in enumerate(masks)]
        else:
            out = [(x & mask).bit_count() >= max(1, mask.bit_count() // 2) for mask in masks]
        rows.append(''.join(str(int(value)) for value in out))
    library = [('2 2','7 3','1 1'), ('4 2','2 1','1 2'), ('2 2','2 2','1 1')][replicate % 3]
    target = (40 + 2 * replicate) if family == 0 else (40 + 4 * (i % 4))
    (destination/f'case_{i+1:03}.txt').write_text('\n'.join([
        str(n), str(m), ' '.join(rows), *library, str(target), '6000', '131072'
    ]) + '\n')
