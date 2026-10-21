# Experiment log

This folder records how the implementation and its evidence change over time.
Keep raw benchmark output beside each trial entry. Never replace an older raw
result: give each run a new trial number and filename so results remain
auditable.

## Trial index

| Trial | Date | Question | Result |
|---|---|---|---|
| [001](trial-001-b-plus-tree-baseline.md) | 2026-10-11 | How does the first B+ tree compare with binary search and a sorted vector on one 100K-key random dataset? | B+ tree was faster on measured membership and 64-key insertion; sorted vector was faster for 128-key ranges. Exploratory only. |
| [002](trial-002-learned-lookup.md) | 2026-10-11 | How does a trained single-model learned lookup compare across 100K, 1M, and 10M random keys? | Learned lookup was competitive at 1M and faster in this sample at 10M; B+ tree was faster at 100K. Training time was measured separately. One key distribution only. |
| [003](trial-003-adaptive-insert-cost.md) | 2026-10-11 | What is the cost of 64 inserts when the final insert triggers learned-base retraining? | Rebuild cost dominates this batch at every size tested; this quantifies the chosen 64-key threshold rather than establishing an ideal threshold. |
| [004](trial-004-key-shapes.md) | 2026-10-11 | How do existing-key lookups vary across four deterministic key shapes? | Learned index excelled on evenly spaced keys, but B+ tree led on random, clustered, and quadratic-gap keys. The “missing” stream mostly tested out-of-range misses. |
| [005](trial-005-inrange-misses.md) | 2026-10-11 | How do all three indexes handle missing keys selected from actual gaps? | B+ tree led on random, clustered, and quadratic-gap cases; learned lookup was close on random keys and faster on evenly spaced keys. Several cases were noisy. |
| [006](trial-006-model-error.md) | 2026-10-11 | How far is the linear model's prediction from the true rank on each shape? | Mean absolute rank error was 0 for evenly spaced keys and 5,708 for quadratic gaps, matching the observed lookup strengths and weaknesses. |
| [007](trial-007-hot-query-workload.md) | 2026-10-12 | How do the indexes compare when 90% of requests target a 1% hot key set? | B+ tree had the lowest median in one 100K-key run; repeat across hot-set sizes and fresh processes. |
| [008](trial-008-piecewise-learned-index.md) | 2026-10-13 | Does a piecewise-linear rank model improve prediction and lookup on nonlinear key shapes enough to justify its training cost? | Quadratic-gap lookup improved substantially, while random/even/clustered lookups and model training were slower; results are shape-dependent and exploratory. |
| [009](trial-009-correction-distance.md) | 2026-10-14 | How far do global and piecewise predictions sit from the true insertion rank for existing and in-range-missing queries? | Piecewise predictions substantially reduced sampled rank distance on random and quadratic-gap keys; rank distance is a correction-work proxy, not a probe count or latency result. |
| [010](trial-010-rmi-index.md) | 2026-10-15 | How does a two-stage RMI compare with binary search, B+ tree, global learned, and piecewise learned lookups? | RMI was exact but did not lead in this first 100K-key sample and trained slower; broader segment-size and repeatability study remains. |
| [011](trial-011-adaptive-concurrency.md) | 2026-10-21 | What happens to adaptive-index reader throughput and insert/rebuild latency during mixed reads and writes? | Exact results in all runs; reader throughput was lower with 4–8 readers than with one, and a scheduling outlier reinforces that this first Windows sample is exploratory. |

## Recording the next trial

1. Create a new `trial-NNN-short-name.md` file by copying the template below.
2. Save the unedited Google Benchmark CSV as `trial-NNN-raw.csv` in this folder.
3. Record the exact command, source revision, compiler, build mode, machine,
   dataset shape, seeds, benchmark arguments, and any deviations.
4. Compare implementations only when the timed work and setup are equivalent.
   Report medians and variation; investigate timer-resolution and high-variance
   results instead of presenting them as wins.
5. Add the trial to the index above. Preserve earlier entries and raw files.

## Trial template

```markdown
# Trial NNN: Short title

- Date:
- Question:
- Source revision / working-tree description:
- Raw output:
- Command:
- Compiler and version:
- Build type and flags:
- Machine / operating system:
- Dataset sizes, distribution, and seed:
- Workload and timed setup:
- Repetitions / warm-up:
- Results (include units and variability):
- Interpretation:
- Limitations / deviations:
- Next experiment:
```
