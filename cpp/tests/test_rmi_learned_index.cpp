#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/rmi_learned_index.hpp>
#include "../benchmarks/benchmark_dataset.hpp"

namespace learned_index {
namespace {

TEST(RmiLearnedIndexTest, RejectsZeroLeafSizeAndHandlesEmptyAndSingleKeyInputs) {
    const SortedDataset empty(std::vector<Key>{});
    EXPECT_THROW(RmiLearnedIndex(empty, 0), std::invalid_argument);
    const RmiLearnedIndex empty_index(empty);
    EXPECT_FALSE(empty_index.find(10).has_value());
    EXPECT_EQ(empty_index.predict_position(10), 0);

    const SortedDataset one({42});
    const RmiLearnedIndex one_index(one);
    EXPECT_EQ(one_index.find(42), 0);
    EXPECT_FALSE(one_index.contains(41));
    EXPECT_FALSE(one_index.contains(43));
}

TEST(RmiLearnedIndexTest, FindsEveryKeyAcrossAllBenchmarkShapesAndLeafSizes) {
    constexpr benchmark_data::DatasetShape shapes[] = {
        benchmark_data::DatasetShape::random_64,
        benchmark_data::DatasetShape::evenly_spaced,
        benchmark_data::DatasetShape::clustered,
        benchmark_data::DatasetShape::quadratic_gaps
    };
    constexpr std::size_t leaf_sizes[] = {1, 73, 1024, 10000};

    for (const auto shape : shapes) {
        const auto dataset = benchmark_data::make_dataset(2048, shape);
        for (const auto leaf_size : leaf_sizes) {
            const RmiLearnedIndex index(dataset, leaf_size);
            for (std::size_t position = 0; position < dataset.size(); ++position) {
                ASSERT_EQ(index.find(dataset[position]), position)
                    << "shape=" << benchmark_data::shape_name(shape)
                    << " leaf_size=" << leaf_size
                    << " position=" << position;
            }
        }
    }
}

TEST(RmiLearnedIndexTest, RejectsInRangeMissesAndOutOfRangeKeys) {
    const auto dataset = benchmark_data::make_dataset(
        4096, benchmark_data::DatasetShape::quadratic_gaps
    );
    const RmiLearnedIndex index(dataset, 127);
    const auto missing = benchmark_data::make_in_range_missing_queries(dataset, 2000);

    for (const Key key : missing) {
        EXPECT_FALSE(index.contains(key)) << "false positive for " << key;
    }
    EXPECT_FALSE(index.contains(dataset[0] - 1));
    EXPECT_FALSE(index.contains(dataset[dataset.size() - 1] + 1));
}

TEST(RmiLearnedIndexTest, HandlesUnsignedKeyBoundaries) {
    const SortedDataset dataset({0, 1, std::numeric_limits<Key>::max() - 1,
                                 std::numeric_limits<Key>::max()});
    const RmiLearnedIndex index(dataset, 2);
    EXPECT_EQ(index.find(0), 0);
    EXPECT_EQ(index.find(std::numeric_limits<Key>::max()), 3);
    EXPECT_FALSE(index.contains(2));
}

} // namespace
} // namespace learned_index
