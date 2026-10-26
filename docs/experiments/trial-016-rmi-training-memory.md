# Trial 016: RMI training cost and model storage

- Date: 2026-10-26
- Question: How does the configured RMI target leaf size affect training time and estimated index-owned model storage across four key shapes?
- Source revision before this trial: `c4a3a90`.
- Raw output: [trial-016-rmi-training-memory-raw.csv](trial-016-rmi-training-memory-raw.csv)
- Command: `learned_index_benchmarks.exe --benchmark_filter="BM_RmiLearnedIndex_TrainByLeafSize/.*" --benchmark_out=trial-016-rmi-training-memory-raw.csv --benchmark_out_format=csv`.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type: CMake `build-release`, Release configuration; Google Benchmark 1.9.1.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home `10.0.26200`.
- Dataset: 100,000 deterministic keys per shape: random 64-bit (seed 12345), evenly spaced, clustered, and quadratic gaps.
- Workload: Construct a new RMI inside each timed iteration and train it on the full dataset. Leaf targets are 256, 512, 1,024, and 2,048 keys. The reported storage counter is the RMI object size plus the leaf-model vector's allocated capacity; it excludes the borrowed dataset, allocator bookkeeping, and other process overhead.
- Repetitions / warm-up: Five Google Benchmark repetitions per case, with a 0.5-second minimum warm-up; 16 cases total.
- Results: Median CPU training time in milliseconds; storage is the estimated bytes per trained index.

| Shape | Leaf 256 | Leaf 512 | Leaf 1,024 | Leaf 2,048 |
|---|---:|---:|---:|---:|
| Random 64-bit | 3.418 ms / 25,136 B | 4.236 ms / 12,656 B | 4.261 ms / 6,384 B | 4.142 ms / 3,248 B |
| Evenly spaced | 4.199 ms / 25,136 B | 4.179 ms / 12,656 B | 4.103 ms / 6,384 B | 4.047 ms / 3,248 B |
| Clustered | 4.103 ms / 25,136 B | 3.997 ms / 12,656 B | 4.088 ms / 6,384 B | 4.179 ms / 3,248 B |
| Quadratic gaps | 4.142 ms / 25,136 B | 4.023 ms / 12,656 B | 3.997 ms / 6,384 B | 4.330 ms / 3,248 B |

## Interpretation

The estimated storage shrinks monotonically as the leaf target grows: 2,048-key leaves use about 7.7 times less RMI-owned model storage than 256-key leaves in this 100K-key setup. Median training times show no consistent leaf-size winner and vary by less than about 0.4 ms across the tested settings. This suggests that, for this workload, the clear leaf-size trade-off is model storage; earlier lookup measurements show that lookup behavior still depends on shape and query type (Trial 015).

The RMI borrows the dataset, so this counter intentionally reports model-owned storage only. It is an estimate, not total resident memory: it omits vector allocator bookkeeping, dataset storage, executable/runtime overhead, and any temporary memory used while constructing the model.

## Limitations and next step

This is one machine, one dataset size, four deterministic shapes, and five repetitions per case in one benchmark process. The benchmark measures model construction and training together, not training alone; small time differences should not be interpreted as wins. The storage calculation covers only the RMI object's leaf-model vector capacity plus the object itself. Repeat at larger dataset sizes and in fresh processes, and add consistent total-memory accounting before selecting an automatic leaf-size policy.
