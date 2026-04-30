# Autonomous Optimization Program

You are optimizing a C limit order book matching engine derived from the
QuantCup price-time matching engine.

## Goal

Minimize `MEDIAN_SCORE` reported by:

```sh
python3 evaluate.py
```

The score is elapsed benchmark time in milliseconds. Lower is better.

## Files

You may edit:

- `engine.c`
- `engine.h`
- `Makefile`
- `README.md`
- `program.md`

You must not edit:

- `test.c`
- `score.c`
- `evaluate.py`

Those files define the correctness and performance harness. Changing them makes
the result invalid.

## Loop

1. Run `python3 evaluate.py` and record the current `MEDIAN_SCORE`.
2. Propose exactly one optimization hypothesis.
3. Make the smallest code change that tests that hypothesis.
4. Run `python3 evaluate.py`.
5. If build or tests fail, fix the issue. After three failed repair attempts,
   discard the experiment.
6. If tests pass and `MEDIAN_SCORE` is clearly lower than the best baseline,
   keep the change and commit it.
7. If tests pass but the score is equal, noisy, or slower, discard the change.
8. Append a row to the experiment log.

## Keep Criteria

Keep only changes that improve median score beyond benchmark noise. For this
local harness, require at least a 0.5% improvement unless the result is repeated
and stable.

## Experiment Log

| Iter | Hypothesis / Change | Tests | MEDIAN_SCORE | Decision | Notes |
| :--: | :-- | :--: | :--: | :--: | :-- |
| 0 | Baseline run of cleaned flat-array order book | Pass | 6.66 ms | Keep | Initial benchmark on local harness, 15 runs |
| 1 | Cache `bookEntry->size` and skip cancelled zero-size entries before trade reporting | Pass | 6.99 ms | Discard | Slower on local harness; extra branch outweighed reduced loads/calls |
| 2 | Remove bounds branch in `cancel()` to restore single-store cancel fast path | Pass | 6.53 ms | Keep | Valid harness order ids make the branch unnecessary in the hot cancel path |
| 3 | Add `-march=native` to compiler flags | Pass | 6.62 ms | Discard | Slower than current best; compiler target tuning did not help this harness |
| 4 | Force inline `ppInsertOrder` and `EXECUTE_TRADE` | Pass | 6.49 ms | Keep | Repeated run improved from 6.52 to 6.49; small but stable enough to keep |
| 5 | Replace manually unrolled 4-byte field copy with fixed-size `memcpy` | Pass | 6.08 ms | Keep | Three runs gave 6.18/6.46/6.08; compiler-generated fixed copy beats manual byte stores |
| 6 | Store resting trader as 4 bytes without NUL terminator | Pass | 6.15 ms | Discard | Less insertion work was slower overall; reverted to field-length storage |
| 7 | Add branch prediction hints to side and fill-size comparisons | Pass | 8.29 ms | Discard | Mispredict-prone on this random-side feed; branch hints hurt badly |
| 8 | Enable link-time optimization with `-flto` in compile and link flags | Pass | 4.93 ms | Keep | Repeated run improved 5.12 -> 4.93; cross-file optimization is a large win |
| 9 | Replace `-O3` with `-Ofast` while keeping LTO | Pass | 4.93 ms | Discard | Tied current best; no clear signal worth keeping |
| 10 | Add `-fomit-frame-pointer -fno-stack-protector` with LTO | Pass | 5.10 ms | Discard | Slower than current best; reverted compiler flag change |
| 11 | Cache incoming order `symbol` and `trader` pointers in `limit()` | Pass | 5.12 ms | Discard | LTO already optimizes field address calculations; manual caching hurt register allocation |
| 12 | Align global `pricePoints` and `arenaBookEntries` arrays to 64-byte cache lines | Pass | 4.87 ms | Keep | Repeated run 4.87/4.88; cheap alignment improved locality slightly |
