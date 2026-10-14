#include <charconv>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <learned_index/diagnostics/rank_error.hpp>
#include <learned_index/index/learned_index.hpp>
#include <learned_index/index/piecewise_learned_index.hpp>
#include "../benchmarks/benchmark_dataset.hpp"

namespace {

template <typename Index>
void print_summary(
    const char* shape_name,
    std::size_t key_count,
    const char* model_name,
    const char* query_kind,
    const learned_index::SortedDataset& dataset,
    const std::vector<learned_index::Key>& queries,
    const Index& index
) {
    const auto summary = learned_index::diagnostics::summarize_rank_error(
        dataset, queries, [&index](learned_index::Key key) {
            return index.predict_position(key);
        }
    );

    std::cout << shape_name << ',' << key_count << ',' << model_name << ','
              << query_kind << ',' << queries.size() << ','
              << std::fixed << std::setprecision(2)
              << summary.mean_signed_residual << ','
              << summary.signed_p05 << ',' << summary.signed_p50 << ','
              << summary.signed_p95 << ',' << summary.mean_absolute_distance << ','
              << summary.absolute_p50 << ',' << summary.absolute_p95 << ','
              << summary.absolute_max << '\n';
}

} // namespace

int main(int argc, char** argv) {
    std::size_t key_count = 100000;
    if (argc > 1) {
        const char* end = argv[1] + std::char_traits<char>::length(argv[1]);
        const auto parsed = std::from_chars(argv[1], end, key_count);
        if (parsed.ec != std::errc{} || parsed.ptr != end) {
            std::cerr << "usage: learned_correction_diagnostics [positive-key-count]\n";
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

    std::cout << "dataset_shape,key_count,model,query_kind,query_count,"
                 "mean_signed_residual,p05_signed_residual,p50_signed_residual,"
                 "p95_signed_residual,mean_absolute_rank_distance,"
                 "p50_absolute_rank_distance,p95_absolute_rank_distance,"
                 "max_absolute_rank_distance\n";

    for (const auto shape : shapes) {
        const auto dataset = benchmark_data::make_dataset(key_count, shape);
        const auto existing = benchmark_data::make_existing_queries(dataset, 1000);
        bool has_in_range_gap = false;
        for (std::size_t i = 0; i + 1 < dataset.size(); ++i) {
            has_in_range_gap = has_in_range_gap || dataset[i + 1] - dataset[i] > 1;
        }
        const auto missing = has_in_range_gap
            ? benchmark_data::make_in_range_missing_queries(dataset, 1000)
            : std::vector<learned_index::Key>{};
        const learned_index::LearnedIndex global_index(dataset);
        const learned_index::PiecewiseLearnedIndex piecewise_index(dataset);
        const char* name = benchmark_data::shape_name(shape);

        print_summary(name, dataset.size(), "global", "existing", dataset,
                      existing, global_index);
        print_summary(name, dataset.size(), "piecewise", "existing", dataset,
                      existing, piecewise_index);
        print_summary(name, dataset.size(), "global", "in_range_missing", dataset,
                      missing, global_index);
        print_summary(name, dataset.size(), "piecewise", "in_range_missing", dataset,
                      missing, piecewise_index);
    }
    return 0;
}
