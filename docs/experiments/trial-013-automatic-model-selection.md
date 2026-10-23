# Trial 013: Automatic adaptive model selection

- Date: 2026-10-23
- Question: How does the automatic global-versus-RMI choice vary across key
  shapes, and what rebuild cost does the selection policy add?
- Source revision / working-tree description: Automatic model policy and
  shape-selectable concurrency benchmark for this trial; raw output is saved
  beside this report.
- Raw output: [trial-013-auto-model-selection-raw.csv](trial-013-auto-model-selection-raw.csv)
- Command: Three fresh processes for each data shape under forced global and
  automatic policy:
  `adaptive_concurrency_benchmark.exe 1 100000 256 <global|auto> 1024 <shape>`.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type and flags: CMake `build-release`, Release configuration.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home
  `10.0.26200`.
- Dataset sizes, distribution, and seed: 100,000 keys using the shared
  deterministic `evenly_spaced`, `clustered`, `quadratic_gaps`, and `random_64`
  generators. Random generator seed is 12345. Each run inserts 256 unique keys
  immediately above the initial maximum and triggers one rebuild. RMI leaf size
  is 1,024.
- Workload and timed setup: One reader queries existing base keys while one
  writer inserts the batch. The initial model is built before timing; the
  reported rebuild-trigger insert includes merge, model training and
  publication. `selected_model` reports the active model after this rebuild.
  Final size and inserted-key membership are checked.
- Repetitions / warm-up: Three fresh processes per policy and shape; 1,024
  warm-up lookups by the reader.
- Results (median rebuild-trigger insertion latency):

  | Dataset shape | Model selected by auto | Forced global (µs) | Automatic (µs) |
  |---|---|---:|---:|
  | Evenly spaced | Global | 3,462.8 | 8,987.9 |
  | Clustered | Global | 1,897.7 | 5,261.1 |
  | Quadratic gaps | Global | 1,885.0 | 5,166.4 |
  | Random 64-bit | RMI | 1,905.7 | 5,432.7 |

- Interpretation: The 20% sampled-error rule selected RMI for the random-key
  workload and retained the global model for the other shapes. Exact checks
  passed in all 24 runs. Automatic rebuilds were about 2.6–2.9 times slower in
  this sample because both candidate models are trained before selecting one.
  This exposes the selection-cost trade-off directly.
- Limitations / deviations: Three repetitions on one machine are preliminary.
  The benchmark measures a single reader/writer, base-key lookups, and one
  rebuild; it does not measure model memory or sustained workload adaptation.
  The selection score is prediction error on sampled existing keys, not actual
  query latency or frequency. The selected model is reported after rebuilding,
  so it reflects the appended keys too.
- Next experiment: Evaluate more RMI leaf sizes and a lower-cost selection
  method, measure model memory and sampled rank errors, then test multiple
  reader/write ratios across the same shapes.
