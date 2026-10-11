#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/b_plus_tree.hpp>
#include <learned_index/index/index.hpp>

namespace learned_index {

static_assert(Index<BPlusTree>);

TEST(BPlusTreeTest, RejectsFanoutBelowMinimum) {
    EXPECT_THROW(BPlusTree(2), std::invalid_argument);
}

TEST(BPlusTreeTest, EmptyTreeSupportsLookupAndValidation) {
    const BPlusTree tree(3);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.size(), 0);
    EXPECT_FALSE(tree.contains(42));
    EXPECT_TRUE(tree.range_query(0, 100).empty());
    EXPECT_TRUE(tree.validate());
}

TEST(BPlusTreeTest, BulkLoadsEmptySmallAndLargeDatasets) {
    for (std::size_t fanout = 3; fanout <= 8; ++fanout) {
        for (std::size_t count = 0; count <= 160; ++count) {
            std::vector<Key> keys;
            keys.reserve(count);
            for (std::size_t i = 0; i < count; ++i) {
                keys.push_back(static_cast<Key>(i * 3));
            }
            const SortedDataset dataset(std::move(keys));
            const BPlusTree tree = BPlusTree::bulk_load(dataset, fanout);

            ASSERT_EQ(tree.size(), count);
            ASSERT_TRUE(tree.validate()) << "fanout=" << fanout << ", count=" << count;
            for (std::size_t i = 0; i < count; ++i) {
                EXPECT_TRUE(tree.contains(static_cast<Key>(i * 3)));
            }
            EXPECT_FALSE(tree.contains(1));
        }
    }
}

TEST(BPlusTreeTest, InsertReportsNewAndDuplicateKeys) {
    BPlusTree tree(3);

    EXPECT_TRUE(tree.insert(42));
    EXPECT_FALSE(tree.insert(42));
    EXPECT_EQ(tree.size(), 1);
    EXPECT_TRUE(tree.contains(42));
    EXPECT_TRUE(tree.validate());
}

TEST(BPlusTreeTest, HandlesIncreasingAndDecreasingInsertionsAndRepeatedSplits) {
    for (const bool reverse : {false, true}) {
        BPlusTree tree(3);
        for (std::size_t i = 0; i < 500; ++i) {
            const std::size_t key = reverse ? 499 - i : i;
            EXPECT_TRUE(tree.insert(static_cast<Key>(key)));
        }

        EXPECT_EQ(tree.size(), 500);
        EXPECT_TRUE(tree.validate());
        for (std::size_t i = 0; i < 500; ++i) {
            EXPECT_TRUE(tree.contains(static_cast<Key>(i)));
        }
        EXPECT_FALSE(tree.contains(500));
    }
}

TEST(BPlusTreeTest, RandomizedInsertionsMatchOrderedSet) {
    std::vector<Key> keys;
    for (std::uint64_t i = 0; i < 2000; ++i) {
        keys.push_back(i * 17);
    }
    std::mt19937_64 random(20261011);
    std::shuffle(keys.begin(), keys.end(), random);

    BPlusTree tree(5);
    std::set<Key> reference;
    for (const Key key : keys) {
        EXPECT_EQ(tree.insert(key), reference.insert(key).second);
    }
    EXPECT_FALSE(tree.insert(keys.front()));
    EXPECT_EQ(tree.size(), reference.size());
    EXPECT_TRUE(tree.validate());

    for (std::uint64_t i = 0; i < 2200; ++i) {
        const Key key = i * 13;
        EXPECT_EQ(tree.contains(key), reference.contains(key));
    }
}

TEST(BPlusTreeTest, InclusiveRangeQueriesCrossLeafBoundaries) {
    std::vector<Key> keys;
    for (std::uint64_t i = 0; i < 100; ++i) {
        keys.push_back(i * 2);
    }
    const SortedDataset dataset(std::move(keys));
    const BPlusTree tree = BPlusTree::bulk_load(dataset, 3);

    EXPECT_EQ(tree.range_query(14, 30),
              (std::vector<Key>{14, 16, 18, 20, 22, 24, 26, 28, 30}));
    EXPECT_EQ(tree.range_query(15, 29),
              (std::vector<Key>{16, 18, 20, 22, 24, 26, 28}));
    EXPECT_TRUE(tree.range_query(40, 39).empty());
    EXPECT_TRUE(tree.range_query(201, 300).empty());
}

TEST(BPlusTreeTest, HandlesUnsignedKeyBoundaries) {
    BPlusTree tree(3);
    EXPECT_TRUE(tree.insert(std::numeric_limits<Key>::max()));
    EXPECT_TRUE(tree.insert(0));
    EXPECT_TRUE(tree.validate());
    EXPECT_EQ(tree.range_query(0, std::numeric_limits<Key>::max()),
              (std::vector<Key>{0, std::numeric_limits<Key>::max()}));
}

} // namespace learned_index
