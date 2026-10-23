# Level 6 optimizer

Requires Python 3, Node.js and a C++17 compiler (macOS Command Line Tools include clang++). The native optimizer has no third-party Python dependencies.

Run all four datasets for two hours, using four workers and independently validated checkpoints every five minutes:

```sh
cd /Users/eduard/Desktop/CCC
./optimizer/run.sh --input-dir "$HOME/Downloads/level-6" --output-dir output/level-6-local --seconds 7200 --workers 4 --checkpoint-every 300 --resume
```

`--output-dir` starts with the best supplied outputs in `output/level-6` when the directory is new. Subsequent runs reuse its per-case incumbents. Do not run two optimizers against the same output directory simultaneously.

Continue from the current best results while skipping small:

```sh
./optimizer/run.sh --output-dir output/level-6 --seconds 7200 --workers 4 --checkpoint-every 300 --resume --skip-small
```

`--skip-small` schedules only medium, large and enormous. Existing small programs remain in the four output files and are validated in checkpoints. `--resume` keeps every `work/*/best.txt` and resumes unfinished search rounds from `work/search-state.json`; older runs are recovered from completed search logs. A fresh run without `--resume` restarts search scheduling while still retaining its best programs. Checkpoint names are unique to each run, so previous snapshots remain available.

Results:

- Four `out_level-6_*.txt` files in the selected output directory, ready to upload.
- `progress.md`: scores of validated checkpoints.
- `checkpoints/`: immutable complete intermediate outputs and JSON score summaries.
- `work/`: per-case incumbent programs and search logs, used to resume.

The official sandbox defaults to `$HOME/Downloads/level-6/sandbox-level-6.html`; override `FAB_SANDBOX` when stored elsewhere. Set `FAB_NODE` to a Node.js executable if `node` is not on PATH.

To verify a result manually:

```sh
node optimizer/verify.cjs "$HOME/Downloads/level-6/in_level-6_1-small.txt" output/level-6-local/out_level-6_1-small.txt
```

The score is the number of nonempty instruction lines, excluding blank separators. Every published checkpoint is validated by the supplied sandbox simulator. Search is heuristic; a smaller score is not a proof of global optimality.
