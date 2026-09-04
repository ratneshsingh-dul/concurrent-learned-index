#include "benchmark_dataset.hpp"
#include <learned_index/search/binary_search.hpp>
#include <utility>
#include <vector>
#include <random>
#include <algorithm>

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

}