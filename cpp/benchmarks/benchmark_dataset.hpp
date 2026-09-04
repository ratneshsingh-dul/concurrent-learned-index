#pragma once
#include <cstddef>
#include <vector>
#include <learned_index/data/sorted_dataset.hpp>

namespace benchmark_data{

learned_index::SortedDataset make_dataset(std::size_t size);

std::vector<learned_index::Key> make_existing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

std::vector<learned_index::Key> make_missing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count
);

}