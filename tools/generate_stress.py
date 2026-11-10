#!/usr/bin/env python3
from pathlib import Path
import argparse
p=argparse.ArgumentParser();p.add_argument('output',type=Path);a=p.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
with a.output.open('w') as f:
    f.write('20\n8\n')
    for i in range(1<<20):
        vals=(0,1,(i>>19)&1,i&1,i.bit_count()%2,int(i.bit_count()>=10),int((i&1023)>=513),((0x9357>>(i&15))&1))
        f.write((' ' if i else '')+''.join(map(str,vals)))
    f.write('\n2 2\n3 3\n1 1\n100\n10000\n131072\n')
