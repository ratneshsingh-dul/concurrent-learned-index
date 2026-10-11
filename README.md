# Concurrent Learned Index

An experimental C++20 project comparing exact lookup structures for static and
changing key sets. The long-term goal is to measure where learned indexes help
and where traditional structures remain the better choice.

## Implemented

- A validated, sorted, vector-backed SortedDataset.
- A binary-search baseline that returns an exact vector position.
- A BinarySearchIndex adapter that also exposes exact membership through
  contains(key).
- An in-memory B+ tree for unique uint64_t keys, with bulk loading, insertion,
  exact membership lookup, inclusive range queries, and an invariant checker.
- A static single-model learned index that predicts key positions and applies
  exact exponential/binary correction, so model errors cannot cause wrong
  lookup results.
- An insert-only adaptive learned index with an ordered delta, configurable
  merge/retraining threshold, and shared-reader/exclusive-writer locking.
- GoogleTest coverage and Google Benchmark workloads for lookup, construction,
  insertion, and range queries.

The B+ tree is single-threaded and in-memory. It is not a persistent database
tree. Duplicate insertion returns false and does not change the tree.
`AdaptiveLearnedIndex` is the only thread-safe structure in the current
prototype: concurrent lookups share a reader lock and inserts/rebuilds take an
exclusive lock. A rebuild currently blocks readers. The implementation is
insert-only; deletion, tombstones, lock-free reads, and automatic threshold
tuning are not implemented.

## Build and test

The project uses CMake, GoogleTest, and Google Benchmark. From the repository
root in PowerShell, run:

    cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-release
    ctest --test-dir build-release --output-on-failure

Run the small demonstration program:

    .\build-release\cpp\learned_index_app.exe

Inspect the first model's rank error on all four deterministic benchmark key
shapes (optional argument changes the dataset size):

    .\build-release\cpp\learned_model_diagnostics.exe 100000

## Benchmarks

The benchmark executable includes the historical binary-search position lookup
and a matched exact-membership comparison for binary search and the B+ tree.
Additional cases measure learned-model training, B+ tree bulk loading, batches
of 64 inserts against sorted-vector insertion, adaptive inserts that trigger a
rebuild, and 128-key inclusive range queries against a sorted-vector range
scan. Deterministic lookup shapes include random 64-bit, evenly spaced,
clustered, and quadratic-gap keys. Out-of-range misses and misses chosen from
in-range gaps are separate workloads.

Lookup and bulk-load cases use datasets of 1K through 10M keys. Insert cases use
1K through 100K initial keys, and range cases use 10K through 1M. Cases use
five repetitions and a 0.5-second minimum warm-up. Dataset and query generation
are outside the timed lookup loops.

To run the suite and save raw Google Benchmark output:

    .\build-release\cpp\learned_index_benchmarks.exe --benchmark_out=b_plus_tree_results.csv --benchmark_out_format=csv

The existing benchmark_results.csv contains historical binary-search
measurements. New runs are not assumed to reproduce those machine-specific
values. No performance winner is claimed until comparable runs are collected
and reviewed.

### Preliminary 100K-key sample

One Release run on Windows 10.0.26200, GCC 16.2.0, and Google Benchmark 1.9.1
produced these five-repetition medians using the same generated 100K-key data:

| Operation | Binary search / sorted vector | B+ tree |
|---|---:|---:|
| Existing-key membership | 168 ns | 80.6 ns |
| Missing-key membership | 217 ns | 81.1 ns |
| Batch of 64 new inserts | 681 us | 35.5 us |
| Inclusive range returning 128 keys | 215 ns | 745 ns |
| Index bulk load | — | 597 us |

This sample suggests the B+ tree handles this insert workload well, while the
contiguous vector is faster for range scans. It covers one key distribution,
one dataset size, and one machine; the insert measurements also had substantial
run-to-run variation. Some Windows CPU-time readings for the short insert
batches were below timer resolution, so those results use Google Benchmark's
wall-clock real-time field. The raw output is in
b_plus_tree_100k_results.csv.

Each benchmark run should be kept as a separate trial with its raw output,
machine/build details, interpretation, and limitations. See the
[experiment log](docs/experiments/README.md) and the
[first B+ tree trial](docs/experiments/trial-001-b-plus-tree-baseline.md).
The first learned lookup comparison is recorded in
[Trial 002](docs/experiments/trial-002-learned-lookup.md), including its raw
CSV output and training measurements. The update/retraining trade-off is in
[Trial 003](docs/experiments/trial-003-adaptive-insert-cost.md). The adaptive
index currently supports insertion only; it has no deletion/tombstone path.
Trials [004](docs/experiments/trial-004-key-shapes.md) and
[005](docs/experiments/trial-005-inrange-misses.md) add key-shape and
in-range-miss comparisons. [Trial 006](docs/experiments/trial-006-model-error.md)
records the model's per-shape rank error.

## Roadmap

The current prototype includes exact static learned lookup and a thresholded
insert buffer with synchronous retraining and shared/exclusive locking. The
next research steps are broader workload distributions, learned-model error
and correction-distance reporting, threshold tuning, concurrent throughput
benchmarks, and sanitizer/race-tool coverage. See the
[implementation status](docs/STATUS.md) and [experiment log](docs/experiments/README.md)
for what has been measured and what remains.
