#include "benchmark_dataset.hpp"
#include <learned_index/search/binary_search.hpp>
#include <utility>
#include <vector>
#include <random>
#include <algorithm>
#include <stdexcept>

namespace benchmark_data{

    learned_index::SortedDataset make_dataset(std::size_t size){
        std::vector<learned_index::Key>keys;
        keys.reserve(size);

        std::mt19937_64 generator(12345);

        while(keys.size()<size){
            keys.push_back(generator());
        }

        std::sort(keys.begin(),keys.end());

        keys.erase(
            std::unique(keys.begin(),keys.end()),
            keys.end()
        );

       while (keys.size() < size) {
        keys.push_back(generator());
        std::sort(keys.begin(), keys.end());

        keys.erase(
            std::unique(keys.begin(), keys.end()),
            keys.end()
        );
    }

        return learned_index::SortedDataset(std::move(keys));
    }

    learned_index::SortedDataset make_dataset(std::size_t size, DatasetShape shape) {
        if (shape == DatasetShape::random_64) {
            return make_dataset(size);
        }

        std::vector<learned_index::Key> keys;
        keys.reserve(size);
        for (std::size_t i = 0; i < size; ++i) {
            const auto position = static_cast<learned_index::Key>(i);
            switch (shape) {
            case DatasetShape::random_64:
                break;
            case DatasetShape::evenly_spaced:
                keys.push_back(position * 2);
                break;
            case DatasetShape::clustered:
                keys.push_back(position + (position / 256) * 1000000ULL);
                break;
            case DatasetShape::quadratic_gaps:
                keys.push_back(position * position);
                break;
            }
        }
        return learned_index::SortedDataset(std::move(keys));
    }

    const char* shape_name(DatasetShape shape) noexcept {
        switch (shape) {
        case DatasetShape::random_64: return "random_64";
        case DatasetShape::evenly_spaced: return "evenly_spaced";
        case DatasetShape::clustered: return "clustered";
        case DatasetShape::quadratic_gaps: return "quadratic_gaps";
        }
        return "unknown";
    }

    std::vector<learned_index::Key> make_existing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count){
        std::vector<learned_index::Key>queries;
        queries.reserve(query_count);
        std::mt19937_64 generator(67890);

        std::uniform_int_distribution<std::size_t> distribution(
            0,dataset.size()-1
        );

        for(std::size_t i=0;i<query_count;++i){
            const std::size_t index=distribution(generator);
            queries.push_back(dataset[index]);
        }

        return queries;
    }

    std::vector<learned_index::Key> make_missing_queries(
    const learned_index::SortedDataset& dataset,
    std::size_t query_count){

        std::vector<learned_index::Key>queries;
        queries.reserve(query_count);
        std::mt19937_64 generator(13579);

        while(queries.size()<query_count){
            const auto candidate=generator();
            const auto result=learned_index::find(dataset,candidate);

            if(!result.has_value()){
                queries.push_back(candidate);
            }
        }

        return queries;
    }

    std::vector<learned_index::Key> make_in_range_missing_queries(
        const learned_index::SortedDataset& dataset,
        std::size_t query_count) {
        std::vector<std::size_t> gap_positions;
        for (std::size_t i = 0; i + 1 < dataset.size(); ++i) {
            if (dataset[i + 1] - dataset[i] > 1) {
                gap_positions.push_back(i);
            }
        }
        if (gap_positions.empty() && query_count != 0) {
            throw std::invalid_argument(
                "dataset has no in-range missing uint64 key to query"
            );
        }

        std::vector<learned_index::Key> queries;
        queries.reserve(query_count);
        std::mt19937_64 generator(97531);
        std::uniform_int_distribution<std::size_t> gap_distribution(
            0, gap_positions.empty() ? 0 : gap_positions.size() - 1
        );
        while (queries.size() < query_count) {
            const std::size_t left = gap_positions[gap_distribution(generator)];
            const auto lower = dataset[left];
            const auto gap_size = dataset[left + 1] - lower - 1;
            queries.push_back(lower + 1 + (generator() % gap_size));
        }
        return queries;
    }

}
