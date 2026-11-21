"""Integration checks for optional-search admission and the two-path CLI."""
import importlib.util
import random
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('stage2_checker', ROOT / 'tools/check_circuit.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


class Stage2Resources(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='stage2-resources-')
        cls.directory = Path(cls.tmp.name)
        cls.binary = cls.directory / 'synth'
        subprocess.run(['g++', '-O2', '-std=c++17', str(ROOT / 'src/synth_stage2.cpp'),
                        '-o', str(cls.binary)], check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_case(self, limits):
        inp, out = self.directory / 'input.txt', self.directory / 'output.txt'
        # Shared/complemented outputs, constants and an asymmetric function.
        rows = [''.join(map(str, (i.bit_count() % 2, 1 - i.bit_count() % 2,
                                int(i > 4), 0, 1))) for i in range(16)]
        inp.write_text('4\n5\n' + ' '.join(rows) + '\n2 2\n3 3\n1 1\n100\n' + limits)
        proc = subprocess.run([str(self.binary), str(inp), str(out)],
                              capture_output=True, text=True, timeout=10, check=True)
        checker.verify(inp, out)
        return proc.stderr, out.read_text()

    def test_buffered_output_write_failure(self):
        proc = subprocess.run([self.binary, ROOT / 'examples/shared_outputs.txt', '/dev/full'],
                              capture_output=True, text=True)
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn('output write failed', proc.stderr)

    def test_omitted_and_zero_memory_limits(self):
        a, circuit_a = self.run_case('')
        b, circuit_b = self.run_case('0\n0\n')
        self.assertIn('ram_search_stopped=0', a)
        self.assertIn('ram_search_stopped=0', b)
        self.assertEqual(circuit_a, circuit_b)

    def test_insufficient_search_headroom_keeps_first_valid_circuit(self):
        # Intentionally below process overhead: this tests search scheduling,
        # not a promise to execute the mandatory first circuit in 1 KiB.
        diagnostic, _ = self.run_case('0\n1\n')
        self.assertIn('candidates=1 ', diagnostic)
        self.assertIn('ram_search_stopped=1', diagnostic)

    def test_generous_memory_budget_allows_search(self):
        diagnostic, _ = self.run_case('0\n131072\n')
        self.assertIn('ram_search_stopped=0', diagnostic)
        self.assertNotIn('candidates=1 ', diagnostic)

    def test_row_buffers_stay_within_small_ram_limit(self):
        inp, out = self.directory / 'large.txt', self.directory / 'large.out'
        peak = self.directory / 'peak-kib'
        for pair, expected in [('0 1', '20'), ('1 0', '21 NOT 20\n21')]:
            with self.subTest(pair=pair):
                rows = ' '.join([pair] * (1 << 19))
                inp.write_text(f'20\n1\n{rows}\n1 1\n1 1\n1 1\n10\n0\n9000\n')
                result = subprocess.run(['/usr/bin/time', '-f', '%M', '-o', str(peak),
                                         str(self.binary), str(inp), str(out)],
                                        capture_output=True, text=True, check=True, timeout=15)
                self.assertLessEqual(int(peak.read_text()), 9000)
                self.assertEqual(out.read_text().strip(), expected)
                if pair == '0 1':
                    self.assertIn('candidates=1 ', result.stderr)
                else:
                    # The inverter depends on just one input, so every order fits.
                    self.assertIn('ram_search_stopped=0', result.stderr)

    def test_full_stress_table_under_64_mib_address_space_limit(self):
        inp = self.directory / 'stress.txt'
        out = self.directory / 'stress.out'
        subprocess.run([sys.executable, str(ROOT / 'tools/generate_stress.py'), str(inp),
                        '--ram-kib', '65536'], check=True)
        subprocess.run(['prlimit', '--as=67108864', '--', str(self.binary), str(inp), str(out)],
                       check=True, capture_output=True, timeout=10)
        result = checker.verify(inp, out)
        self.assertEqual(result['assignments'], 1 << 20)
        self.assertTrue(result['target_met'])

    def test_dense_table_streams_under_hard_memory_limit(self):
        case, out = self.directory / 'dense.txt', self.directory / 'dense.out'
        rng = random.Random(210698)
        rows = ' '.join(format(rng.getrandbits(4), '04b') for _ in range(1 << 16))
        case.write_text(f'16\n4\n{rows}\n2 1\n3 2\n1 1\n100\n10000\n16384\n')
        result = subprocess.run(['prlimit', '--as=16777216', '--', self.binary, case, out],
                                text=True, capture_output=True, check=True, timeout=10)
        self.assertIn('streaming=1', result.stderr)
        score = checker.verify(case, out)
        self.assertEqual(score['assignments'], 1 << 16)
        self.assertTrue(score['target_met'])
        self.assertIn(f"area={score['area']} delay={score['delay']}", result.stderr)

    def test_full_support_skips_oversized_optional_buffer(self):
        inp, out = self.directory / 'parity.txt', self.directory / 'parity.out'
        rows = ' '.join('1' if i.bit_count() % 2 else '0' for i in range(1 << 20))
        inp.write_text(f'20\n1\n{rows}\n1 1\n1 1\n1 1\n100\n0\n9000\n')
        result = subprocess.run([str(self.binary), str(inp), str(out)],
                                capture_output=True, text=True, check=True, timeout=10)
        self.assertIn('ram_search_stopped=1', result.stderr)
        self.assertTrue(checker.verify(inp, out)['valid'])

    def test_large_launcher_does_not_consume_child_search_budget(self):
        self.run_case('0\n32768\n')
        # Fork from a parent with touched memory well above the child's limit.
        # The child's own RSS is small after exec; inherited ru_maxrss is not.
        wrapper = """
import os, sys
memory = bytearray(64 * 1024 * 1024)
pid = os.fork()
if pid == 0:
    os.execv(sys.argv[1], sys.argv[1:])
_, status = os.waitpid(pid, 0)
sys.exit(os.waitstatus_to_exitcode(status))
"""
        proc = subprocess.run([sys.executable, '-c', wrapper, str(self.binary),
                               str(self.directory / 'input.txt'),
                               str(self.directory / 'output.txt')],
                              capture_output=True, text=True, timeout=10, check=True)
        self.assertIn('ram_search_stopped=0', proc.stderr)
        self.assertNotIn('candidates=1 ', proc.stderr)
        checker.verify(self.directory / 'input.txt', self.directory / 'output.txt')


if __name__ == '__main__':
    unittest.main()
