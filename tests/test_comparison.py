#!/usr/bin/env python3
import importlib.util,tempfile,unittest
from pathlib import Path
r=Path(__file__).resolve().parents[1];spec=importlib.util.spec_from_file_location('comparison',r/'tools/compare_stage_results.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class ComparisonTests(unittest.TestCase):
    def setUp(self):self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name)
    def tearDown(self):self.tmp.cleanup()
    def csv(self,name,rows):
        p=self.root/name;p.write_text('case,status,area,delay,runtime_ms,target_met\n'+rows);return p
    def test_failures_are_retained(self):
        a=self.csv('a.csv','x,PASS,10,5,1,yes\ny,FAIL,,,,\n')
        b=self.csv('b.csv','x,FAIL,,,,\ny,PASS,4,2,3,yes\n')
        s=m.compare([('stage1',a,b)])['summary'];self.assertEqual(s['case_count'],2);self.assertEqual(s['both_pass_count'],0);self.assertEqual(s['regressed_validity'],1);self.assertEqual(s['recovered_validity'],1)
    def test_stage_names_prevent_case_collision(self):
        a=self.csv('a.csv','case_001,PASS,0,0,1,yes\n');b=self.csv('b.csv','case_001,PASS,0,0,2,yes\n')
        self.assertEqual(m.compare([('stage1',a,b),('stage2',a,b)])['summary']['case_count'],2)
    def test_missing_case_rejected(self):
        a=self.csv('a.csv','x,PASS,1,1,1,yes\ny,PASS,1,1,1,yes\n');b=self.csv('b.csv','x,PASS,1,1,1,yes\n')
        with self.assertRaises(ValueError):m.compare([('s',a,b)])
    def test_duplicate_case_rejected(self):
        a=self.csv('a.csv','x,PASS,1,1,1,yes\nx,PASS,1,1,1,yes\n')
        with self.assertRaises(ValueError):m.read_csv(a)
    def test_nan_rejected(self):
        a=self.csv('a.csv','x,PASS,1,1,nan,yes\n')
        with self.assertRaises(ValueError):m.read_csv(a)
    def test_deltas(self):
        a=self.csv('a.csv','x,PASS,12,5,1,yes\n');b=self.csv('b.csv','x,PASS,8,6,3,no\n');r=m.compare([('s',a,b)])['cases'][0]
        self.assertEqual((r['area_delta'],r['delay_delta'],r['runtime_delta_ms']),(-4,1,2))
if __name__=='__main__':unittest.main()
