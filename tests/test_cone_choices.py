"""Directly verify mixed cones and both BDD representations on complete tables."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from check_circuit import verify


class ConeChoices(unittest.TestCase):
    def test_candidate_truth_tables(self):
        with tempfile.TemporaryDirectory() as folder:
            d = Path(folder)
            exe, inp, out = d/'candidates', d/'case.txt', d/'out'
            subprocess.run(['g++', '-O2', '-std=c++17', ROOT/'tests/cone_checks.cpp',
                            '-o', exe], check=True)
            for n, rows in [
                (2, ['0011', '0111', '0111', '1111']),
                (3, [''.join(str((f >> x) & 1) for f in range(256)) for x in range(8)]),
                (9, [''.join(str(((x >> k) ^ (x >> ((k+4) % 9)) ^
                                  ((x >> 8) & (x >> ((k+2) % 9)))) & 1)
                             for k in range(6)) for x in range(512)]),
            ]:
                with self.subTest(n=n):
                    inp.write_text(f'{n}\n{len(rows[0])}\n' + ' '.join(rows) +
                                   '\n1 1\n1 1\n1 1\n70\n')
                    subprocess.run([exe, inp, out], check=True)
                    self.assertTrue(verify(inp, d/"out-factors")['valid'])
                    self.assertTrue(verify(inp, d/"out-cofactors")["valid"])
                    if n == 2:
                        self.assertEqual(verify(inp, d/"out-timing")["delay"], 1)
                        self.assertEqual(verify(inp, d/"out-mixed")["area"], 3)
                        self.assertLess(verify(inp, d/"out-cuts")["area"], 6)
                    for expensive in (1, 2):
                        self.assertTrue(verify(inp, d/f'out-stream{expensive}')['valid'])
                    for regular in (0, 1):
                        self.assertTrue(verify(inp, d/f'out{regular}')['valid'])
                        self.assertTrue(verify(inp, d/f'out-table{regular}')['valid'])
