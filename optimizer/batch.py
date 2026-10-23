#!/usr/bin/env python3
"""Prepare cases, validate improvements, and publish immutable score checkpoints."""
import argparse,json,os,shutil,subprocess,time
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
SOURCE=Path('/Users/eduard/Downloads/level-6')
OUT=ROOT/'output/level-6'
WORK=OUT/'work'
NAMES=['1-small','2-medium','3-large','4-enormous']
NODE=os.environ.get('FAB_NODE') or shutil.which('node') or '/Users/eduard/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node'
def prepare():
 WORK.mkdir(parents=True,exist_ok=True)
 meta=[]
 for name in NAMES:
  src=SOURCE/f'in_level-6_{name}.txt';tok=iter(src.read_text().split());n=int(next(tok))
  original=(OUT/f'out_level-6_{name}.txt').read_text().strip().split('\n\n')
  for i in range(n):
   r,rows,cols,count=[int(next(tok)) for _ in range(4)];board=[next(tok) for _ in range(rows)]
   key=f'{name}_{i+1:03d}';folder=WORK/key;folder.mkdir(exist_ok=True)
   (folder/'input.txt').write_text(f'1\n{r} {rows} {cols} {count}\n'+'\n'.join(board)+'\n')
   if not (folder/'best.txt').exists():(folder/'best.txt').write_text(original[i]+'\n')
   baseline=len(original[i].splitlines())
   meta.append(dict(key=key,set=name,index=i+1,radius=r,count=count,cells=sum(s.count('M') for s in board),baseline=baseline))
 (WORK/'cases.json').write_text(json.dumps(meta,indent=2))
 return meta

def checkpoint(label):
 meta=json.loads((WORK/'cases.json').read_text());stage=OUT/'checkpoints'/label
 stage.mkdir(parents=True,exist_ok=True);scores={};checks=[]
 for name in NAMES:
  cases=[c for c in meta if c['set']==name];programs=[(WORK/c['key']/'best.txt').read_text().strip() for c in cases]
  dest=stage/f'out_level-6_{name}.txt';dest.write_text('\n\n'.join(programs)+'\n\n')
  scores[name]=sum(len(p.splitlines()) for p in programs)
  checks.extend([str(SOURCE/f'in_level-6_{name}.txt'),str(dest)])
 subprocess.run([NODE,str(ROOT/'optimizer/verify.cjs'),*checks],check=True,capture_output=True,text=True)
 prior=[]
 for p in (OUT/'checkpoints').glob('*/score.json'):
  if p.parent!=stage:prior.append(json.loads(p.read_text()))
 baseline={name:sum(c['baseline'] for c in meta if c['set']==name) for name in NAMES}
 summary=dict(label=label,utc=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),scores=scores,total=sum(scores.values()),baseline=baseline,baseline_total=sum(baseline.values()),verified=True,improved=sum(len((WORK/c['key']/'best.txt').read_text().splitlines())<c['baseline'] for c in meta))
 (stage/'score.json').write_text(json.dumps(summary,indent=2))
 for name in NAMES:shutil.copyfile(stage/f'out_level-6_{name}.txt',OUT/f'out_level-6_{name}.txt')
 lines=['# Level 6 optimization checkpoints','', '|Checkpoint|Small|Medium|Large|Enormous|Total|Reduction|','|---|---:|---:|---:|---:|---:|---:|']
 for s in sorted(prior+[summary],key=lambda s:s['utc']):lines.append('|'+s['label']+'|'+'|'.join(str(s['scores'][n]) for n in NAMES)+f"|{s['total']}|{s['baseline_total']-s['total']}|")
 (OUT/'progress.md').write_text('\n'.join(lines)+'\n')
 print(json.dumps(summary),flush=True)
 return summary
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('command',choices=['prepare','checkpoint']);ap.add_argument('label',nargs='?',default='initial');a=ap.parse_args()
 if a.command=='prepare':prepare()
 else:checkpoint(a.label)
