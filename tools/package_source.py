#!/usr/bin/env python3
"""Package code and reproducibility evidence, excluding course data and executables."""
from pathlib import Path
import argparse,zipfile
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'artifacts/ufug2106-project-current')
parser.add_argument('--evidence',type=Path,help='Additional evaluation directory inside the project')
parser.add_argument('--archive-name',default='UFUG2106_Project_Source.zip')
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
    files += [p for p in evidence.rglob('*') if p.is_file() and p.suffix in {'.json','.csv','.log'}]
archive=OUT/args.archive_name
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(set(files)):z.write(p,'UFUG2106_Project_Source/'+str(p.relative_to(ROOT)))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
print(archive)
