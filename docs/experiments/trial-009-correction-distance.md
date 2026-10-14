# Trial 009: Query rank residuals and correction distance

- Date: 2026-10-14
- Question: For deterministic hit and in-range-miss query streams, how far are
  global and piecewise model predictions from the exact lower-bound rank?
- Source revision: `933c85b` (`Add learned correction diagnostic command`)
- Raw output: [CSV diagnostics](trial-009-correction-distance-raw.csv)
- Build: CMake Release, GCC 16.2.0 (MSYS2), Windows.
- Dataset: 100,000 keys for each deterministic shape: random 64-bit, evenly
  spaced, clustered, and quadratic gaps. Piecewise segment size: default
  1,024 keys.
- Workload: 1,000 seeded existing-key queries and 1,000 seeded in-range
  missing-key queries per shape. Existing queries use seed 67890; in-range
  misses use seed 97531. The two model types see the same queries.
- Command:

  ```powershell
  .\build-release\cpp\learned_correction_diagnostics.exe 100000
  ```

The signed residual is `predicted rank - exact lower-bound insertion rank`;
negative values mean the model predicts too far left. Absolute rank distance
is the number of array positions separating those ranks. It is a useful
correction-work proxy, but it is not a count of probes or comparisons in the
index's exponential-bracketing and binary-search implementation. Percentiles
use the lower-index convention `floor((n - 1) * p / 100)`.

## Mean and tail distance

Values are mean absolute rank distance / p95 distance, in array positions.

| Shape | Query kind | Global | Piecewise |
|---|---|---:|---:|
| Random 64-bit | Existing | 69.80 / 153 | 6.60 / 15 |
| Random 64-bit | In-range missing | 68.12 / 155 | 6.38 / 16 |
| Evenly spaced | Existing | 0 / 0 | 0 / 0 |
| Evenly spaced | In-range missing | 0.05 / 0 | 0 / 0 |
| Clustered | Existing | 63.41 / 121 | 62.90 / 120 |
| Clustered | In-range missing | 63.60 / 121 | 47.11 / 119 |
| Quadratic gaps | Existing | 5,799.34 / 13,712 | 2.01 / 7 |
| Quadratic gaps | In-range missing | 5,982.84 / 14,535 | 2.26 / 8 |

The signed residuals are also in the raw output. For example, on quadratic-gap
existing queries, the global model's mean signed residual was -576.27 and its
median was -2,233: the average hides a strongly left-biased distribution and
large tail. The piecewise model's corresponding values were 0.03 and 0.

## Interpretation and limits

This query-sampled view agrees with the earlier stored-key prediction study:
piecewise segments substantially reduce rank distance on random and
quadratic-gap keys, while the two models are similar on clustered existing
keys and both have zero distance on evenly spaced hits. Actual lookup cost
still depends on the correction algorithm and memory behavior; these rank
statistics alone do not establish latency improvements.

This is one deterministic sample per shape at one key count and one segment
size. It does not count actual probes, model memory, cold-cache effects, or
fresh-process variance. It is a diagnostic aid, not a performance benchmark.

## Next experiment

Measure actual correction probe counts or benchmark latency while sweeping
piecewise segment size, and preserve the same query stream across models.
