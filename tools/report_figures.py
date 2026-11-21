"""Vector explanations and a measured tradeoff plot for the Stage 1 report."""
import base64
import html
import json
import math
from pathlib import Path

from report_theme import INK, ACCENT, SECONDARY as TEAL, PANEL


def svg_start(width, height):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<defs><marker id="arrow" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto"><path d="M0,0 L7,3.5 L0,7" fill="{INK}"/></marker></defs>
<rect width="100%" height="100%" rx="10" fill="{PANEL}"/>'''


def label(x, y, text, size=17, color=INK, anchor='middle'):
    return f'<text x="{x}" y="{y}" text-anchor="{anchor}" font-family="Liberation Sans, sans-serif" font-size="{size}" fill="{color}">{html.escape(text)}</text>'


def box(x, y, w, h, lines, color=ACCENT):
    result = f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="white" stroke="{color}" stroke-width="2"/>'
    for i, text in enumerate(lines):
        result += label(x+w/2, y+h/2 + 6 + (i-(len(lines)-1)/2)*21, text)
    return result


def wire(points, arrow=True, dashed=False):
    dash = ' stroke-dasharray="6 4"' if dashed else ''
    result = f'<polyline points="{points}" fill="none" stroke="{INK}" stroke-width="2"{dash}/>'
    if arrow:
        coordinates = [tuple(map(float, p.split(','))) for p in points.split()]
        (x0,y0),(x,y) = coordinates[-2:]
        length = math.hypot(x-x0,y-y0)
        ux,uy = (x-x0)/length,(y-y0)/length
        bx,by=x-9*ux,y-9*uy
        result += f'<polygon points="{x},{y} {bx-4*uy},{by+4*ux} {bx+4*uy},{by-4*ux}" fill="{INK}"/>'
    return result


def pipeline():
    s = svg_start(1000, 270)
    s += label(500,26,'SEARCH FOR A CIRCUIT, THEN EMIT THE BEST COMPLETED RESULT',20,ACCENT)
    for x,w,lines in [(20,150,['Truth table','+ gate costs']),
                      (215,220,['First complete circuit','build / map / balance']),
                      (565,200,['Retained best circuit','replace only if better']),
                      (820,160,['Output file','AND2 / OR2 / NOT'])]:
        s += box(x,45,w,64,lines)
    for a,b in [(170,215),(435,565),(765,820)]:s += wire(f'{a},77 {b-6},77')
    s += label(500,65,'initialize',15)
    s += wire('325,109 325,153')
    s += label(475,134,'OPTIONAL SEARCH',16,ACCENT)
    for x,w,lines in [(215,170,['Trial order','build / reuse BDD']),
                      (425,165,['Map / balance','measure area + delay']),
                      (635,160,['Compare with best','using mapped cost'])]:
        s += box(x,158,w,52,lines)
    s += wire('385,184 419,184')+wire('590,184 629,184')
    s += wire('715,158 715,115')
    s += wire('715,210 715,234 300,234 300,216',dashed=True)
    s += label(500,228,'Repeat while budget remains',15)
    s += label(500,260,'Rank: delay excess first, then area, then delay. The first construction is not hard-bounded.',17,TEAL)
    return s+'</svg>'


# Shared data drives the rendered decision edges and the example verification.
SHANNON_TREE = {
    'a': ('a', 'b0', 'b1'),
    'b0': ('b', 'c00', 'c01'), 'b1': ('b', 'c10', 'c11'),
    'c00': ('c', 't0', 't0'), 'c01': ('c', 't0', 't1'),
    'c10': ('c', 't0', 't1'), 'c11': ('c', 't0', 't1'),
}
SHANNON_BDD = {'a': ('a', 'b', 'c'), 'b': ('b', 't0', 'c'),
               'c': ('c', 't0', 't1')}


def shannon():
    s = svg_start(1000, 365)
    s += label(500, 28, 'ONE FUNCTION, FOUR REPRESENTATIONS:  f = (a OR b) AND c', 21, ACCENT)
    for x, title in [(83,'1  Truth table'),(335,'2  Decision tree'),
                     (607,'3  Reduced BDD'),(871,'4  Legal gates')]:
        s += label(x, 64, title, 19, ACCENT)
    for x in [169,492,726]:
        s += f'<path d="M{x},80 V305" stroke="#CFDCE8" fill="none"/>'
    s += label(80, 99, 'a  b  c  |  f', 18)
    for row in range(8):
        a,b,c=(row>>2)&1,(row>>1)&1,row&1
        f=(a or b) and c
        if f:
            s += f'<rect x="22" y="{108+row*23}" width="116" height="22" rx="3" fill="#E8EFF7"/>'
        s += label(80, 125+row*23, f'{a}  {b}  {c}  |  {f}', 18)

    def node(x,y,name,shared=False,terminal=False):
        if terminal:
            return (f'<rect x="{x-11}" y="{y-11}" width="22" height="22" rx="3" fill="white" stroke="{INK}"/>'
                    +label(x,y+6,name,17))
        return (f'<circle cx="{x}" cy="{y}" r="17" fill="{"#E8EFF7" if shared else "white"}" stroke="{TEAL if shared else INK}" stroke-width="2"/>'
                +label(x,y+6,name,19))

    positions={'a':(335,106),'b0':(265,162),'b1':(405,162),
               'c00':(218,225),'c01':(296,225),'c10':(374,225),'c11':(452,225)}
    for key,(var,low,high) in SHANNON_TREE.items():
        x,y=positions[key]
        if var!='c':
            for bit,child in enumerate((low,high)):
                xx,yy=positions[child]
                s += wire(f'{x+(-8 if bit==0 else 8)},{y+15} {xx},{yy-20}',dashed=bit==0)
        else:
            if key!='c00':
                s += f'<rect x="{x-34}" y="203" width="68" height="92" rx="6" fill="#E8EFF7"/>'
            for bit,child in enumerate((low,high)):
                xx=x+(-21 if bit==0 else 21)
                s += wire(f'{x+(-7 if bit==0 else 7)},{y+16} {xx},266',dashed=bit==0)
                s += node(xx,280,child[-1],terminal=True)
        s += node(x,y,var,shared=key in ('c01','c10','c11'))
    s += label(335,317,'7 decision nodes',16,TEAL)

    positions={'a':(610,106),'b':(551,175),'c':(666,217),'t0':(543,280),'t1':(690,280)}
    for key,(var,low,high) in SHANNON_BDD.items():
        x,y=positions[key]
        for bit,child in enumerate((low,high)):
            xx,yy=positions[child]
            pad=14 if child.startswith('t') else 20
            s += wire(f'{x+(-8 if bit==0 else 8)},{y+15} {xx},{yy-pad}',dashed=bit==0)
    for key,(x,y) in positions.items():
        s += node(x,y,key[-1] if key.startswith('t') else key,
                  shared=key=='c',terminal=key.startswith('t'))
    s += label(607,317,'3 decision nodes',16,TEAL)

    # Logic symbols: the circuit implements the same function after factoring.
    s += label(757,120,'a',18)+label(757,156,'b',18)
    s += wire('769,115 805,115',arrow=False)+wire('769,151 805,151',arrow=False)
    s += f'<path d="M794,100 Q825,133 794,166 Q848,166 865,133 Q848,100 794,100 Z" fill="white" stroke="{ACCENT}" stroke-width="2"/>'
    s += label(832,139,'OR2',15)
    s += wire('865,133 885,133 885,176 909,176',arrow=False)
    s += label(854,225,'c',18)+wire('866,220 889,220 889,201 909,201',arrow=False)
    s += f'<path d="M909,160 H933 A29,29 0 0 1 933,218 H909 Z" fill="white" stroke="{ACCENT}" stroke-width="2"/>'
    s += label(935,195,'AND2',14)+wire('962,189 979,189')+label(990,195,'f',18)
    s += label(869,270,'s = a OR b',18)+label(869,294,'f = s AND c',18)
    s += label(869,317,'2 gates, same function',16,TEAL)
    s += wire('28,346 62,346',dashed=True)+label(70,352,'0 branch',16,anchor='start')
    s += wire('176,346 210,346')+label(218,352,'1 branch',16,anchor='start')
    s += label(639,352,'Reduction: remove c with equal children; merge the three matching c subtrees.',16)
    return s+'</svg>'


def logic_gate(x,y,op):
    """70 x 48 gate, input ports at (x,y+12/36), output at (x+70,y+24)."""
    if op=='OR2':
        d=f'M{x-10},{y} Q{x+10},{y+24} {x-10},{y+48} Q{x+48},{y+48} {x+70},{y+24} Q{x+48},{y} {x-10},{y} Z'
    else:
        d=f'M{x},{y} H{x+46} A24,24 0 0 1 {x+46},{y+48} H{x} Z'
    return f'<path d="{d}" fill="white" stroke="{TEAL if op=="OR2" else ACCENT}" stroke-width="2"/>'+label(x+33,y+30,op,16)


# Two explicit equivalent circuits for the diagram's before/after comparison.
SHARING_UNSHARED = '4 OR2 1 2\n5 OR2 1 2\n6 AND2 5 3\n4 6\n'
SHARING_SHARED = '4 OR2 1 2\n5 AND2 4 3\n4 5\n'


def shared_circuit():
    s=svg_start(1000,265)
    s+=label(500,26,'TWO OUTPUTS:  y0 = a OR b;  y1 = (a OR b) AND c',20,ACCENT)
    s+=label(240,58,'BEFORE: compute a OR b twice',19,ACCENT)
    s+=label(755,58,'AFTER: one shared result',19,ACCENT)
    s+='<path d="M500,70 V245" stroke="#CFDCE8"/>'
    for y in [84,150]:
        s+=f'<rect x="96" y="{y-6}" width="105" height="60" rx="7" fill="#E8EFF7"/>'
        s+=label(58,y+18,'a',18)+label(58,y+42,'b',18)
        s+=wire(f'72,{y+12} 110,{y+12}',arrow=False)+wire(f'72,{y+36} 110,{y+36}',arrow=False)
        s+=logic_gate(110,y,'OR2')
    s+=wire('180,108 414,108')+label(449,114,'y0',18)
    s+=wire('180,174 239,174 239,158 300,158',arrow=False)
    s+=label(237,190,'c',18)+wire('250,184 300,184',arrow=False)
    s+=logic_gate(300,146,'AND2')+wire('370,170 414,170')+label(449,176,'y1',18)
    s+=label(250,224,'Area = 8 + 8 + 2 = 18; delay = 5',18,TEAL)
    s+=label(250,248,'3 gates; duplicate OR gates highlighted',16)
    s+='<rect x="586" y="105" width="105" height="60" rx="7" fill="#E8EFF7"/>'
    s+=label(547,129,'a',18)+label(547,153,'b',18)
    s+=wire('561,123 600,123',arrow=False)+wire('561,147 600,147',arrow=False)
    s+=logic_gate(600,111,'OR2')
    s+=wire('670,135 723,135 723,97 925,97')+label(960,103,'y0',18)
    s+=f'<circle cx="723" cy="135" r="4" fill="{TEAL}"/>'
    s+=wire('723,135 723,158 810,158',arrow=False)
    s+=label(744,190,'c',18)+wire('757,184 810,184',arrow=False)
    s+=logic_gate(810,146,'AND2')+wire('880,170 925,170')+label(960,176,'y1',18)
    s+=label(750,224,'Area = 8 + 2 = 10; delay = 5',18,TEAL)
    s+=label(750,248,'2 gates; save one OR gate (8 area units)',16)
    return s+'</svg>'


def tradeoff(profiles, path):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10,'svg.fonttype':'none',
                         'text.color':INK,'axes.labelcolor':INK,'xtick.color':INK,'ytick.color':INK})
    # Common row ordering makes the three different metrics directly comparable.
    fig,axes=plt.subplots(1,3,figsize=(8.4,2.85),layout='constrained',
                          gridspec_kw={'width_ratios':[1.25,1.15,1]})
    fig.patch.set_facecolor(PANEL)
    names=['Baseline','250 ms cap','500 ms cap','1500 ms cap']
    values=[[p[3]/1e6 for p in profiles],[p[4]/1000 for p in profiles],[p[2] for p in profiles]]
    texts=[[f'{p[3]:,}' for p in profiles],[f'{p[4]/1000:.2f} s' for p in profiles],
           [f'{p[2]}/25' for p in profiles]]
    limits=[1.48,max(values[1])*1.35,34]
    titles=['Total circuit area','Total synthesis time','Delay targets met']
    units=['Millions of area units','Seconds across 25 cases','Cases (out of 25)']
    colors=[TEAL,TEAL,TEAL,ACCENT]
    for k,ax in enumerate(axes):
        ax.set_facecolor(PANEL)
        ax.barh(range(4),values[k],height=.5,color=colors)
        for j,value in enumerate(values[k]):
            ax.text(value+limits[k]*.025,j,texts[k][j],va='center',fontsize=9,color=INK)
        ax.set(ylim=(3.65,-.65),xlim=(0,limits[k]),xlabel=units[k])
        ax.set_title(titles[k],fontsize=11,fontweight='bold',loc='left',color=ACCENT)
        ax.set_yticks(range(4),names if k==0 else ['']*4)
        ax.tick_params(axis='y',length=0)
        ax.spines[['top','right','left']].set_visible(False)
        ax.set_axisbelow(True);ax.grid(axis='x',alpha=.15)
    axes[0].set_xticks([0,.5,1.0]);axes[1].set_xticks([0,10,20]);axes[2].set_xticks([0,12,25])
    fig.savefig(path,format='svg');plt.close(fig)

def figures(output, profiles):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    for name,content in [('pipeline',pipeline()),('shannon',shannon()),('sharing',shared_circuit())]:
        (output/(name+'.svg')).write_text(content)
    tradeoff(profiles,output/'tradeoff.svg')
    (output/'tradeoff-data.json').write_text(json.dumps([dict(zip(
        ['configuration','correct_cases','delay_targets_met','total_area','total_runtime_ms'], p))
        for p in profiles], indent=2)+'\n')
    captions={
        'pipeline':'Figure 1. A completed candidate initializes the retained circuit. Optional search compares actual mapped area and delay; stopping the search leaves the best completed result available.',
        'shannon':'Figure 2. Eight-row synthetic example, ordered a → b → c. Reduction preserves every output; node counts exclude terminals. Factoring yields two legal gates; BDD node count alone does not determine area or delay.',
        'sharing':'Figure 3. Synthetic before/after comparison. OR2 costs area 8 / delay 3; AND2 costs area 2 / delay 2. Both circuits have delay max(3, 3 + 2) = 5. Sharing removes one OR2 without changing either output.',
        'tradeoff':'Figure 4. Retained runs: all four configurations pass 25/25 correctness checks. Shorter area/time bars are preferable; more delay targets met is preferable. These are whole-search budgets, not component ablations or an official score.'}
    result={}
    for name,caption in captions.items():
        data=base64.b64encode((output/(name+'.svg')).read_bytes()).decode()
        result[name]=f'<figure><img src="data:image/svg+xml;base64,{data}" alt="{html.escape(caption)}"/><figcaption>{html.escape(caption)}</figcaption></figure>'
    return result
