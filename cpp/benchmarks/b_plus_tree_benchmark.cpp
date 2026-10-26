#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include <learned_index/data/key.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/b_plus_tree.hpp>
#include <learned_index/index/binary_search_index.hpp>
#include <learned_index/index/adaptive_learned_index.hpp>
#include <learned_index/index/learned_index.hpp>
#include <learned_index/index/piecewise_learned_index.hpp>
#include <learned_index/index/rmi_learned_index.hpp>
#include <learned_index/search/binary_search.hpp>
#include "benchmark_dataset.hpp"

namespace {

template <typename IndexFactory>
void benchmark_contains(
    benchmark::State& state,
    bool existing,
    IndexFactory make_index,
    benchmark_data::DatasetShape shape = benchmark_data::DatasetShape::random_64,
    bool in_range_missing = false
) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0)), shape
    );
    state.SetLabel(benchmark_data::shape_name(shape));
    const auto index = make_index(dataset);
    const auto queries = existing
        ? benchmark_data::make_existing_queries(dataset, 1000)
        : in_range_missing
            ? benchmark_data::make_in_range_missing_queries(dataset, 1000)
            : benchmark_data::make_missing_queries(dataset, 1000);
    std::size_t query_index = 0;

    for (auto _ : state) {
        const bool found = index.contains(queries[query_index]);
        benchmark::DoNotOptimize(&found);
        if (++query_index == queries.size()) {
            query_index = 0;
        }
    }
}

template <typename IndexFactory>
void benchmark_hot_contains(benchmark::State& state, IndexFactory make_index) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    const auto index = make_index(dataset);
    const auto workload = benchmark_data::make_hot_query_workload(dataset, 1000);
    state.SetLabel("hot_keys=1%,hot_queries=90%");
    std::size_t query_index = 0;

    for (auto _ : state) {
        const bool found = index.contains(workload.queries[query_index]);
        benchmark::DoNotOptimize(&found);
        if (++query_index == workload.queries.size()) {
            query_index = 0;
        }
    }
    state.SetItemsProcessed(state.iterations());
}

benchmark_data::DatasetShape shape_from_argument(benchmark::State& state);

void BM_BinarySearch_ContainsExisting(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    });
}

void BM_BinarySearch_ContainsMissing(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    });
}

void BM_BPlusTree_ContainsExisting(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    });
}

void BM_BPlusTree_ContainsMissing(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    });
}

void BM_LearnedIndex_ContainsExisting(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    });
}

void BM_LearnedIndex_ContainsMissing(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    });
}

void BM_PiecewiseLearnedIndex_ContainsExisting(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    });
}

void BM_PiecewiseLearnedIndex_ContainsMissing(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    });
}

void BM_RmiLearnedIndex_ContainsExisting(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset);
    });
}

void BM_RmiLearnedIndex_ContainsInRangeMissing(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset);
    }, benchmark_data::DatasetShape::quadratic_gaps, true);
}

void BM_PiecewiseLearnedIndex_ContainsMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    }, shape_from_argument(state));
}

void BM_BinarySearch_ContainsHotQueries(benchmark::State& state) {
    benchmark_hot_contains(state, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    });
}

void BM_BPlusTree_ContainsHotQueries(benchmark::State& state) {
    benchmark_hot_contains(state, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    });
}

void BM_LearnedIndex_ContainsHotQueries(benchmark::State& state) {
    benchmark_hot_contains(state, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    });
}

void BM_PiecewiseLearnedIndex_ContainsHotQueries(benchmark::State& state) {
    benchmark_hot_contains(state, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    });
}

void BM_RmiLearnedIndex_ContainsHotQueries(benchmark::State& state) {
    benchmark_hot_contains(state, [](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset);
    });
}

void BM_PiecewiseLearnedIndex_Train(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    for (auto _ : state) {
        const learned_index::PiecewiseLearnedIndex index(dataset);
        benchmark::DoNotOptimize(index.predict_position(dataset[dataset.size() / 2]));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(dataset.size())
    );
}

void BM_RmiLearnedIndex_Train(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    for (auto _ : state) {
        const learned_index::RmiLearnedIndex index(dataset);
        benchmark::DoNotOptimize(index.predict_position(dataset[dataset.size() / 2]));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(dataset.size())
    );
}

