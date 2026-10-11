# Trial 001: Initial B+ tree comparison

- Date: 2026-10-11
- Question: On one random 100,000-key dataset, how does the first B+ tree
  compare with binary search for membership and a sorted vector for batched
  insertion and range retrieval?
- Source revision: based on repository commit `d4ab0ff`; the B+ tree,
  membership adapter, tests, benchmarks, README updates, and this record were
  present as uncommitted working-tree changes. No commit or push was made.
- Raw output: [`b_plus_tree_100k_results.csv`](../../b_plus_tree_100k_results.csv)
- Compiler / machine: GCC 16.2.0, Google Benchmark 1.9.1, Windows
  10.0.26200, 12 logical processors reported at 2496 MHz.
- Dataset: deterministic pseudorandom `uint64_t` keys, seed 12345; 100,000
  unique sorted keys. Existing and missing lookup query streams use fixed seeds.
- Repetitions / warm-up: five repetitions; 0.5-second minimum warm-up.
- Invocation: Release build, filtered to 100,000-key benchmark cases, with
  Google Benchmark CSV output. The saved CSV includes the complete run output.

## Results

Reported values below are medians from that run. Lookup and range values are
nanoseconds per operation. Insert values are real-time nanoseconds for a batch
of 64 new keys.

| Operation | Binary search / sorted vector | B+ tree | Observation |
|---|---:|---:|---|
| Existing-key membership | 168.017 ns | 80.5565 ns | B+ tree lower in this run |
| Missing-key membership | 216.849 ns | 81.1442 ns | B+ tree lower in this run |
| Batch of 64 inserts | 680,885 ns | 35,475 ns | B+ tree lower; high variation |
| Inclusive range returning 128 keys | 214.743 ns | 744.973 ns | Sorted vector lower |
| Bulk-load 100,000 keys | — | 597,126 ns | B+ tree construction cost |

The insert batch coefficients of variation were approximately 29.6% for the
sorted vector and 23.4% for the B+ tree. Treat those figures as noisy. CPU-time
samples for the B+ tree batch were below the timer's useful resolution, so the
comparison uses Google Benchmark's real-time field.

## Interpretation and limits

This is an initial baseline, not evidence that one index is universally
better. It covers one machine, one data size, uniformly pseudorandom keys, one
fanout (32), and five repetitions. The vector insertion baseline also copies
the original keys outside the timed region; the B+ tree is bulk-loaded outside
the timed region. Both time only the 64 insertions, but this does not compare
end-to-end construction-plus-update cost. Range timings include returning an
allocated vector of results. The `benchmark_results.csv` file is a separate
historical binary-search run and should not be combined with this trial.

## Follow-up

Repeat across multiple sizes and key distributions; sweep B+ tree fanout; add
random, monotonic, and clustered insertion patterns; report repeated runs and
spread; and measure end-to-end build-plus-update as a separate workload. Keep
the current raw CSV unchanged and record the next result under a new trial ID.
