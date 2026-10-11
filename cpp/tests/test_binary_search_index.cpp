#include <gtest/gtest.h>
#include <learned_index/index/index.hpp>
#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/binary_search_index.hpp>

namespace learned_index {

static_assert(Index<BinarySearchIndex>);

TEST(BinarySearchIndexTest, FindsExistingKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});
    const BinarySearchIndex index(dataset);
    const auto result = index.find(40);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 2);
}

TEST(BinarySearchIndexTest, ReturnsNulloptForMissingKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});
    const BinarySearchIndex index(dataset);
    const auto result = index.find(41);

    EXPECT_FALSE(result.has_value());
}

TEST(BinarySearchIndexTest, FindsFirstKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});
    const BinarySearchIndex index(dataset);
    const auto result = index.find(10);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0);
}

TEST(BinarySearchIndexTest, FindsLastKey) {
    const SortedDataset dataset({10, 25, 40, 55, 70});
    const BinarySearchIndex index(dataset);
    const auto result = index.find(70);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 4);
}

TEST(BinarySearchIndexTest, ImplementsExactMembershipContract) {
    const SortedDataset dataset({10, 25, 40, 55, 70});
    const BinarySearchIndex index(dataset);

    EXPECT_TRUE(index.contains(10));
    EXPECT_TRUE(index.contains(40));
    EXPECT_FALSE(index.contains(41));
    EXPECT_FALSE(index.contains(5));
}

}
