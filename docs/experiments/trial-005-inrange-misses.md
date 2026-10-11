# Trial 005: In-range missing-key lookups

- Date: 2026-10-11
- Question: How do the three indexes compare for absent keys located between
  stored keys rather than outside the dataset bounds?
- Source revision: based on `d4ab0ff` plus uncommitted implementation changes;
  no commit or push was made.
- Raw output:
  [`trial-005-inrange-misses-raw.csv`](trial-005-inrange-misses-raw.csv)
- Command:
  `.\build-release\cpp\learned_index_benchmarks.exe --benchmark_filter='BM_(BinarySearch|BPlusTree|LearnedIndex)_ContainsInRangeMissingByShape/100000/' --benchmark_out=docs/experiments/trial-005-inrange-misses-raw.csv --benchmark_out_format=csv`
- Build / machine: Release, GCC 16.2.0, Google Benchmark 1.9.1, Windows
  10.0.26200, 12 logical processors reported at 2496 MHz.
- Dataset: 100,000 keys for each of random 64-bit, evenly spaced, clustered,
  and quadratic-gap shapes. Five repetitions and 0.5-second warm-up.
- Queries: 1,000 deterministic absent keys are selected by choosing an
  available adjacent-key gap uniformly and then choosing an integer strictly
  inside that gap (seed 97531). Each structure receives the same query stream
  for each dataset shape.

## Results

Real-time median nanoseconds per in-range miss:

| Dataset shape | Binary search | B+ tree | Learned index |
|---|---:|---:|---:|
| Random 64-bit | 141 | 105 | 107 |
| Evenly spaced | 136 | 106 | 40.3 |
| Clustered | 88.9 | 53.3 | 131 |
| Quadratic gaps | 134 | 74.0 | 190 |

The learned index was close to the B+ tree for random in-range misses and
faster for evenly spaced keys. The B+ tree led on clustered and quadratic-gap
data. Variation was notable: the evenly spaced B+ tree case had a 35% real-time
coefficient of variation, and the quadratic-gap binary-search case had a 24%
coefficient. Treat close cases as inconclusive until repeated in fresh
processes.

## Interpretation and limitations

This workload closes one gap in the earlier random-miss benchmarks: absent
queries now land inside the data range, where the index must locate a real gap.
It still uses one key count and one process. Gap selection is uniform across
available gaps, not weighted by gap width or by a production workload. It does
not measure model prediction residuals or correction distance directly.
