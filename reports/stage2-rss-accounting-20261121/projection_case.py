#!/usr/bin/env python3
"""Generate a projection table, or find and verify a shared input wire on every row."""
import re
import sys
from pathlib import Path


def generate(path, outputs):
    pair = '0' * outputs + ' ' + '1' * outputs + ' '
    with path.open('w') as out:
        out.write(f'20\n{outputs}\n')
        for _ in range(1024):
            out.write(pair * 512)
        out.write('\n2 2\n3 3\n1 1\n100\n10000\n65536\n')


def synthesize(source, dest):
    with source.open('rb') as inp:
        n, m = int(inp.readline()), int(inp.readline())
        rows = re.finditer(rb'\S+', inp.read())
        zeros, ones = b'0' * m, b'1' * m
        first = 0
        for i in range(1 << n):
            row = next(rows).group()
            if not first and row == ones:
                if i == 0 or i & (i - 1):
                    raise ValueError('expected a positive projection')
                first = i
            if row != (ones if i & first else zeros):
                raise ValueError('outputs differ from the proposed input wire')
    if not first:
        raise ValueError('no positive input projection found')
    wire = n - (first.bit_length() - 1)
    dest.write_text(' '.join([str(wire)] * m) + '\n')


if __name__ == '__main__':
    if sys.argv[1] == '--generate':
        generate(Path(sys.argv[2]), int(sys.argv[3]) if len(sys.argv) == 4 else 8)
    else:
        synthesize(Path(sys.argv[1]), Path(sys.argv[2]))
