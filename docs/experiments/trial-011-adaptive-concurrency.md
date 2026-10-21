# Trial 011: Adaptive mixed reader/writer workload

- Date: 2026-10-21
- Question: How does the adaptive learned index behave with simultaneous
  readers and one inserting writer, including the inserts that trigger rebuilds?
- Source revision / working-tree description: Changes for commit
  `664c036` plus the benchmark added for this trial; the benchmark source and
  raw output are committed with the trial.
- Raw output: [trial-011-adaptive-concurrency-raw.csv](trial-011-adaptive-concurrency-raw.csv)
- Command: `adaptive_concurrency_benchmark.exe <readers> 100000 2500`, five
  fresh process runs each at 1, 4, and 8 readers.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type and flags: CMake `build-release`, Release configuration.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home
  `10.0.26200`.
- Dataset sizes, distribution, and seed: 100,000 evenly spaced even `uint64_t`
  keys; inserted keys are unique odd values above the initial key range. No RNG.
- Workload and timed setup: Readers warm up with 1,024 base-key lookups, then
  concurrently look up base keys while one writer inserts 2,500 keys. Rebuild
  threshold is 256; the writer waits for rebuild completion. The output reports
  aggregate read operations per second, ordinary writer p50/p95 insert latency,
  and p50/p95/max latency of inserts that trigger a rebuild. Final size and
  every inserted key are checked after the timed phase.
- Repetitions / warm-up: Five fresh processes per reader count; 1,024 untimed
  lookups per reader before the barrier.
- Results (medians across five runs):

  | Readers | Aggregate reader ops/s | Writer p50 (µs) | Writer p95 (µs) | Rebuild insert p50 (µs) | Rebuild insert p95 (µs) |
  |---:|---:|---:|---:|---:|---:|
  | 1 | 379,403 | 1.9 | 16.0 | 1,889.7 | 2,026.8 |
  | 4 | 64,904.7 | 1.8 | 14.2 | 1,904.2 | 1,987.0 |
  | 8 | 63,183.9 | 1.8 | 14.4 | 1,963.2 | 2,039.5 |

- Interpretation: In this workload, aggregate reader throughput was much lower
  with 4 or 8 readers than with one, consistent with contention around this
  short lookup path and `shared_mutex`. Most individual rebuild-trigger inserts
  took around 1.9–2.0 ms at the median. The exact checks passed in all 15 runs.
- Limitations / deviations: One of five 4-reader runs had substantially higher
  elapsed time and writer latency than the others. OS scheduling affects this
  small single-machine sample. Readers query only the original base keys;
  write-heavy mixes, delta-hit queries, alternative lock designs, comparison
  against the previous lock-held implementation, memory use, and race detection
  are not covered. These numbers are not a general scalability claim.
- Next experiment: Compare the same workload against a retained baseline
  implementation or add delta-hit/mixed query ratios and longer timed runs.
