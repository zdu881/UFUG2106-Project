import sys
from pathlib import Path
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from evaluate_release import validate_cases


class ReleaseValidationTests(unittest.TestCase):
    def check_rows(self, rows, names):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            csv = root / 'official.csv'
            csv.write_text('case,status,area,delay,runtime_ms,target_met\n' + rows)
            return validate_cases(csv, [root / name for name in names])

    def test_missing_case_rejected(self):
        with self.assertRaises(ValueError):
            self.check_rows('a.txt,PASS,1,1,2,yes\n', ['a.txt', 'b.txt'])

    def test_duplicate_case_rejected(self):
        with self.assertRaises(ValueError):
            self.check_rows('a.txt,PASS,1,1,2,yes\n' * 2, ['a.txt'])

    def test_failed_case_rejected(self):
        with self.assertRaises(ValueError):
            self.check_rows('a.txt,FAIL,,,,\n', ['a.txt'])

    def test_delay_target_miss_is_not_correctness_failure(self):
        rows = self.check_rows('a.txt,PASS,1,12,2,no\n', ['a.txt'])
        self.assertFalse(rows['a.txt']['target_met'])

    def test_empty_case_set_rejected(self):
        with self.assertRaises(ValueError):
            self.check_rows('', [])


if __name__ == '__main__':
    unittest.main()
