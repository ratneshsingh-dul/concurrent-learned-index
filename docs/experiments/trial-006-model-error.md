# Trial 006: Learned-model rank error

- Date: 2026-10-11
- Question: How closely does the one-line regression predict the true rank
  across the four deterministic dataset shapes?
- Source revision: based on `d4ab0ff` plus uncommitted implementation changes;
  no commit or push was made.
- Raw output: [`trial-006-model-error.csv`](trial-006-model-error.csv)
- Command:
  `.\build-release\cpp\learned_model_diagnostics.exe 100000`
- Build / machine: Release, GCC 16.2.0, Windows 10.0.26200.
- Dataset size: 100,000 keys per shape. The tool evaluates each stored key and
  reports absolute rank error `|predicted_position - actual_position|`.

## Results

| Dataset shape | Mean absolute rank error | P50 | P95 | Maximum |
|---|---:|---:|---:|---:|
| Random 64-bit | 68.91 | 66 | 154 | 207 |
| Evenly spaced | 0 | 0 | 0 | 0 |
| Clustered | 63.95 | 64 | 122 | 128 |
| Quadratic gaps | 5,708.16 | 5,590 | 13,983 | 18,749 |

The exact learned lookup still corrects these predictions before returning a
result. The rank-error distribution supports the latency pattern in Trials 004
and 005: the model is exact on evenly spaced keys and needs far more correction
for quadratic gaps. These are in-sample errors over the stored keys. They do
not establish bounds for unseen queries or for a changed dataset.
