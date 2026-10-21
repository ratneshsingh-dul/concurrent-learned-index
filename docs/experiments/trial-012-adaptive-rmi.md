# Trial 012: Adaptive RMI mixed reader/writer workload

- Date: 2026-10-22
- Question: Can the adaptive index retain a selected RMI base through
  insert-triggered retraining, and what is its first mixed-workload trade-off
  against the global model?
- Source revision / working-tree description: Dynamic RMI selection and
  benchmark CLI changes for this trial; exact raw rows are committed beside
  this report.
- Raw output: [trial-012-adaptive-rmi-raw.csv](trial-012-adaptive-rmi-raw.csv)
- Command: Three fresh processes for each of global, RMI leaf size 512, and RMI
  leaf size 1,024, at reader counts 1 and 4:
  `adaptive_concurrency_benchmark.exe <readers> 100000 2500 <model> [leaf-size]`.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type and flags: CMake `build-release`, Release configuration.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home
  `10.0.26200`.
- Dataset sizes, distribution, and seed: 100,000 evenly spaced even
  `uint64_t` keys; writer inserts 2,500 unique odd keys above the initial range.
  No RNG. Adaptive rebuild threshold is 256, producing 9 rebuilds per run.
- Workload and timed setup: Readers warm up with 1,024 lookups, then query only
  base keys while one writer inserts. Output includes aggregate reader
  operations per second, ordinary insert p50/p95, and rebuild-trigger insert
  p50/p95. Final size and every inserted key are checked after timing.
- Repetitions / warm-up: Three fresh processes for each model and reader-count
  setting; 1,024 warm-up lookups per reader.
- Results (medians across three runs):

  | Base model | Readers | Aggregate reader ops/s | Writer p50 (µs) | Writer p95 (µs) | Rebuild insert p50 (µs) | Rebuild insert p95 (µs) |
  |---|---:|---:|---:|---:|---:|---:|
  | Global | 1 | 265,551 | 3.3 | 25.6 | 2,915 | 3,401 |
  | Global | 4 | 26,001 | 2.8 | 30.0 | 3,157 | 3,711 |
  | RMI (512) | 1 | 340,148 | 2.8 | 23.9 | 5,541 | 6,387 |
  | RMI (512) | 4 | 27,239 | 2.6 | 25.9 | 6,266 | 6,867 |
  | RMI (1,024) | 1 | 345,348 | 2.7 | 20.8 | 5,094 | 5,594 |
  | RMI (1,024) | 4 | 33,833 | 2.5 | 23.4 | 5,670 | 6,305 |

- Interpretation: The adaptive index rebuilt and answered all exactness checks
  successfully with either model. In this small sample, RMI reader throughput
  was higher than the global model at both reader counts, while rebuild-trigger
  inserts took longer. The RMI's extra leaf models increase retraining work;
  leaf size changes both lookup and rebuild cost.
- Limitations / deviations: Only one evenly spaced dataset, one writer,
  base-key lookups, and three repetitions per setting were measured. Reader
  scheduling and this machine affect the short runs. There is no static RMI
  baseline or pre-change lock-held adaptive baseline in this comparison, and
  no model-memory measurement. These figures are exploratory, not a general
  performance ranking.
- Next experiment: Repeat across random, clustered, and quadratic-gap shapes;
  include delta-hit queries and more repetitions, then measure model memory and
  initial training separately from rebuild time.
