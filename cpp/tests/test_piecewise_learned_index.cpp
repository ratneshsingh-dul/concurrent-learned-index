#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/learned_index.hpp>
#include <learned_index/index/piecewise_learned_index.hpp>
#include "../benchmarks/benchmark_dataset.hpp"

namespace {

TEST(PiecewiseLearnedIndexTest, RejectsZeroSegmentSizeAndHandlesEmptyInput) {
    const learned_index::SortedDataset empty(std::vector<learned_index::Key>{});
    EXPECT_THROW(learned_index::PiecewiseLearnedIndex(empty, 0), std::invalid_argument);

    const learned_index::PiecewiseLearnedIndex index(empty);
    EXPECT_FALSE(index.contains(1));
    EXPECT_FALSE(index.find(1).has_value());
    EXPECT_EQ(index.predict_position(1), 0);
}

TEST(PiecewiseLearnedIndexTest, FindsEveryKeyAcrossAllBenchmarkShapes) {
    constexpr benchmark_data::DatasetShape shapes[] = {
        benchmark_data::DatasetShape::random_64,
        benchmark_data::DatasetShape::evenly_spaced,
        benchmark_data::DatasetShape::clustered,
        benchmark_data::DatasetShape::quadratic_gaps
    };

    for (const auto shape : shapes) {
        const auto dataset = benchmark_data::make_dataset(2048, shape);
        const learned_index::PiecewiseLearnedIndex index(dataset, 73);
        for (std::size_t position = 0; position < dataset.size(); ++position) {
            const auto found = index.find(dataset[position]);
            ASSERT_TRUE(found.has_value()) << "shape=" << benchmark_data::shape_name(shape)
                                           << " position=" << position;
            EXPECT_EQ(*found, position);
        }
        if (dataset[0] > 0) {
            EXPECT_FALSE(index.find(dataset[0] - 1).has_value());
        }
        EXPECT_FALSE(index.find(dataset[dataset.size() - 1] + 1).has_value());
    }
}

TEST(PiecewiseLearnedIndexTest, FindsInRangeMissesAndRejectsSegmentBoundaryGaps) {
    const auto dataset = benchmark_data::make_dataset(
        4096, benchmark_data::DatasetShape::evenly_spaced
    );
    const learned_index::PiecewiseLearnedIndex index(dataset, 257);

    for (learned_index::Key missing = 1; missing < dataset[dataset.size() - 1]; missing += 2) {
        EXPECT_FALSE(index.contains(missing)) << "missing key=" << missing;
    }
    for (std::size_t position = 0; position < dataset.size(); ++position) {
        EXPECT_EQ(index.find(dataset[position]), position);
    }
}

TEST(PiecewiseLearnedIndexTest, ReducesRankErrorOnQuadraticGapData) {
    const auto dataset = benchmark_data::make_dataset(
        10000, benchmark_data::DatasetShape::quadratic_gaps
    );
    const learned_index::LearnedIndex global_model(dataset);
    const learned_index::PiecewiseLearnedIndex piecewise_model(dataset, 256);

    std::size_t global_max_error = 0;
    std::size_t piecewise_max_error = 0;
    for (std::size_t position = 0; position < dataset.size(); ++position) {
        const auto global_prediction = global_model.predict_position(dataset[position]);
        const auto piecewise_prediction = piecewise_model.predict_position(dataset[position]);
        global_max_error = std::max(
            global_max_error,
            global_prediction > position ? global_prediction - position : position - global_prediction
        );
        piecewise_max_error = std::max(
            piecewise_max_error,
            piecewise_prediction > position
                ? piecewise_prediction - position
                : position - piecewise_prediction
        );
    }

    EXPECT_GT(global_max_error, 0);
    EXPECT_LT(piecewise_max_error, global_max_error);
}

TEST(PiecewiseLearnedIndexTest, HandlesUnsignedKeyBoundaries) {
    const learned_index::SortedDataset dataset({
        0,
        std::numeric_limits<learned_index::Key>::max()
    });
    const learned_index::PiecewiseLearnedIndex index(dataset, 1);

    EXPECT_EQ(index.find(0), 0);
    EXPECT_EQ(index.find(std::numeric_limits<learned_index::Key>::max()), 1);
    EXPECT_FALSE(index.contains(1));
}

} // namespace
