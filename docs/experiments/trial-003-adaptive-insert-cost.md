# Trial 003: Adaptive learned-index batch insert cost

- Date: 2026-10-11
- Question: What does a 64-key update batch cost when the adaptive learned
  index rebuilds and retrains its base on the final insert?
- Source revision: based on `d4ab0ff`, with Trials 001–002 and the adaptive
  index / locking implementation present as uncommitted working-tree changes.
  No commit or push was made.
- Raw output: [`trial-003-raw.csv`](trial-003-raw.csv)
- Command:
  `.\build-release\cpp\learned_index_benchmarks.exe --benchmark_filter='BM_(BPlusTree|SortedVector|AdaptiveLearnedIndex)_InsertBatch/1000|BM_(BPlusTree|SortedVector|AdaptiveLearnedIndex)_InsertBatch/10000|BM_(BPlusTree|SortedVector|AdaptiveLearnedIndex)_InsertBatch/100000' --benchmark_out=docs/experiments/trial-003-raw.csv --benchmark_out_format=csv`
- Build / machine: Release, GCC 16.2.0, Google Benchmark 1.9.1, Windows
  10.0.26200, 12 logical processors reported at 2496 MHz.
- Data: same seeded random dataset generator and 64 new keys for each size.
- Sizes: 1K, 10K, and 100K base keys. Twenty batches per repetition, five
  repetitions, 0.5-second minimum warm-up. Real-time used because the measured
  operations are short and Windows CPU-time readings are quantized.

## Results

Median wall-clock time to apply a 64-key batch:

| Base keys | B+ tree inserts | Sorted-vector inserts | Adaptive learned inserts plus threshold rebuild |
|---:|---:|---:|---:|
| 1K | 4.86 us | 4.31 us | 310 us |
| 10K | 36.2 us | 42.1 us | 660 us |
| 100K | 58.4 us | 988 us | 3.43 ms |

The adaptive case bulk-loads/trains its starting snapshot outside the timed
region, times all 64 insert operations, and has a rebuild threshold of 64, so
the final insertion includes merging all keys and fitting a new model. The
other structures also prepare their initial state outside timing but do not
rebuild on their final insert. This is an end-of-batch policy-cost comparison,
not a comparison of identical per-insert maintenance work. Several short
samples have noisy or quantized CPU times; use the real-time medians above and
consult the raw repetitions before drawing conclusions.

## Interpretation and next step

With threshold 64, rebuild cost dominates the adaptive index's batch update
time. That is a concrete cost of keeping the learned base fresh. A practical
system needs a workload-supported policy to choose when to rebuild, and should
also measure lookup degradation as the delta grows. This trial does not tune
the threshold and does not establish that 64 is a good setting. Next compare
several thresholds under read-heavy, update-heavy, and bursty workloads while
recording lookup correction work and rebuild pauses.
