#include <benchmark/benchmark.h>

#include <learned_index/index/binary_search_index.hpp>
#include "benchmark_dataset.hpp"

static void BM_BinarySearch_Existing(benchmark::State& state)
{
   const auto dataset=benchmark_data::make_dataset(state.range(0));
   const learned_index::BinarySearchIndex index(dataset);
   const auto queries=benchmark_data::make_existing_queries(dataset,1000);
   std::size_t query_index=0;


   for(auto _:state){
    const auto result=index.find(queries[query_index]);
    benchmark::DoNotOptimize(&result);

    ++query_index;

    if (query_index == queries.size()) {
        query_index = 0;
    }
   }
}

static void BM_BinarySearch_Missing(benchmark::State& state)
{
    const auto dataset = benchmark_data::make_dataset(state.range(0));
    const learned_index::BinarySearchIndex index(dataset);
    const auto queries = benchmark_data::make_missing_queries(dataset, 1000);
    std::size_t query_index = 0;

    for (auto _ : state) {
        const auto result =index.find(queries[query_index]);
        benchmark::DoNotOptimize(&result);

        ++query_index;

        if (query_index == queries.size()) {
            query_index = 0;
        }
    }
}

BENCHMARK(BM_BinarySearch_Existing)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000)
    ->Arg(10000000)
    ->Repetitions(5)
    ->MinWarmUpTime(0.5);

BENCHMARK(BM_BinarySearch_Missing)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000)
    ->Arg(10000000)
    ->Repetitions(5)
    ->MinWarmUpTime(0.5);

BENCHMARK_MAIN();
