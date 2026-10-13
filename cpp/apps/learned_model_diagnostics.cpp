#include <algorithm>
#include <charconv>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <learned_index/index/learned_index.hpp>
#include <learned_index/index/piecewise_learned_index.hpp>
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

    std::cout << "dataset_shape,key_count,global_mean_abs_rank_error,global_p50,global_p95,global_max,"
                 "piecewise_mean_abs_rank_error,piecewise_p50,piecewise_p95,piecewise_max\n";
    for (const auto shape : shapes) {
        const auto dataset = benchmark_data::make_dataset(key_count, shape);
        const learned_index::LearnedIndex global_index(dataset);
        const learned_index::PiecewiseLearnedIndex piecewise_index(dataset);
        const auto rank_errors = [&](const auto& index) {
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
            return std::tuple{static_cast<double>(total_error / errors.size()),
                              errors[p50], errors[p95], errors.back()};
        };
        const auto [global_mean, global_p50, global_p95, global_max] = rank_errors(global_index);
        const auto [piecewise_mean, piecewise_p50, piecewise_p95, piecewise_max] = rank_errors(piecewise_index);
        std::cout << benchmark_data::shape_name(shape) << ',' << dataset.size() << ','
                  << std::fixed << std::setprecision(2)
                  << global_mean << ',' << global_p50 << ',' << global_p95 << ',' << global_max << ','
                  << piecewise_mean << ',' << piecewise_p50 << ',' << piecewise_p95 << ',' << piecewise_max << '\n';
    }
    return 0;
}
