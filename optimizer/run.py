#!/usr/bin/env python3
import argparse,concurrent.futures,json,subprocess,time
from pathlib import Path
import batch
ap=argparse.ArgumentParser();ap.add_argument('--seconds',type=int,default=6000);ap.add_argument('--workers',type=int,default=4);ap.add_argument('--checkpoint-every',type=int,default=300);ap.add_argument('--resume',action='store_true');ap.add_argument('--input-dir',type=Path,default=batch.SOURCE);ap.add_argument('--output-dir',type=Path,default=batch.OUT);ap.add_argument('--binary',type=Path,default=Path('/private/tmp/ccc-beam'));args=ap.parse_args()
batch.SOURCE=args.input_dir.resolve();batch.OUT=args.output_dir.resolve();batch.WORK=batch.OUT/'work'
if not (batch.WORK/'cases.json').exists():
 batch.OUT.mkdir(parents=True,exist_ok=True)
 for name in batch.NAMES:
  dest=batch.OUT/f'out_level-6_{name}.txt'
  if not dest.exists():
   import shutil
   shutil.copyfile(batch.ROOT/'output/level-6'/dest.name,dest)
 batch.prepare()
start=time.monotonic();end=start+args.seconds;next_checkpoint=start+args.checkpoint_every
meta=json.loads((batch.WORK/'cases.json').read_text())
logdir=batch.WORK/'logs';logdir.mkdir(exist_ok=True)
roundno=0
while time.monotonic()<end:
 seconds=[2,4,8,15][min(roundno,3)];width=[24,32,64,128][min(roundno,3)]
 def size(c):return len((batch.WORK/c['key']/'best.txt').read_text().splitlines())
 # Long programs first: the score rewards total improvement, not case counts.
 cases=sorted(meta,key=size,reverse=True)
 def run(c):
  folder=batch.WORK/c['key'];before=size(c)
  with (logdir/(c['key']+f'-{roundno}.log')).open('w') as f:
   result=subprocess.run([str(args.binary),str(folder/'input.txt'),str(folder/'best.txt'),'--seconds',str(min(seconds,max(.1,end-time.monotonic()))),'--width',str(width),'--seed',str(42+roundno*10007+c['index'])],stderr=f)
  return c['key'],before,size(c),result.returncode
 with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
  pending={};it=iter(cases)
  for _ in range(args.workers):
   c=next(it,None)
   if c:pending[pool.submit(run,c)]=c
  while pending:
   done,_=concurrent.futures.wait(pending,timeout=1,return_when=concurrent.futures.FIRST_COMPLETED)
   for f in done:
    key,before,after,code=f.result();del pending[f]
    if after<before:print(json.dumps(dict(case=key,before=before,after=after,round=roundno)),flush=True)
    c=next(it,None)
    if c and time.monotonic()<end:pending[pool.submit(run,c)]=c
   if time.monotonic()>=next_checkpoint:
    label=f'{int((time.monotonic()-start)//60):04d}m-round{roundno}'
    try:batch.checkpoint(label)
    except Exception as e:print('CHECKPOINT FAILED '+str(e),flush=True)
    next_checkpoint=time.monotonic()+args.checkpoint_every
 roundno+=1
 print(json.dumps(dict(round_finished=roundno,elapsed=time.monotonic()-start)),flush=True)
 try:batch.checkpoint(f'round-{roundno:02d}')
 except Exception as e:print('CHECKPOINT FAILED '+str(e),flush=True)
batch.checkpoint('final-run')
