# Trial 007: Hot-query lookup workload

- Date: 2026-10-12
- Question: How do binary search, B+ tree, and learned-index membership
  lookups compare when most requests repeatedly hit a small hot set?
- Source revision: `754a80f`, with the hot-query generator, tests, and
  benchmarks committed. This report and its raw CSV were added afterward.
- Raw output: [`trial-007-hot-queries-raw.csv`](trial-007-hot-queries-raw.csv)
- Command:
  `.\build-release\cpp\learned_index_benchmarks.exe --benchmark_filter='BM_.*ContainsHotQueries/100000/' --benchmark_out=docs/experiments/trial-007-hot-queries-raw.csv --benchmark_out_format=csv`
- Build / compiler: Release, GCC 16.2.0 (MSYS2 UCRT64), Google Benchmark
  1.9.1.
- Machine: Windows 10.0.26200; 12 logical processors reported at 2496 MHz.
- Dataset: 100,000 fixed-seed random 64-bit keys, sorted and unique.
- Query workload: 1,000 pre-generated existing-key queries; 90% are sampled
  from a uniformly selected hot set containing 1% of dataset keys, and 10%
  are sampled uniformly from the full dataset. The same deterministic stream
  is reused for all three indexes. Dataset and query generation are outside
  the timed loop.
- Repetitions / warm-up: five repetitions; 0.5-second minimum warm-up.

## Results

Real-time median per membership query, with coefficient of variation (CV) from
the five repetitions:

| Index | Median | CV |
|---|---:|---:|
| Binary search over sorted vector | 133.7 ns | 7.1% |
| B+ tree | 86.7 ns | 10.4% |
| Learned index | 118.7 ns | 4.8% |

The B+ tree had the lowest median in this run. The learned index was between
the B+ tree and binary-search medians. The result is directionally consistent
with earlier random-query measurements, but the hot-set access pattern is now
explicit and repeatable.

## Limits and next step

This is one 100K-key dataset, one machine, one hot-set size, and one process.
The 1,000-query stream cycles repeatedly inside the timed region, so this
measures a warm, steady hot workload rather than cold-start behavior. The
sample does not establish a general winner or demonstrate production traffic.
Repeat in fresh processes and vary both the hot-set size and request skew
before making stronger claims.
