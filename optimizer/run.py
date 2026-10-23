#!/usr/bin/env python3
"""Resume per-case searches and publish validated batch checkpoints."""
import argparse
import concurrent.futures
import json
import shutil
import subprocess
import time
from pathlib import Path

import batch


def load_completed(work, resume):
    if not resume:
        return {}
    path = work / 'search-state.json'
    if path.exists():
        return json.loads(path.read_text()).get('completed_rounds', {})
    # Recover progress from runs made before search-state.json was introduced.
    completed = {}
    for log in (work / 'logs').glob('*.log'):
        key, separator, number = log.stem.rpartition('-')
        if separator and number.isdigit() and 'beam final ' in log.read_text():
            completed[key] = max(completed.get(key, -1), int(number))
    return completed


def pending_cases(meta, completed, skip_small):
    selected = [c for c in meta if not (skip_small and c['set'] == '1-small')]
    if not selected:
        raise ValueError('No datasets selected for optimization.')
    roundno = min(completed.get(c['key'], -1) + 1 for c in selected)
    return roundno, [c for c in selected if completed.get(c['key'], -1) < roundno]


def save_completed(work, completed):
    path = work / 'search-state.json'
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps({'version': 1, 'completed_rounds': completed}, indent=2))
    temporary.replace(path)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--seconds', type=float, default=6000)
    ap.add_argument('--workers', type=int, default=4)
    ap.add_argument('--checkpoint-every', type=float, default=300)
    ap.add_argument('--resume', action='store_true', help='Resume best programs and completed search rounds.')
    ap.add_argument('--skip-small', action='store_true', help='Optimize medium, large and enormous only; retain small in checkpoints.')
    ap.add_argument('--input-dir', type=Path, default=batch.SOURCE)
    ap.add_argument('--output-dir', type=Path, default=batch.OUT)
    ap.add_argument('--binary', type=Path, default=Path('/private/tmp/ccc-beam'))
    args = ap.parse_args(argv)
    if args.seconds < 0 or args.workers < 1 or args.checkpoint_every <= 0:
        ap.error('seconds must be nonnegative; workers and checkpoint-every must be positive.')
    batch.SOURCE = args.input_dir.resolve()
    batch.OUT = args.output_dir.resolve()
    batch.WORK = batch.OUT / 'work'
    if not (batch.WORK / 'cases.json').exists():
        batch.OUT.mkdir(parents=True, exist_ok=True)
        for name in batch.NAMES:
            dest = batch.OUT / f'out_level-6_{name}.txt'
            if not dest.exists():
                shutil.copyfile(batch.ROOT / 'output/level-6' / dest.name, dest)
        batch.prepare()

    start = time.monotonic()
    end = start + args.seconds
    next_checkpoint = start + args.checkpoint_every
    stamp = time.strftime('%Y%m%dT%H%M%SZ', time.gmtime())
    meta = json.loads((batch.WORK / 'cases.json').read_text())
    logdir = batch.WORK / 'logs'
    logdir.mkdir(exist_ok=True)
    completed = load_completed(batch.WORK, args.resume)
    save_completed(batch.WORK, completed)
    first_round, selected = pending_cases(meta, completed, args.skip_small)
    print(json.dumps(dict(resume=args.resume, skip_small=args.skip_small,
                          round=first_round, pending_cases=len(selected),
                          output_dir=str(batch.OUT))), flush=True)

    def size(case):
        return len((batch.WORK / case['key'] / 'best.txt').read_text().splitlines())

    while time.monotonic() < end:
        roundno, cases = pending_cases(meta, completed, args.skip_small)
        seconds = [2, 4, 8, 15][min(roundno, 3)]
        width = [24, 32, 64, 128][min(roundno, 3)]
        cases.sort(key=size, reverse=True)

        def run(case):
            folder = batch.WORK / case['key']
            before = size(case)
            with (logdir / (case['key'] + f'-{roundno}.log')).open('w') as log:
                result = subprocess.run([
                    str(args.binary), str(folder / 'input.txt'), str(folder / 'best.txt'),
                    '--seconds', str(min(seconds, max(.1, end - time.monotonic()))),
                    '--width', str(width), '--seed', str(42 + roundno * 10007 + case['index'])
                ], stderr=log)
            return case['key'], before, size(case), result.returncode

        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as pool:
            pending = {}
            iterator = iter(cases)
            for _ in range(args.workers):
                case = next(iterator, None)
                if case and time.monotonic() < end:
                    pending[pool.submit(run, case)] = case
            while pending:
                done, _ = concurrent.futures.wait(
                    pending, timeout=1, return_when=concurrent.futures.FIRST_COMPLETED)
                for future in done:
                    key, before, after, code = future.result()
                    del pending[future]
                    if code != 0:
                        # Leave the round pending so --resume retries failed work.
                        raise RuntimeError(f'Optimizer failed for {key} (exit {code}).')
                    completed[key] = roundno
                    save_completed(batch.WORK, completed)
                    if after < before:
                        print(json.dumps(dict(case=key, before=before, after=after, round=roundno)), flush=True)
                    case = next(iterator, None)
                    if case and time.monotonic() < end:
                        pending[pool.submit(run, case)] = case
                if time.monotonic() >= next_checkpoint:
                    label = f'{stamp}-{int((time.monotonic() - start) // 60):04d}m-round{roundno}'
                    try:
                        batch.checkpoint(label)
                    except Exception as error:
                        print('CHECKPOINT FAILED ' + str(error), flush=True)
                    next_checkpoint = time.monotonic() + args.checkpoint_every
        round_finished = all(completed.get(c['key'], -1) >= roundno for c in cases)
        if round_finished:
            print(json.dumps(dict(round_finished=roundno + 1, elapsed=time.monotonic() - start)), flush=True)
            try:
                batch.checkpoint(f'{stamp}-round-{roundno + 1:02d}')
            except Exception as error:
                print('CHECKPOINT FAILED ' + str(error), flush=True)
    batch.checkpoint(f'{stamp}-final-run')


if __name__ == '__main__':
    main()
