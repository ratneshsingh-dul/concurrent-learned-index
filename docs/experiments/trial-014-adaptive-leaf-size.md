# Trial 014: Adaptive RMI leaf-size comparison

- Date: 2026-10-25
- Question: How does the RMI leaf-size setting affect adaptive rebuild latency
  across the four deterministic key shapes, compared with global and automatic
  model policies?
- Source revision: `ff1e1f9` (benchmark now honors a leaf-size argument when a
  shape argument is also present).
- Raw output: [trial-014-adaptive-leaf-size-raw.csv](trial-014-adaptive-leaf-size-raw.csv)
- Command, repeated in five fresh processes per setting and shape:
  `adaptive_concurrency_benchmark.exe 1 100000 4096 <global|auto|rmi> <leaf-size> <even|clustered|quadratic|random>`.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type: CMake `build-release`, Release configuration.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home
  `10.0.26200`.
- Dataset: 100,000 deterministic keys; random generator seed 12345. Shapes:
  evenly spaced, clustered, quadratic gaps, and random 64-bit.
- Workload: One reader performs sequential existing-key membership checks while
  one writer inserts 4,096 unique keys above the initial maximum. The adaptive
  rebuild threshold is 256, so each process performs 16 merges and retrains.
  Each run checks final size and membership for every inserted key. Initial
  construction is outside the timed interval; the reported rebuild timings
  include merge, training, and publication.
- Repetitions: Five independent processes for each of six policies/settings
  on each shape (120 runs total). The CSV preserves each process row, including
  its median and p95 across the 16 rebuild-trigger insertions.

## Median rebuild-trigger insertion latency

Values below are medians across the five process-level medians, in
microseconds. Each RMI column names its target leaf size.

| Shape | Global | Auto (selected after final rebuild) | RMI 256 | RMI 512 | RMI 1,024 | RMI 2,048 |
|---|---:|---|---:|---:|---:|---:|
| Evenly spaced | 1,878.0 | 5,197.0 (RMI) | 3,645.1 | 3,698.3 | 3,640.1 | 3,660.9 |
| Clustered | 1,912.4 | 5,225.3 (RMI) | 3,709.5 | 3,749.8 | 3,624.0 | 3,690.0 |
| Quadratic gaps | 1,871.0 | 5,145.5 (global) | 3,674.2 | 3,677.7 | 3,865.1 | 3,740.5 |
| Random 64-bit | 1,902.2 | 5,281.4 (RMI) | 4,021.4 | 4,103.1 | 4,070.9 | 4,021.0 |

## Interpretation

- All 120 runs completed 16 rebuilds and passed the exact final membership
  check.
- The global model had the lowest median rebuild-trigger latency on every
  shape in this workload. The RMI settings took roughly 1.9–2.2 times as long
  as global; automatic mode took roughly 2.7 times as long because it trains
  both candidate models at each rebuild.
- No RMI leaf size was a consistent winner. The 256–2,048 settings were close
  on evenly spaced, clustered, and quadratic-gap data; their differences were
  small beside the global-versus-RMI cost gap. Random-key rebuilds were slower
  than the other shapes for all RMI leaf settings in this sample.
- Automatic mode's final selected model was RMI for evenly spaced, clustered,
  and random keys, and global for quadratic gaps. The choice differs from Trial
  013 on some shapes because this run inserts 4,096 keys and observes the final
  post-rebuild snapshot; the sampled-error policy can change its choice as the
  base changes.
- Reader throughput is retained in the raw CSV as context. The reader uses a
  single sequential hit stream while rebuilds run, so these figures do not
  represent a broader query distribution or establish a lookup winner.

## Limitations and next step

Five processes on one Windows machine are still an exploratory sample. The
trial varies only one reader, one writer, 100K initial keys, and a fixed 256-key
rebuild threshold. It does not measure model memory, lookup latency by RMI leaf
size, multiple query distributions, or another operating system. The result
does not identify an optimal RMI configuration. Next, measure static lookup
latency and model size by leaf size under seeded hit and in-range-miss queries,
then revisit whether automatic selection should account for rebuild cost.
