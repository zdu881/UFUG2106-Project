#!/usr/bin/env python3
"""Refresh review deliverables and validate their current contents."""
from pathlib import Path
import argparse, json, shutil, subprocess, zipfile
import xml.etree.ElementTree as ET
from pypdf import PdfReader

R = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, default=R / 'artifacts/stage1-review-current')
parser.add_argument('--report', type=Path, default=R / 'reports/UFUG2106_Stage1_Technical_Report.pdf')
parser.add_argument('--evidence', type=Path, default=R / 'results/stage1-final-20261110')
args = parser.parse_args()
O = args.output
O.mkdir(parents=True, exist_ok=True)

source_cmd = ['python3', str(R / 'tools/package_source.py'), '--output', str(O), '--archive-name',
              'UFUG2106_Project_Source.zip']
if args.evidence.exists():
    source_cmd += ['--evidence', str(args.evidence)]
subprocess.run(source_cmd, check=True)

pdf = O / args.report.name
shutil.copy2(args.report, pdf)
shutil.copy2(R / 'docs/FINAL_REVIEW.md', O / 'REVIEW.md')
assert len(PdfReader(pdf).pages) == 6
root = ET.fromstring(subprocess.check_output(['pdftotext', '-bbox', str(pdf), '-']))
for page in root.iter('{http://www.w3.org/1999/xhtml}page'):
    w, h = float(page.attrib['width']), float(page.attrib['height'])
    for word in page.iter('{http://www.w3.org/1999/xhtml}word'):
        b = word.attrib
        assert 0 <= float(b['xMin']) < float(b['xMax']) <= w
        assert 0 <= float(b['yMin']) < float(b['yMax']) <= h

archive = O / 'UFUG2106_Project_Source.zip'
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert z.read('UFUG2106_Project_Source/src/synth.cpp') == (R / 'src/synth.cpp').read_bytes()
    assert z.read('UFUG2106_Project_Source/src/baseline.cpp') == (R / 'src/baseline.cpp').read_bytes()

bundle = O / 'UFUG2106_Stage1_Review_Bundle.zip'
entries = {}
for p in O.iterdir():
    if p.is_file() and p != bundle and p.name != 'VERIFICATION.json':
        entries['UFUG2106_Stage1/' + p.name] = p
for group in ['official', 'results/final/outputs', 'results/baseline_outputs',
              'results/final/stress_cases', 'results/final/stress_outputs', 'reports']:
    for p in (R / group).rglob('*'):
        if p.is_file():
            entries['UFUG2106_Stage1/workspace/' + str(p.relative_to(R))] = p
with zipfile.ZipFile(bundle, 'w', zipfile.ZIP_DEFLATED) as z:
    for name, p in sorted(entries.items()):
        z.write(p, name)
with zipfile.ZipFile(bundle) as z:
    assert z.testzip() is None

result = {'result': 'PASS', 'report_pages': 6, 'bundle_files': len(entries),
          'source_archive': archive.name, 'report': pdf.name,
          'note': 'Structural checks only; SHA-256 is not a workflow gate.'}
(O / 'VERIFICATION.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
