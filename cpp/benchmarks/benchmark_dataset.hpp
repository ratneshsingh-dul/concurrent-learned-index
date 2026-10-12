#pragma once
#include <cstddef>
#include <vector>
#include <learned_index/data/sorted_dataset.hpp>

namespace benchmark_data{

struct HotQueryWorkload {
    std::vector<learned_index::Key> hot_keys;
    std::vector<learned_index::Key> queries;
};

enum class DatasetShape {
    random_64,
    evenly_spaced,
    clustered,
    quadratic_gaps
};

learned_index::SortedDataset make_dataset(std::size_t size);
learned_index::SortedDataset make_dataset(std::size_t size, DatasetShape shape);
const char* shape_name(DatasetShape shape) noexcept;

std::vector<learned_index::Key> make_existing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

std::vector<learned_index::Key> make_missing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

std::vector<learned_index::Key> make_in_range_missing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

// Builds a deterministic workload that samples 90% of queries from a random
// 1% hot-key set and the remainder uniformly from the full dataset.
HotQueryWorkload make_hot_query_workload(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

}