void BM_LearnedIndex_Train(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    for (auto _ : state) {
        const learned_index::LearnedIndex index(dataset);
        benchmark::DoNotOptimize(index.predict_position(dataset[dataset.size() / 2]));
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(dataset.size())
    );
}

benchmark_data::DatasetShape shape_from_argument(benchmark::State& state) {
    return static_cast<benchmark_data::DatasetShape>(state.range(1));
}

void BM_BinarySearch_ContainsExistingByShape(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    }, shape_from_argument(state));
}

void BM_BinarySearch_ContainsMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    }, shape_from_argument(state));
}

void BM_BPlusTree_ContainsExistingByShape(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    }, shape_from_argument(state));
}

void BM_BPlusTree_ContainsMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    }, shape_from_argument(state));
}

void BM_LearnedIndex_ContainsExistingByShape(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    }, shape_from_argument(state));
}

void BM_PiecewiseLearnedIndex_ContainsExistingByShape(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    }, shape_from_argument(state));
}

void BM_RmiLearnedIndex_ContainsExistingByShape(benchmark::State& state) {
    benchmark_contains(state, true, [](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset);
    }, shape_from_argument(state));
}

void BM_RmiLearnedIndex_ContainsExistingByLeafSize(benchmark::State& state) {
    const auto leaf_size = static_cast<std::size_t>(state.range(2));
    benchmark_contains(state, true, [leaf_size](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset, leaf_size);
    }, shape_from_argument(state));
}

void BM_LearnedIndex_ContainsMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    }, shape_from_argument(state));
}

void BM_BinarySearch_ContainsInRangeMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BinarySearchIndex(dataset);
    }, shape_from_argument(state), true);
}

void BM_BPlusTree_ContainsInRangeMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::BPlusTree::bulk_load(dataset);
    }, shape_from_argument(state), true);
}

void BM_LearnedIndex_ContainsInRangeMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::LearnedIndex(dataset);
    }, shape_from_argument(state), true);
}

void BM_PiecewiseLearnedIndex_ContainsInRangeMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::PiecewiseLearnedIndex(dataset);
    }, shape_from_argument(state), true);
}

void BM_RmiLearnedIndex_ContainsInRangeMissingByShape(benchmark::State& state) {
    benchmark_contains(state, false, [](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset);
    }, shape_from_argument(state), true);
}

void BM_RmiLearnedIndex_ContainsInRangeMissingByLeafSize(benchmark::State& state) {
    const auto leaf_size = static_cast<std::size_t>(state.range(2));
    benchmark_contains(state, false, [leaf_size](const auto& dataset) {
        return learned_index::RmiLearnedIndex(dataset, leaf_size);
    }, shape_from_argument(state), true);
}

void BM_RmiLearnedIndex_TrainByLeafSize(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0)), shape_from_argument(state)
    );
    const auto leaf_size = static_cast<std::size_t>(state.range(2));
    state.SetLabel(benchmark_data::shape_name(shape_from_argument(state)));
    std::size_t model_storage_bytes = 0;

    for (auto _ : state) {
        const learned_index::RmiLearnedIndex index(dataset, leaf_size);
        model_storage_bytes = index.estimated_model_storage_bytes();
        benchmark::DoNotOptimize(index.predict_position(dataset[dataset.size() / 2]));
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(dataset.size())
    );
    state.counters["model_storage_bytes"] =
        static_cast<double>(model_storage_bytes);
}

void BM_BPlusTree_BulkLoad(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    for (auto _ : state) {
        {
            auto tree = learned_index::BPlusTree::bulk_load(dataset);
            benchmark::DoNotOptimize(tree.size());
            benchmark::ClobberMemory();
            state.PauseTiming();
        }
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(dataset.size())
    );
}

std::vector<learned_index::Key> copy_keys(
    const learned_index::SortedDataset& dataset
) {
    std::vector<learned_index::Key> keys;
    keys.reserve(dataset.size());
    for (std::size_t i = 0; i < dataset.size(); ++i) {
        keys.push_back(dataset[i]);
    }
    return keys;
}

