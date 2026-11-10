#!/usr/bin/env python3
"""Package code and reproducibility evidence, excluding course data and executables."""
from pathlib import Path
import argparse,zipfile,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT.parents[1]/'artifacts/ufug2106-project-20261108')
parser.add_argument('--evidence',type=Path,help='Additional evaluation directory inside the project')
args=parser.parse_args()
OUT=args.output
OUT.mkdir(parents=True,exist_ok=True)
files=[]
for directory in ['src','tools','tests','docs','releases','examples']:
    files.extend(p for p in (ROOT/directory).rglob('*') if p.is_file() and '__pycache__' not in p.parts)
files += [ROOT/'Makefile',ROOT/'README.md',ROOT/'.clang-format']
files += [ROOT/'results/baseline.csv',ROOT/'results/cap250.csv',ROOT/'results/cap500.csv']
files += [ROOT/'results/synth_cap250.cpp',ROOT/'results/synth_cap500.cpp']
files += [p for p in (ROOT/'results/final').iterdir() if p.is_file() and p.suffix in {'.json','.csv','.log'}]
if args.evidence:
    evidence=args.evidence.resolve()
    evidence.relative_to(ROOT.resolve())
    files += [p for p in evidence.iterdir() if p.is_file() and p.suffix in {'.json','.csv','.log'}]
manifest={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(set(files))}
archive=OUT/'UFUG2106_Project_Source_AI_Assisted.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(set(files)):z.write(p,'UFUG2106_Project_Source/'+str(p.relative_to(ROOT)))
    z.writestr('UFUG2106_Project_Source/SHA256SUMS.json',json.dumps(manifest,indent=2))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    for name,h in manifest.items():assert hashlib.sha256(z.read('UFUG2106_Project_Source/'+name)).hexdigest()==h
print(archive)
