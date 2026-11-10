#!/usr/bin/env python3
"""Independent generated-case regression; all truth-table rows, not sampled outputs."""
import importlib.util, json, random, subprocess, sys, tempfile, hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('checker',ROOT/'tools/check_circuit.py');checker=importlib.util.module_from_spec(spec);spec.loader.exec_module(checker)

def write_case(path,n,rows,costs=((2,2),(8,3),(1,1)),target=200,limit=10000,ram=0):
    m=len(rows[0]);path.write_text(f'{n}\n{m}\n'+ ' '.join(rows)+'\n'+'\n'.join(f'{a} {d}' for a,d in costs)+f'\n{target}\n{limit}\n{ram}\n')

def run(binary):
    rng=random.Random(21062026);records=[]
    with tempfile.TemporaryDirectory(prefix='ufug2106-regression-') as tmp:
        d=Path(tmp);cases=[]
        # In one multi-output circuit, every Boolean function of n<=3 is present.
        for n in (1,2,3):
            functions=1<<(1<<n)
            rows=[''.join(str((f>>i)&1) for f in range(functions)) for i in range(1<<n)]
            for costs in (((2,2),(8,3),(1,1)),((8,3),(2,2),(1,1)),((2,2),(2,3),(4,2))):
                cases.append((f'all_functions_n{n}_{len(cases)}',n,rows,costs))
        for k in range(80):
            n=rng.randrange(1,9);m=rng.randrange(1,10)
            rows=[''.join(str(rng.getrandbits(1)) for _ in range(m)) for _ in range(1<<n)]
            costs=tuple((rng.randrange(1,12),rng.randrange(1,6)) for _ in range(3))
            cases.append((f'random_{k:03}',n,rows,costs))
        # Diverse structures and constants, including negated/shared outputs.
        for n in (4,8,12):
            rows=[]
            for i in range(1<<n):
                parity=i.bit_count()%2;majority=int(i.bit_count()>n//2)
                vals=[0,1,(i>>(n-1))&1,i&1,parity,1-parity,majority,majority,int(i==0),int(i==(1<<n)-1)]
                rows.append(''.join(map(str,vals)))
            cases.append((f'structured_n{n}',n,rows,((3,1),(1,4),(1,1))))
        for name,n,rows,costs in cases:
            inp=d/(name+'.txt');out=d/(name+'.out');write_case(inp,n,rows,costs)
            result=subprocess.run([str(binary),str(inp),str(out)],capture_output=True,text=True,timeout=15)
            if result.returncode:raise AssertionError(f'{name}: {result.stderr}')
            record=checker.verify(inp,out);record['test']=name;records.append(record)
        # Validate the independent checker itself using known-correct and corrupt outputs.
        inp=d/'oracle.txt';write_case(inp,1,['0','1'])
        good=d/'good.out';good.write_text('1\n');assert checker.verify(inp,good)['area']==0
        mutations=['0\n','3 NOT 1\n3\n','2 NOT 2\n2\n','2 XOR 1 0\n2\n','1 0\n','2 NOT 1 0\n2\n']
        rejected=0
        for text in mutations:
            bad=d/'bad.out';bad.write_text(text)
            try:checker.verify(inp,bad)
            except (ValueError,IndexError):rejected+=1
        assert rejected==len(mutations)
    return {'binary':str(binary),'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'generated_cases':len(records),'all_passed':True,'checker_mutations_rejected':rejected,'records':records}

if __name__=='__main__':
    binary=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else ROOT/'build/synth'
    result=run(binary)
    target=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/'results/regression.json'
    target.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2))
