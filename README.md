# QuantCup AI Optimizer

This project packages the price-time matching engine you provided into a small
autonomous optimization sandbox.

## What Is Included

- `engine.c`: cleaned C implementation of the flat price-array order book.
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
```

`MEDIAN_SCORE` is the optimization target. Lower is better.

## Autonomous Optimization Flow

The safe workflow is:

```sh
git init
git add .
git commit -m "Initial order book optimizer baseline"
python3 evaluate.py
```

Then follow `program.md`: one hypothesis per iteration, benchmark with
`evaluate.py`, keep only statistically meaningful improvements, and discard
failed or slower changes.
