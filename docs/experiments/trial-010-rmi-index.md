# Trial 010: First two-stage RMI comparison

- Date: 2026-10-15
- Question: How does the first two-stage Recursive Model Index (RMI) compare
  with binary search, B+ tree, global learned, and piecewise learned indexes?
- Source revision: `11697d4` (`Benchmark RMI lookup and training`)
- Raw output: [Google Benchmark CSV](trial-010-rmi-raw.csv)
- Compiler and version: GCC 16.2.0 (MSYS2)
- Build type: CMake Release configuration, `build-release`
- Machine: Windows; Google Benchmark reported 12 logical CPUs at 2496 MHz.
- Dataset: four deterministic 100,000-key shapes: random 64-bit, evenly
  spaced, clustered, and quadratic gaps. Default RMI leaf size is 1,024 keys.
- Workload: five repetitions and 0.5-second minimum warmup. Existing-key and
  in-range-missing membership cases were run on the same seeded query streams
  for each index. Training time was measured separately for the three learned
  models on random 100K keys.
- Command:

  ```powershell
  .\build-release\cpp\learned_index_benchmarks.exe `
    --benchmark_filter="BM_(BinarySearch|BPlusTree|LearnedIndex|PiecewiseLearnedIndex|RmiLearnedIndex)_(ContainsExistingByShape|ContainsInRangeMissingByShape)/100000/|BM_(LearnedIndex|PiecewiseLearnedIndex|RmiLearnedIndex)_Train/100000/" `
    --benchmark_out=docs/experiments/trial-010-rmi-raw.csv `
    --benchmark_out_format=csv
  ```

Times below are median CPU nanoseconds over five repetitions. Each cell is
`binary search / B+ tree / global learned / piecewise / RMI`.

## Existing-key lookup

| Shape | Median ns |
|---|---:|
| Random 64-bit | 223.2 / 184.2 / 202.3 / 209.3 / 213.1 |
| Evenly spaced | 198.8 / 165.0 / 82.3 / 136.7 / 153.8 |
| Clustered | 213.4 / 168.8 / 188.3 / 278.3 / 255.0 |
| Quadratic gaps | 213.1 / 165.0 / 293.0 / 153.5 / 311.5 |

## In-range-missing lookup

| Shape | Median ns |
|---|---:|
| Random 64-bit | 225.0 / 175.8 / 180.3 / 188.3 / 214.8 |
| Evenly spaced | 219.7 / 160.1 / 96.3 / 73.2 / 176.5 |
| Clustered | 235.8 / 78.5 / 192.5 / 129.4 / 224.6 |
| Quadratic gaps | 230.2 / 150.0 / 226.0 / 108.1 / 306.9 |

## Model training

| Model | Median time |
|---|---:|
| Global learned | 2.011 ms |
| Piecewise learned | 1.759 ms |
| RMI | 3.830 ms |

## Interpretation and limits

The first RMI implementation is an exact, two-stage static index, but this
sample shows no workload where it leads the tested alternatives. It is slower
than piecewise on quadratic-gap keys, where a segmented model already fits
well, and its model training takes longer. It remains a useful architecture
for further study, not a recommended default. RMI lookup is globally corrected
after the two-stage prediction, so its exactness does not depend on the root
choosing the leaf most likely to contain a key.

Several results have noticeable variation, including selected B+ tree and
miss cases. This is one short process on one machine at one dataset size and
one RMI leaf size; the trial does not measure model memory or concurrency.
These results do not establish a general ranking.

## Next experiment

Sweep RMI leaf size and compare correction distance, training cost, model
memory, and fresh-process lookup measurements. Keep concurrency testing as a
separate milestone; no RMI thread-safety or concurrent-throughput claim is
made here.
