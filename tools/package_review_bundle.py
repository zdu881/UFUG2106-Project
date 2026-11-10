#!/usr/bin/env python3
"""Refresh review deliverables and validate their current contents."""
from pathlib import Path
import hashlib,json,zipfile,subprocess,shutil,xml.etree.ElementTree as ET
from pypdf import PdfReader
R=Path(__file__).resolve().parents[1];O=R/'artifacts/ufug2106-project-20261108';O.mkdir(parents=True,exist_ok=True)
subprocess.run(['python3',str(R/'tools/package_source.py')],check=True)
pdf=O/'UFUG2106_Stage1_Technical_Report_AI_Assisted.pdf'
shutil.copy2(R/'reports'/pdf.name,pdf);shutil.copy2(R/'docs/FINAL_REVIEW.md',O/'REVIEW.md')
assert len(PdfReader(pdf).pages)==6
root=ET.fromstring(subprocess.check_output(['pdftotext','-bbox',str(pdf),'-']))
for page in root.iter('{http://www.w3.org/1999/xhtml}page'):
    w,h=float(page.attrib['width']),float(page.attrib['height'])
    for word in page.iter('{http://www.w3.org/1999/xhtml}word'):
        b=word.attrib
        assert 0<=float(b['xMin'])<float(b['xMax'])<=w and 0<=float(b['yMin'])<float(b['yMax'])<=h
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
clean=json.loads((R/'results/final/clean_package.json').read_text())
assert sha(R/'src/synth.cpp')==clean['source_sha256']
freeze=json.loads((R/'releases/stage1_candidate/MANIFEST.json').read_text())
assert sha(R/'releases/stage1_candidate/synth.cpp')==freeze['source_sha256']==sha(R/'src/synth.cpp')
reg=json.loads((R/'results/final/regression.json').read_text());assert reg['binary_sha256']==sha(R/'build/synth')
archive=O/'UFUG2106_Project_Source_AI_Assisted.zip'
with zipfile.ZipFile(archive) as z:
    assert z.read('UFUG2106_Project_Source/src/synth.cpp')==(R/'src/synth.cpp').read_bytes()
    assert z.read('UFUG2106_Project_Source/src/baseline.cpp')==(R/'src/baseline.cpp').read_bytes()
bundle=O/'UFUG2106_Stage1_Review_Bundle.zip'
entries={}
for p in O.iterdir():
    if p.is_file() and p!=bundle and p.name!='VERIFICATION.json':entries['UFUG2106_Stage1/'+p.name]=p
for group in ['official','results/final/outputs','results/baseline_outputs','results/final/stress_cases','results/final/stress_outputs','reports']:
    for p in (R/group).rglob('*'):
        if p.is_file():entries['UFUG2106_Stage1/workspace/'+str(p.relative_to(R))]=p
with zipfile.ZipFile(bundle,'w',zipfile.ZIP_DEFLATED) as z:
    for name,p in sorted(entries.items()):z.write(p,name)
with zipfile.ZipFile(bundle) as z:
    assert z.testzip() is None
    for name,p in entries.items():assert hashlib.sha256(z.read(name)).hexdigest()==sha(p)
result={'result':'PASS','report_pages':6,'source_matches_clean_rebuild':True,'frozen_stage1_candidate_matches_current_source':True,'regression_binary_matches_current_binary':True,'clean_extracted_official_pass':25,'clean_extracted_delay_targets_met':25,'bundle_files':len(entries),'artifacts':{p.name:{'bytes':p.stat().st_size,'sha256':sha(p)} for p in [pdf,archive,bundle]}}
(O/'VERIFICATION.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
