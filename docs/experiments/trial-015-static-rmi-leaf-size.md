# Trial 015: Static RMI leaf-size lookup comparison

- Date: 2026-10-25
- Question: Does the RMI target leaf size change static lookup latency across
  key shapes, and how do those results compare with binary search, a B+ tree,
  and the global learned index?
- Source revision: `11be8fb`.
- Raw output: [trial-015-static-rmi-leaf-size-raw.csv](trial-015-static-rmi-leaf-size-raw.csv)
- Command:
  `learned_index_benchmarks.exe --benchmark_filter="BM_(BinarySearch|BPlusTree|LearnedIndex)_Contains(Existing|InRangeMissing)ByShape/.*|BM_RmiLearnedIndex_Contains(Existing|InRangeMissing)ByLeafSize/.*" --benchmark_out=trial-015-static-rmi-leaf-size-raw.csv --benchmark_out_format=csv`.
- Compiler and version: GCC 16.2.0 (MSYS2), C++20.
- Build type: CMake `build-release`, Release configuration; Google Benchmark
  1.9.1.
- Machine / operating system: Intel Core i5-12450H, Windows 11 Home
  `10.0.26200`.
- Dataset: 100,000 deterministic keys per shape: random 64-bit (seed 12345),
  evenly spaced, clustered, and quadratic gaps.
- Workload: Four exact static indexes on existing-key hits and in-range misses.
  RMI target leaf sizes are 256, 512, 1,024, and 2,048. Binary search, B+ tree,
  and global learned lookup are the baselines. Index construction and query
  generation occur outside the timed loop. Each case cycles through 1,000
  deterministic queries; the same generator is used for each index on a shape.
- Repetitions / warm-up: Five Google Benchmark repetitions per case, with a
  0.5-second minimum warm-up. The CSV contains 56 cases, each with five raw
  repetitions and mean, median, standard-deviation, and CV aggregates.
- Reported results: median CPU time across the five repetitions, in ns/query.

## Median lookup latency

| Shape | Query | Binary | B+ tree | Global | RMI 256 | RMI 512 | RMI 1,024 | RMI 2,048 |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Random 64-bit | Hit | 169.5 | 191.8 | 223.2 | 256.7 | 244.8 | 251.8 | 278.7 |
| Random 64-bit | In-range miss | 263.7 | 167.4 | 208.6 | 249.0 | 276.2 | 262.3 | 261.6 |
| Evenly spaced | Hit | 249.0 | 188.0 | 100.1 | 200.2 | 188.3 | 190.4 | 168.8 |
| Evenly spaced | In-range miss | 245.9 | 168.5 | 107.4 | 226.0 | 209.3 | 205.1 | 195.3 |
| Clustered | Hit | 251.1 | 193.6 | 224.6 | 188.3 | 322.3 | 336.9 | 337.6 |
| Clustered | In-range miss | 258.4 | 77.4 | 196.7 | 139.5 | 229.5 | 263.7 | 308.7 |
| Quadratic gaps | Hit | 257.4 | 167.4 | 351.6 | 388.2 | 380.9 | 380.9 | 351.6 |
| Quadratic gaps | In-range miss | 234.4 | 169.5 | 331.3 | 380.9 | 368.3 | 376.0 | 367.3 |

## Interpretation

- Results depend on both key shape and query type. The global learned model
  was clearly fastest on evenly spaced keys. The B+ tree led most remaining
  cases, including both quadratic-gap cases and clustered in-range misses.
- RMI with 256-key leaves was fastest on clustered hits in this sample
  (188.3 ns versus 193.6 ns for the B+ tree), but the difference is small
  relative to the observed run variation. That is a measured lead in this
  sample, not evidence that RMI is generally faster.
- The best RMI leaf size changed by shape and query. A smaller leaf helped the
  clustered distribution; 2,048 was fastest among RMI settings on evenly
  spaced hits and quadratic hits, while 512 was fastest on random hits and
  quadratic misses. No single setting won across all cases.
- Most coefficients of variation were below 10%; the binary random-hit case
  was 17.5% and the global quadratic in-range-miss case was 23.2%. Small gaps
  between settings should not be treated as reliable improvements.
- This supports retaining binary search and B+ tree as serious baselines. A
  learned index is useful on favorable distributions, but the RMI remains an
  experimental option whose benefit depends on shape and query mix.

## Limitations and next step

This is one machine, one dataset size, one deterministic query corpus per
shape, and five repetitions within one benchmark process. The comparison does
not measure model memory, model training time, other dataset sizes, hot-query
traffic, or production-style mixed operations. Benchmark loops consume the
generated queries but do not validate every returned answer inside the timed
run; RMI correctness is separately covered by the unit tests, including every
key on all four shapes at multiple leaf sizes. Next, measure RMI model memory
and training cost alongside static lookup, then repeat the strongest cases in
fresh processes before changing any default leaf size.
