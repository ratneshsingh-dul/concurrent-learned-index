#include <algorithm>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <learned_index/index/learned_index.hpp>
#include "../benchmarks/benchmark_dataset.hpp"

int main(int argc, char** argv) {
    std::size_t key_count = 100000;
    if (argc > 1) {
        const char* end = argv[1] + std::char_traits<char>::length(argv[1]);
        const auto parsed = std::from_chars(argv[1], end, key_count);
        if (parsed.ec != std::errc{} || parsed.ptr != end) {
            std::cerr << "usage: learned_model_diagnostics [positive-key-count]\n";
            return 2;
        }
    }
    if (key_count == 0) {
        std::cerr << "key count must be positive\n";
        return 2;
    }

    constexpr benchmark_data::DatasetShape shapes[] = {
        benchmark_data::DatasetShape::random_64,
        benchmark_data::DatasetShape::evenly_spaced,
        benchmark_data::DatasetShape::clustered,
        benchmark_data::DatasetShape::quadratic_gaps
    };

    std::cout << "dataset_shape,key_count,mean_abs_rank_error,p50,p95,max\n";
    for (const auto shape : shapes) {
        const auto dataset = benchmark_data::make_dataset(key_count, shape);
        const learned_index::LearnedIndex index(dataset);
        std::vector<std::size_t> errors;
        errors.reserve(dataset.size());
        long double total_error = 0.0L;
        for (std::size_t position = 0; position < dataset.size(); ++position) {
            const std::size_t prediction = index.predict_position(dataset[position]);
            const std::size_t error = prediction > position
                ? prediction - position
                : position - prediction;
            errors.push_back(error);
            total_error += static_cast<long double>(error);
        }
        std::sort(errors.begin(), errors.end());
        const std::size_t p50 = ((errors.size() - 1) * 50) / 100;
        const std::size_t p95 = ((errors.size() - 1) * 95) / 100;
        std::cout << benchmark_data::shape_name(shape) << ',' << dataset.size() << ','
                  << std::fixed << std::setprecision(2)
                  << static_cast<double>(total_error / errors.size()) << ','
                  << errors[p50] << ',' << errors[p95] << ',' << errors.back() << '\n';
    }
    return 0;
}
