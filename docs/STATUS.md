# Project status

Last verified: 2026-10-14. This is an educational research prototype, not a
production database index. The implementation and experiment baseline is
committed on `main` as `3080c5c`.

## Implemented

- Sorted, unique `uint64_t` dataset and exact binary search.
- In-memory B+ tree with bulk load, unique insertion, exact membership,
  inclusive range scans, and structural validation.
- Static learned index trained with one normalized least-squares linear
  model. The predicted location is corrected by exponential bracketing and
  binary search before an exact answer is returned.
- Optional piecewise-linear static learned index, with a configurable target
  segment size, segment routing, and exact local correction. It is benchmarked
  against the global model; it does not replace that baseline.
- Insert-only adaptive learned index with an immutable learned base, ordered
  delta, configurable rebuild threshold, synchronous merge/retraining, and
  `std::shared_mutex` reader/writer coordination.
- CMake targets for app, library, tests, benchmarks, learned-model rank-error
  reports, and a seeded query correction-distance diagnostic; raw benchmark
  output and per-trial records.

## Verification

On Windows 10.0.26200 with GCC 16.2.0 and Google Benchmark 1.9.1:

- `cmake --build build-release` completed successfully.
- `ctest --test-dir build-release --output-on-failure`: 47/47 tests passed.
- `build-release/cpp/learned_index_app.exe` ran and demonstrated all four
  index modes.
- `git diff --check` reported no whitespace errors.
- Trials 002–005 ran with five repetitions and saved raw CSV outputs in
  `docs/experiments/`.
- Trial 006 ran the rank-error report on 100K keys per shape and saved CSV
  output in `docs/experiments/`.
- Trial 007 compared hot-query lookups on 100K keys and saved raw output plus
  the interpretation and limitations in `docs/experiments/`.
- Trial 008 compared global and piecewise models on four 100K-key shapes,
  including rank-error distributions, existing-key and in-range-miss lookup,
  and training cost; both raw CSVs are saved in `docs/experiments/`.
- Trial 009 recorded signed rank residual and absolute distance from exact
  lower-bound ranks for 1,000 existing and 1,000 in-range-missing queries per
  shape and model.

These checks do not prove correctness for all inputs and do not establish
general performance rankings. No sanitizer or race detector was run.

## Known scope limits and next work

1. The global model remains a single linear regression, with an optional
   piecewise alternative using fixed-size segments. Initial evenly spaced,
   clustered, quadratic-gap, and one hot-query cases are recorded, but the
   segment-size trade-off, additional skew levels, hot-set sizes, and fresh-
   process repeats remain open.
2. Rank residual and absolute correction-distance distributions are reported
   for seeded query samples, but actual probe counts and lookup latency by
   correction distance still need direct measurement.
3. The adaptive policy uses a configured count threshold. It does not estimate
   workload changes or tune that threshold automatically. Trial 003 shows that
   synchronous rebuild cost can dominate an update batch.
4. Rebuild holds the exclusive lock, so readers pause during merge and model
   training. Concurrent correctness has a test, but concurrent throughput,
   latency under writes, and race-sanitizer coverage remain to be measured.
5. Updates support insertion only. Deletion, tombstones, persistence, crash
   recovery, B+ tree concurrency, and lock-free access are outside the current
   prototype.
6. The benchmark suite has point lookups, training, bulk load, inserts, range
   scans, and an initial hot-query case. It still needs memory accounting,
   sequential scans, read/write mixes, and repeatability analysis across fresh
   processes.

The raw measurements and the limitations of each run are recorded in the
[experiment log](experiments/README.md). The project should only be described
as complete for its current prototype scope; the broader research specification
remains in progress.
