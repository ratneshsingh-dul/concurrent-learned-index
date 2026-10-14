#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/diagnostics/rank_error.hpp>

namespace learned_index::diagnostics {
namespace {

TEST(RankErrorDiagnosticsTest, SummarizesSignedResidualsAndCorrectionDistances) {
    const SortedDataset dataset({10, 20, 30, 40, 50});
    const std::vector<Key> queries{10, 25, 50};
    const auto summary = summarize_rank_error(
        dataset, queries, [](Key query) {
            if (query == 10) return std::size_t{1};
            if (query == 25) return std::size_t{1};
            return std::size_t{3};
        }
    );

    EXPECT_DOUBLE_EQ(summary.mean_signed_residual, -1.0 / 3.0);
    EXPECT_DOUBLE_EQ(summary.mean_absolute_distance, 1.0);
    EXPECT_EQ(summary.signed_p05, -1);
    EXPECT_EQ(summary.signed_p50, -1);
    EXPECT_EQ(summary.signed_p95, -1);
    EXPECT_EQ(summary.absolute_p50, 1);
    EXPECT_EQ(summary.absolute_p95, 1);
    EXPECT_EQ(summary.absolute_max, 1);
}

TEST(RankErrorDiagnosticsTest, ReturnsZeroSummaryForNoQueries) {
    const SortedDataset dataset({10, 20});
    const std::vector<Key> queries;
    const auto summary = summarize_rank_error(
        dataset, queries, [](Key) { return std::size_t{0}; }
    );

    EXPECT_DOUBLE_EQ(summary.mean_signed_residual, 0.0);
    EXPECT_DOUBLE_EQ(summary.mean_absolute_distance, 0.0);
    EXPECT_EQ(summary.absolute_max, 0);
}

} // namespace
} // namespace learned_index::diagnostics
