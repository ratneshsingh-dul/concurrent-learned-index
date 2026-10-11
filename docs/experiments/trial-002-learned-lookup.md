# Trial 002: Static learned lookup

- Date: 2026-10-11
- Question: How does the first trained learned-position model compare with
  binary search and B+ tree membership lookup on larger random datasets, and
  what is its training cost?
- Source revision: based on `d4ab0ff`, with Trial 001 changes and the static
  `LearnedIndex` implementation present as uncommitted working-tree changes.
  No commit or push was made.
- Raw output: [`trial-002-raw.csv`](trial-002-raw.csv)
- Command:
  `.\build-release\cpp\learned_index_benchmarks.exe --benchmark_filter='BM_(BinarySearch|BPlusTree|LearnedIndex)_(ContainsExisting|ContainsMissing|Train)/100000' --benchmark_out=docs/experiments/trial-002-raw.csv --benchmark_out_format=csv`
- Build: CMake Release, GCC 16.2.0, Google Benchmark 1.9.1.
- Machine: Windows 10.0.26200, 12 logical processors reported at 2496 MHz.
- Dataset: fixed-seed `mt19937_64` unique keys (dataset seed 12345), sorted;
  existing and missing query streams use fixed seeds 67890 and 13579.
- Sizes measured: 100,000; 1,000,000; and 10,000,000 keys. The filter matches
  those three numeric prefixes. Five repetitions and 0.5-second warm-up.
- The complete CSV keeps the raw Google Benchmark environment preamble and
  every repetition. Summary values below are real-time medians, ns per lookup;
  training values are real-time medians for a full model fit.

## Results

| Keys | Query | Binary search | B+ tree | Learned index |
|---:|---|---:|---:|---:|
| 100K | Existing | 101 ns | 147 ns | 205 ns |
| 100K | Missing | 224 ns | 153 ns | 204 ns |
| 1M | Existing | 379 ns | 279 ns | 279 ns |
| 1M | Missing | 372 ns | 280 ns | 284 ns |
| 10M | Existing | 961 ns | 514 ns | 423 ns |
| 10M | Missing | 706 ns | 534 ns | 393 ns |

Learned-index training took 2.00 ms at 100K, 19.8 ms at 1M, and 199 ms at
10M. The 100K existing-key binary-search sample had a 38% coefficient of
variation; the 10M B+ tree missing-key sample had a 30% real-time coefficient
of variation. Treat those comparisons as noisy. Other cases were generally
less variable, though several 10M cases still showed 9–11% variation.

## Interpretation and limitations

This run demonstrates that the trained model is integrated into exact lookup
and that its construction cost is independently measurable. It does not show
that the learned index is generally faster: B+ tree lookup is better at 100K,
the learned and B+ tree medians are close at 1M, and the learned sample is
lower at 10M for this one uniformly pseudorandom key generator. Results are
machine- and run-specific. The 10M variation and the unexpectedly wide
100K binary-search spread warrant repeat runs before drawing stronger
conclusions. Only one key distribution and random query stream were covered.

The learned model borrows the immutable `SortedDataset`; callers must keep that
dataset alive. Its regression predicts a position, but lookup correctness does
not depend on prediction accuracy: exponential bracketing followed by binary
search verifies a candidate exactly.

## Next trial

Compare uniform, clustered, uneven-gap, and skewed key distributions; collect
multiple independent process runs; record prediction error and correction
distance; and compare model training against B+ tree bulk-load time. Then
measure an explicit delta-buffer/retraining policy under update workloads.
