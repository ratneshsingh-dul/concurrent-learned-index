#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace learned_index::diagnostics {

// Rank residual is prediction minus the exact lower-bound insertion rank.
// Its absolute value measures correction distance in array positions, not
// the number of probes/comparisons performed by a particular search routine.
struct RankErrorSummary {
    double mean_signed_residual = 0.0;
    double mean_absolute_distance = 0.0;
    std::ptrdiff_t signed_p05 = 0;
    std::ptrdiff_t signed_p50 = 0;
    std::ptrdiff_t signed_p95 = 0;
    std::size_t absolute_p50 = 0;
    std::size_t absolute_p95 = 0;
    std::size_t absolute_max = 0;
};

namespace detail {

template <typename Value>
Value percentile(const std::vector<Value>& sorted, std::size_t percent) {
    const std::size_t index = ((sorted.size() - 1) * percent) / 100;
    return sorted[index];
}

} // namespace detail

template <typename Dataset, typename QueryRange, typename Predictor>
RankErrorSummary summarize_rank_error(
    const Dataset& dataset,
    const QueryRange& queries,
    Predictor predict
) {
    RankErrorSummary summary;
    if (queries.empty()) {
        return summary;
    }

    std::vector<std::ptrdiff_t> signed_residuals;
    std::vector<std::size_t> absolute_distances;
    signed_residuals.reserve(queries.size());
    absolute_distances.reserve(queries.size());
    long double signed_total = 0.0L;
    long double absolute_total = 0.0L;

    for (const auto query : queries) {
        std::size_t first = 0;
        std::size_t last = dataset.size();
        while (first < last) {
            const std::size_t middle = first + (last - first) / 2;
            if (dataset[middle] < query) {
                first = middle + 1;
            } else {
                last = middle;
            }
        }

        const std::size_t prediction = predict(query);
        const std::size_t distance = prediction > first
            ? prediction - first
            : first - prediction;
        const auto signed_residual = prediction >= first
            ? static_cast<std::ptrdiff_t>(prediction - first)
            : -static_cast<std::ptrdiff_t>(first - prediction);

        signed_residuals.push_back(signed_residual);
        absolute_distances.push_back(distance);
        signed_total += static_cast<long double>(signed_residual);
        absolute_total += static_cast<long double>(distance);
    }

    std::sort(signed_residuals.begin(), signed_residuals.end());
    std::sort(absolute_distances.begin(), absolute_distances.end());
    summary.mean_signed_residual = static_cast<double>(
        signed_total / static_cast<long double>(queries.size())
    );
    summary.mean_absolute_distance = static_cast<double>(
        absolute_total / static_cast<long double>(queries.size())
    );
    summary.signed_p05 = detail::percentile(signed_residuals, 5);
    summary.signed_p50 = detail::percentile(signed_residuals, 50);
    summary.signed_p95 = detail::percentile(signed_residuals, 95);
    summary.absolute_p50 = detail::percentile(absolute_distances, 50);
    summary.absolute_p95 = detail::percentile(absolute_distances, 95);
    summary.absolute_max = absolute_distances.back();
    return summary;
}

} // namespace learned_index::diagnostics
