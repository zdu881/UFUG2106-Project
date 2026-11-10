#!/usr/bin/env python3
"""Independent full truth-table circuit verifier, not using the synthesizer or official binary.
Python integer bit lanes evaluate blocks of assignments. Every output bit is checked.
"""
import argparse, json
from pathlib import Path
import numpy as np


def read_case(path):
    with Path(path).open() as f:
        n=int(f.readline());m=int(f.readline())
        tokens=f.readline().split()
        if len(tokens)!=1<<n or any(len(t)!=m or set(t)-{'0','1'} for t in tokens):
            raise ValueError('malformed truth table')
        truth=np.frombuffer(''.join(tokens).encode(),dtype=np.uint8).reshape(1<<n,m)-48
        costs={op:tuple(map(int,f.readline().split())) for op in ('AND2','OR2','NOT')}
        target=int(f.readline());limits=[int(s) for s in f.read().split()]
    return n,m,truth,costs,target,limits


def verify(case_path,circuit_path,block_bits=8192):
    n,m,truth,costs,target,limits=read_case(case_path)
    lines=[line.split() for line in Path(circuit_path).read_text().splitlines() if line.strip()]
    if not lines: raise ValueError('empty circuit')
    gates=[];arrival=[0]*(n+1);uses=[0]*(n+1);area=0
    for tokens in lines[:-1]:
        if len(tokens)<3:raise ValueError('short gate line')
        ident=int(tokens[0]);op=tokens[1]
        if ident!=n+1+len(gates):raise ValueError('non-consecutive ID')
        if op not in costs:raise ValueError('unsupported gate')
        fanins=list(map(int,tokens[2:]))
        if len(fanins)!=(1 if op=='NOT' else 2):raise ValueError('arity mismatch')
        if any(v<0 or v>=ident for v in fanins):raise ValueError('invalid fanin')
        gates.append((op,fanins));uses.append(0)
        for v in fanins:uses[v]+=1
        area+=costs[op][0];arrival.append(max(arrival[v] for v in fanins)+costs[op][1])
    outputs=list(map(int,lines[-1]))
    if len(outputs)!=m or any(v<0 or v>=len(arrival) for v in outputs):raise ValueError('invalid output list')
    for v in outputs:uses[v]+=1
    delay=max(arrival[v] for v in outputs)
    def packed(bits):return int.from_bytes(np.packbits(bits,bitorder='little').tobytes(),'little')
    for start in range(0,1<<n,block_bits):
        stop=min(start+block_bits,1<<n);mask=(1<<(stop-start))-1
        ids=np.arange(start,stop,dtype=np.uint32)
        values=[0]+[packed((ids>>(n-1-j))&1) for j in range(n)]+[None]*len(gates)
        remaining=uses.copy()
        for k,(op,fanins) in enumerate(gates,n+1):
            a=values[fanins[0]]
            if op=='NOT':v=a^mask
            elif op=='AND2':v=a&values[fanins[1]]
            else:v=a|values[fanins[1]]
            values[k]=v
            for u in fanins:
                remaining[u]-=1
                if not remaining[u]:values[u]=None
        for j,u in enumerate(outputs):
            expected=packed(truth[start:stop,j])
            diff=values[u]^expected
            if diff:
                row=start+((diff&-diff).bit_length()-1)
                raise ValueError(f'output mismatch at row {row}, output {j}')
    return dict(case=Path(case_path).name,n=n,m=m,assignments=1<<n,gates=len(gates),area=area,delay=delay,target=target,target_met=delay<=target,valid=True)


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('case');p.add_argument('circuit');p.add_argument('--json');a=p.parse_args()
    result=verify(a.case,a.circuit)
    data=json.dumps(result,indent=2);print(data)
    if a.json:Path(a.json).write_text(data+'\n')
