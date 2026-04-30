# druska/engine.c AI Optimizer

This project packages optimization work for `druska/engine.c` into a small
autonomous performance-iteration sandbox.

## What Is Included

- `engine.c`: optimized C implementation of `druska/engine.c`.
- `test.c`: correctness tests for crossing, FIFO priority, partial fills, and
  cancellation.
- `score.c`: deterministic benchmark feed.
- `evaluate.py`: build, test, benchmark, and score collector.
- `program.md`: instructions for an AI agent to run keep/revert optimization
  experiments.

## Run

```sh
make
./test
./score
python3 evaluate.py
python3 plot_progress.py
```

`MEDIAN_SCORE` is the optimization target. Lower is better.
`plot_progress.py` reads the experiment log in `program.md` and writes
`log.png`.

## Autonomous Optimization Flow

The safe workflow is:

```sh
git init
git add .
git commit -m "Initial druska engine.c optimizer baseline"
python3 evaluate.py
```

Then follow `program.md`: one hypothesis per iteration, benchmark with
`evaluate.py`, keep only statistically meaningful improvements, and discard
failed or slower changes.