void benchmark_insert_batch(benchmark::State& state, bool use_tree) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    std::vector<learned_index::Key> insert_keys;
    insert_keys.reserve(64);
    std::unordered_set<learned_index::Key> selected_keys;
    std::mt19937_64 generator(24680);
    while (insert_keys.size() < 64) {
        const auto candidate = generator();
        if (!learned_index::find(dataset, candidate).has_value() &&
            selected_keys.insert(candidate).second) {
            insert_keys.push_back(candidate);
        }
    }

    for (auto _ : state) {
        state.PauseTiming();
        {
            std::vector<learned_index::Key> sorted_vector;
            learned_index::BPlusTree tree;
            if (use_tree) {
                tree = learned_index::BPlusTree::bulk_load(dataset);
            } else {
                sorted_vector = copy_keys(dataset);
                sorted_vector.reserve(sorted_vector.size() + insert_keys.size());
            }
            state.ResumeTiming();

            if (use_tree) {
                for (const auto key : insert_keys) {
                    benchmark::DoNotOptimize(tree.insert(key));
                }
                benchmark::DoNotOptimize(tree.size());
            } else {
                for (const auto key : insert_keys) {
                    const auto position =
                        std::lower_bound(sorted_vector.begin(), sorted_vector.end(), key);
                    sorted_vector.insert(position, key);
                }
                benchmark::DoNotOptimize(sorted_vector.data());
            }
            benchmark::ClobberMemory();
            state.PauseTiming();
        }
        state.ResumeTiming();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(insert_keys.size())
    );
    state.counters["batch_size"] = static_cast<double>(insert_keys.size());
}

void BM_BPlusTree_InsertBatch(benchmark::State& state) {
    benchmark_insert_batch(state, true);
}

void BM_SortedVector_InsertBatch(benchmark::State& state) {
    benchmark_insert_batch(state, false);
}

void BM_AdaptiveLearnedIndex_InsertBatch(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    std::vector<learned_index::Key> insert_keys;
    insert_keys.reserve(64);
    std::unordered_set<learned_index::Key> selected_keys;
    std::mt19937_64 generator(24680);
    while (insert_keys.size() < 64) {
        const auto candidate = generator();
        if (!learned_index::find(dataset, candidate).has_value() &&
            selected_keys.insert(candidate).second) {
            insert_keys.push_back(candidate);
        }
    }

    for (auto _ : state) {
        state.PauseTiming();
        learned_index::AdaptiveLearnedIndex index(dataset, insert_keys.size());
        state.ResumeTiming();
        for (const auto key : insert_keys) {
            benchmark::DoNotOptimize(index.insert(key));
        }
        benchmark::DoNotOptimize(index.size());
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(insert_keys.size())
    );
    state.counters["batch_size"] = static_cast<double>(insert_keys.size());
}

using Range = std::pair<learned_index::Key, learned_index::Key>;

std::vector<Range> make_ranges(const learned_index::SortedDataset& dataset) {
    constexpr std::size_t range_width = 128;
    constexpr std::size_t query_count = 128;
    std::vector<Range> ranges;
    if (dataset.size() < range_width) {
        return ranges;
    }

    ranges.reserve(query_count);
    const std::size_t last_start = dataset.size() - range_width;
    for (std::size_t i = 0; i < query_count; ++i) {
        const std::size_t start =
            (last_start * i) / (query_count - 1);
        ranges.emplace_back(dataset[start], dataset[start + range_width - 1]);
    }
    return ranges;
}

void benchmark_tree_ranges(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    const auto tree = learned_index::BPlusTree::bulk_load(dataset);
    const auto ranges = make_ranges(dataset);
    const std::size_t items_per_pass = ranges.size() * 128;
    std::size_t query_index = 0;

    for (auto _ : state) {
        const auto result = tree.range_query(
            ranges[query_index].first,
            ranges[query_index].second
        );
        benchmark::DoNotOptimize(result.data());
        benchmark::DoNotOptimize(result.size());
        benchmark::ClobberMemory();
        if (++query_index == ranges.size()) {
            query_index = 0;
        }
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(items_per_pass)
    );
}

