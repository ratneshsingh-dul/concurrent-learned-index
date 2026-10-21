# Project status

Last verified: 2026-10-21. This is an educational research prototype, not a
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
- Static two-stage RMI with a learned root rank predictor, rank-leaf routing,
  local leaf models, configurable leaf size, and exact global correction.
  Initial correctness and one exploratory performance comparison are recorded.
- Insert-only adaptive learned index with an immutable learned base, ordered
  delta, configurable rebuild threshold, and `std::shared_mutex`
  reader/writer coordination. Rebuild merge/training runs from a captured
  snapshot outside the lock; publication removes only the captured delta keys.
- Standalone mixed reader/writer benchmark for the adaptive index, reporting
  aggregate reader throughput, ordinary insert latency, and rebuild-trigger
  insert latency; first repeated sample is Trial 011.
- CMake targets for app, library, tests, benchmarks, learned-model rank-error
  reports, and a seeded query correction-distance diagnostic; raw benchmark
  output and per-trial records.

## Verification

On Windows 10.0.26200 with GCC 16.2.0 and Google Benchmark 1.9.1:

- `cmake --build build-release` completed successfully.
- `ctest --test-dir build-release --output-on-failure`: 51/51 tests passed.
- `build-release/cpp/learned_index_app.exe` ran and demonstrated binary
  search, B+ tree, global learned, RMI, and adaptive index behavior.
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
- Trial 010 compared binary search, B+ tree, global learned, piecewise, and
  RMI lookup on four 100K-key shapes; RMI was slower than at least one
  alternative in every measured case and had higher training time in this
  first sample.
- Trial 011 ran five fresh processes each with 1, 4, and 8 readers concurrently
  with one writer inserting 2,500 keys. Exact checks passed in every run;
  reader throughput and rebuild-trigger latency are exploratory results with
  scheduling variation, not a scalability claim.

These checks do not prove correctness for all inputs and do not establish
general performance rankings. No sanitizer or race detector was run.

## Known scope limits and next work

1. The global model remains a single linear regression, with piecewise and
   two-stage RMI alternatives using fixed-size rank leaves. Initial evenly
   spaced, clustered, quadratic-gap, and one hot-query cases are recorded, but
   model-size/leaf-size trade-offs, additional skew levels, hot-set sizes, and
   fresh-process repeats remain open.
2. Rank residual and absolute correction-distance distributions are reported
   for seeded query samples, but actual probe counts and lookup latency by
   correction distance still need direct measurement.
3. The adaptive policy uses a configured count threshold. It does not estimate
   workload changes or tune that threshold automatically. Trial 003 shows that
   synchronous rebuild cost can dominate an update batch.
4. Concurrent correctness now covers multiple readers and writers while
   rebuilds publish snapshots. Rebuild merge/training no longer holds the
   exclusive lock, but the triggering writer remains synchronous and readers
   still acquire a shared lock. Concurrent throughput, write/rebuild latency,
   and race-sanitizer coverage remain to be measured. Broader concurrency
   guarantees and supported structures remain to be established; the RMI is
   currently static.
5. Updates support insertion only. Deletion, tombstones, persistence, crash
   recovery, B+ tree concurrency, and lock-free access are outside the current
   prototype.
6. The benchmark suite has point lookups for binary search, B+ tree, global,
   piecewise, and RMI indexes; training, bulk load, inserts, range scans, and
   an initial hot-query case. It still needs memory accounting,
   sequential scans, read/write mixes, and repeatability analysis across fresh
   processes.

The raw measurements and the limitations of each run are recorded in the
[experiment log](experiments/README.md). The project should only be described
as complete for its current prototype scope; the broader research specification
remains in progress.
