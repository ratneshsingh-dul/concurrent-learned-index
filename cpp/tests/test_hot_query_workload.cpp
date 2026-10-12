#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include "../benchmarks/benchmark_dataset.hpp"

namespace {

TEST(HotQueryWorkloadTest, UsesOnePercentHotSetAndIsReproducible) {
    std::vector<learned_index::Key> keys;
    keys.reserve(10000);
    for (std::size_t i = 0; i < 10000; ++i) {
        keys.push_back(static_cast<learned_index::Key>(i * 3));
    }
    const learned_index::SortedDataset dataset(std::move(keys));

    const auto first = benchmark_data::make_hot_query_workload(dataset, 20000);
    const auto second = benchmark_data::make_hot_query_workload(dataset, 20000);

    EXPECT_EQ(first.hot_keys.size(), 100);
    EXPECT_EQ(first.hot_keys, second.hot_keys);
    EXPECT_EQ(first.queries, second.queries);
}

TEST(HotQueryWorkloadTest, DrawsNinetyPercentOfQueriesFromHotSet) {
    std::vector<learned_index::Key> keys;
    keys.reserve(10000);
    for (std::size_t i = 0; i < 10000; ++i) {
        keys.push_back(static_cast<learned_index::Key>(i * 3));
    }
    const learned_index::SortedDataset dataset(std::move(keys));
    const auto workload = benchmark_data::make_hot_query_workload(dataset, 20000);
    const std::unordered_set<learned_index::Key> hot_set(
        workload.hot_keys.begin(), workload.hot_keys.end()
    );

    std::size_t hot_query_count = 0;
    for (const auto key : workload.queries) {
        EXPECT_LT(key, 30000);
        EXPECT_EQ(key % 3, 0);
        hot_query_count += hot_set.contains(key) ? 1 : 0;
    }

    const double hot_query_fraction =
        static_cast<double>(hot_query_count) / workload.queries.size();
    EXPECT_GT(hot_query_fraction, 0.89);
    EXPECT_LT(hot_query_fraction, 0.92);
}

TEST(HotQueryWorkloadTest, HandlesSmallDatasetsAndEmptyQueryStreams) {
    const learned_index::SortedDataset one_key({42});
    const auto one_key_workload =
        benchmark_data::make_hot_query_workload(one_key, 100);
    EXPECT_EQ(one_key_workload.hot_keys, (std::vector<learned_index::Key>{42}));
    EXPECT_EQ(one_key_workload.queries.size(), 100);

    const learned_index::SortedDataset empty(std::vector<learned_index::Key>{});
    EXPECT_NO_THROW(benchmark_data::make_hot_query_workload(empty, 0));
    EXPECT_THROW(
        benchmark_data::make_hot_query_workload(empty, 1),
        std::invalid_argument
    );
}

} // namespace