void benchmark_vector_ranges(benchmark::State& state) {
    const auto dataset = benchmark_data::make_dataset(
        static_cast<std::size_t>(state.range(0))
    );
    const auto keys = copy_keys(dataset);
    const auto ranges = make_ranges(dataset);
    const std::size_t items_per_pass = ranges.size() * 128;
    std::size_t query_index = 0;

    for (auto _ : state) {
        const auto first = std::lower_bound(
            keys.begin(), keys.end(), ranges[query_index].first
        );
        const auto last = std::upper_bound(
            first, keys.end(), ranges[query_index].second
        );
        const std::vector<learned_index::Key> result(first, last);
        benchmark::DoNotOptimize(result.data());
        benchmark::DoNotOptimize(result.size());
        benchmark::ClobberMemory();
        if (++query_index == ranges.size()) {
            query_index = 0;
        }
    }
    state.SetItemsProcessed(
        state.iterations() * static_cast<std::int64_t>(items_per_pass)
    );
}

void BM_BPlusTree_RangeQuery(benchmark::State& state) {
    benchmark_tree_ranges(state);
}

void BM_SortedVector_RangeQuery(benchmark::State& state) {
    benchmark_vector_ranges(state);
}

} // namespace

BENCHMARK(BM_BinarySearch_ContainsExisting)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BinarySearch_ContainsMissing)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsExisting)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsMissing)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_ContainsExisting)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_ContainsMissing)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsExisting)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsMissing)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_ContainsExisting)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_ContainsInRangeMissing)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BinarySearch_ContainsHotQueries)
    ->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsHotQueries)
    ->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_ContainsHotQueries)
    ->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsHotQueries)
    ->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_ContainsHotQueries)
    ->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_Train)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_Train)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_Train)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BinarySearch_ContainsExistingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BinarySearch_ContainsMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsExistingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_ContainsExistingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsExistingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_ContainsExistingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
[[maybe_unused]] const bool rmi_leaf_benchmarks_registered = [] {
    for (const auto shape : {0, 1, 2, 3}) {
        for (const auto leaf_size : {256, 512, 1024, 2048}) {
            benchmark::RegisterBenchmark(
                "BM_RmiLearnedIndex_ContainsExistingByLeafSize",
                &BM_RmiLearnedIndex_ContainsExistingByLeafSize
            )->Args({100000, shape, leaf_size})
                ->Repetitions(5)->MinWarmUpTime(0.5);
            benchmark::RegisterBenchmark(
                "BM_RmiLearnedIndex_ContainsInRangeMissingByLeafSize",
                &BM_RmiLearnedIndex_ContainsInRangeMissingByLeafSize
            )->Args({100000, shape, leaf_size})
                ->Repetitions(5)->MinWarmUpTime(0.5);
            benchmark::RegisterBenchmark(
                "BM_RmiLearnedIndex_TrainByLeafSize",
                &BM_RmiLearnedIndex_TrainByLeafSize
            )->Args({100000, shape, leaf_size})
                ->Repetitions(5)->MinWarmUpTime(0.5);
        }
    }
    return true;
}();
BENCHMARK(BM_LearnedIndex_ContainsMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BinarySearch_ContainsInRangeMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_ContainsInRangeMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_LearnedIndex_ContainsInRangeMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_PiecewiseLearnedIndex_ContainsInRangeMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_RmiLearnedIndex_ContainsInRangeMissingByShape)
    ->Args({100000, 0})->Args({100000, 1})->Args({100000, 2})->Args({100000, 3})
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_BulkLoad)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)
    ->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_BPlusTree_InsertBatch)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Iterations(20)
    ->Repetitions(5)->MinWarmUpTime(0.5)->UseRealTime();
BENCHMARK(BM_SortedVector_InsertBatch)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Iterations(20)
    ->Repetitions(5)->MinWarmUpTime(0.5)->UseRealTime();
BENCHMARK(BM_AdaptiveLearnedIndex_InsertBatch)
    ->Arg(1000)->Arg(10000)->Arg(100000)->Iterations(20)
    ->Repetitions(5)->MinWarmUpTime(0.5)->UseRealTime();
BENCHMARK(BM_BPlusTree_RangeQuery)
    ->Arg(10000)->Arg(100000)->Arg(1000000)->Repetitions(5)->MinWarmUpTime(0.5);
BENCHMARK(BM_SortedVector_RangeQuery)
    ->Arg(10000)->Arg(100000)->Arg(1000000)->Repetitions(5)->MinWarmUpTime(0.5);
