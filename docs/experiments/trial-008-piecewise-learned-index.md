# Trial 008: Piecewise learned-index comparison

- Date: 2026-10-13
- Question: Does fitting one linear model per rank segment improve prediction
  and lookup on nonlinear key shapes enough to justify its additional training
  cost?
- Source revision: `5b81615` (`Compare global and piecewise model error`)
- Raw output: [Google Benchmark CSV](trial-008-piecewise-raw.csv); [rank error CSV](trial-008-piecewise-error.csv)
- Compiler and version: GCC 16.2.0 (MSYS2)
- Build type: CMake Release configuration, `build-release`
- Machine: Windows; Google Benchmark reported 12 logical CPUs at 2496 MHz.
- Dataset: 100,000 deterministic unique `uint64_t` keys per shape; random,
  evenly spaced, clustered, and quadratic gaps. The diagnostic used the same
  four shapes. Piecewise target segment size was the default 1,024 keys.
- Workload: five repetitions per case, 0.5-second minimum warmup. Existing
  key membership and missing queries selected from actual gaps were measured
  separately. Training was timed separately on random-shaped keys.
- Commands:

  ```powershell
  .\build-release\cpp\learned_index_benchmarks.exe `
    --benchmark_filter="BM_(LearnedIndex|PiecewiseLearnedIndex)_(ContainsExistingByShape|ContainsInRangeMissingByShape)/100000/|BM_(LearnedIndex|PiecewiseLearnedIndex)_Train/100000/" `
    --benchmark_out=docs/experiments/trial-008-piecewise-raw.csv `
    --benchmark_out_format=csv
  .\build-release\cpp\learned_model_diagnostics.exe 100000
  ```

## Results

The table reports median CPU time in nanoseconds over five repetitions.

| Shape | Existing global | Existing piecewise | In-range miss global | In-range miss piecewise |
|---|---:|---:|---:|---:|
| Random 64-bit | 188.3 | 214.8 | 190.4 | 200.2 |
| Evenly spaced | 85.4 | 129.4 | 102.5 | 150.0 |
| Clustered | 180.3 | 278.3 | 192.5 | 204.0 |
| Quadratic gaps | 286.0 | 172.6 | 295.1 | 172.3 |

Model training took 1.475 ms median for the global model and 2.051 ms for the
piecewise model. The corresponding coefficient of variation was 12.5% and
2.4%, respectively. Lookup coefficients of variation were between 2.3% and
6.7% in this run.

Absolute prediction errors on existing keys:

| Shape | Global mean / p50 / p95 / max | Piecewise mean / p50 / p95 / max |
|---|---:|---:|
| Random 64-bit | 68.91 / 66 / 154 / 207 | 6.38 / 5 / 16 / 32 |
| Evenly spaced | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |
| Clustered | 63.95 / 64 / 122 / 128 | 63.41 / 63 / 121 / 142 |
| Quadratic gaps | 5,708.16 / 5,590 / 13,983 / 18,749 | 2.06 / 1 / 7 / 191 |

## Interpretation and limits

The piecewise model is a strong fit for the tested quadratic-gap keys: lookup
median fell about 40% for present keys and about 42% for in-range misses, while
mean absolute rank error fell from about 5,708 to 2. It was slower on the
other three existing-key workloads, and training took about 39% longer. Thus
the new model expands the design options for nonlinear data; this sample does
not support making it the universal default. The quadratic maximum error of
191 also shows why average or percentile error alone is not enough to describe
the correction tail.

This was one process on one Windows machine and one dataset size. Repetitions
are short and do not capture cold starts, memory footprint, different segment
sizes, hot-query skew, or variation across fresh processes. The piecewise
class is static and borrows its dataset. Exact correction remains required
for correctness. Treat all performance conclusions as exploratory.

## Next experiment

Sweep segment sizes and dataset sizes, record model memory and correction
distance for actual lookups, and repeat selected cases in fresh processes
before considering an automatic choice between model types.
