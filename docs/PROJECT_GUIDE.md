# Project guide

## What this project demonstrates

The project compares exact in-memory indexes for unique `uint64_t` keys. A
learned model predicts where a key should be in a sorted array, but the
prediction is only a hint: correction against the stored keys determines the
answer. Binary search and the B+ tree are important baselines because a learned
index is not faster on every key distribution or query pattern.

## Structures

| Structure | How it works | Update behavior |
|---|---|---|
| Binary search | `lower_bound` over a sorted vector | Static dataset; rebuilding the vector is required for inserts |
| B+ tree | Sorted leaf pages, separator keys in internal nodes, linked leaves for ranges | Inserts with node splits; no deletion or concurrent interface |
| Global learned index | One linear model predicts a rank; exponential bracketing and binary search correct it | Static model |
| Piecewise learned index | A model per rank segment reduces error on nonlinear key shapes | Static model; segment size is configured |
| RMI | A root model routes to a rank leaf model, then exact correction finds the answer | Static model; target leaf size is configured |
| Adaptive learned index | Immutable learned snapshot plus an ordered set of recent inserts | Insert-only; merges and retrains at a configured threshold |

## Adaptive rebuild and model selection

An adaptive lookup checks the recent-insert delta and then searches the current
base. When enough inserts accumulate, the triggering writer captures the
current snapshot and the inserts to rebuild. It releases the writer lock while
it merges keys and trains a replacement model, so readers continue to use the
old snapshot plus the delta. Publication installs the new snapshot and removes
only the inserts included in that build. The triggering writer waits for the
build to finish; rebuilds are not asynchronous from that writer.

The adaptive index can keep using the global model, always use an RMI, or
select between them on each rebuild. Automatic mode trains both models and
compares mean absolute rank error on up to 256 evenly spaced keys. It selects
RMI only when its sampled error is at least 20% lower. The demonstrated change
from global to RMI is based on the evolving key set, not on live query
frequencies. The RMI leaf size remains a configured parameter.

Multiple readers coordinate through a shared mutex. Builds happen outside the
lock, while snapshot publication and delta bookkeeping use the writer lock.
Tests cover exact membership across rebuilds, multiple readers and writers,
and an automatic global-to-RMI switch while readers query. The project has not
been checked with a race detector or sanitizer.

## How to read the benchmark results

The experiment log preserves raw CSVs, commands, machine details, and caveats.
The evidence is exploratory and comes mostly from one Windows laptop. Results
change with data shape, query type, and leaf size. The recorded trials found
no universal lookup or rebuild winner. Trial 016 shows that larger RMI leaf
targets reduce estimated model-owned storage, while training-time medians were
similar in the tested 100K-key workloads. This is not a total-memory estimate:
it excludes the borrowed dataset and allocator bookkeeping.

## Build, test, and run

From PowerShell at the repository root:

```powershell
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
.\build-release\cpp\learned_index_app.exe
```

The walkthrough prints binary search, B+ tree, learned, and static RMI
lookups, then shows both a configured adaptive RMI rebuild and an automatic
global-to-RMI transition. Additional diagnostic and benchmark commands are in
the [README](../README.md); measured results are indexed in
[experiments](experiments/README.md).

## Source map

- `cpp/include/learned_index/index/`: public index interfaces.
- `cpp/src/index/`: search and update implementations.
- `cpp/tests/`: exactness, update, and concurrency tests.
- `cpp/apps/`: walkthrough, diagnostics, and mixed reader/writer benchmark.
- `cpp/benchmarks/`: Google Benchmark workloads and generators.
- `docs/architecture.md`: detailed lookup and concurrency paths.
- `docs/STATUS.md`: verified scope, evidence, and limitations.
- `docs/experiments/`: trial reports and unmodified raw results.
