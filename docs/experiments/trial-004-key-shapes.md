# Trial 004: Existing lookups across key shapes

- Date: 2026-10-11
- Question: How do binary search, B+ tree, and the learned model compare on
  existing keys from four deterministic shapes?
- Source revision: based on `d4ab0ff` plus uncommitted implementation changes;
  no commit or push was made.
- Raw output: [`trial-004-raw.csv`](trial-004-raw.csv)
- Command:
  `.\build-release\cpp\learned_index_benchmarks.exe --benchmark_filter='BM_(BinarySearch|BPlusTree|LearnedIndex)_Contains(Existing|Missing)ByShape/100000/' --benchmark_out=docs/experiments/trial-004-raw.csv --benchmark_out_format=csv`
- Build / machine: Release, GCC 16.2.0, Google Benchmark 1.9.1, Windows
  10.0.26200, 12 logical processors reported at 2496 MHz.
- Dataset size: 100,000 unique keys; five repetitions, 0.5-second warm-up.
- Shapes: fixed-seed random 64-bit keys, evenly spaced keys, keys clustered
  into consecutive runs with large gaps, and quadratic gaps (`key = i*i`).
- Existing-key queries are selected uniformly by dataset rank. The other
  `ContainsMissingByShape` cases use random 64-bit absent keys, most of which
  are outside the dataset's min/max range for the bounded synthetic shapes.

## Existing-key medians

Nanoseconds per lookup, real-time median:

| Dataset shape | Binary search | B+ tree | Learned index |
|---|---:|---:|---:|
| Random 64-bit | 145 | 67.2 | 145 |
| Evenly spaced | 141 | 65.3 | 23.9 |
| Clustered | 109 | 68.3 | 111 |
| Quadratic gaps | 106 | 69.0 | 187 |

## Interpretation and limitations

The global linear model performed very well on the evenly spaced mapping and
was substantially slower on the quadratic-gap mapping, where its regression
prediction needs more correction. The B+ tree was the fastest existing-key
lookup in three of the four cases. This demonstrates why a single model and a
single workload cannot represent learned-index performance.

The in-range distribution varies by shape, so lookup cases are not a universal
ranking. The “missing” query stream is especially easy for the learned index
when queries fall outside the dataset bounds: it can reject them before
correction. Trial 005 separately measures keys drawn from internal gaps. This
run uses one size and one process; several coefficients of variation exceed
10%, so small timing differences are not meaningful.
