#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include <learned_index/data/sorted_dataset.hpp>
#include <learned_index/index/index.hpp>
#include <learned_index/index/learned_index.hpp>

namespace learned_index {

static_assert(Index<LearnedIndex>);

TEST(LearnedIndexTest, HandlesEmptyAndSingleKeyDatasets) {
    const SortedDataset empty({});
    const LearnedIndex empty_index(empty);
    EXPECT_FALSE(empty_index.find(0).has_value());
    EXPECT_EQ(empty_index.predict_position(99), 0);

    const SortedDataset one({42});
    const LearnedIndex one_index(one);
    EXPECT_EQ(one_index.find(42), 0);
    EXPECT_FALSE(one_index.contains(41));
    EXPECT_FALSE(one_index.contains(43));
}

TEST(LearnedIndexTest, FindsPresentAndRejectsMissingKeysExactly) {
    std::vector<Key> keys;
    for (std::uint64_t i = 0; i < 10000; ++i) {
        keys.push_back(1000 + i * 17);
    }
    const SortedDataset dataset(std::move(keys));
    const LearnedIndex index(dataset);

    for (std::size_t i = 0; i < dataset.size(); ++i) {
        ASSERT_EQ(index.find(dataset[i]), i);
    }
    for (std::uint64_t i = 0; i < 10000; ++i) {
        EXPECT_FALSE(index.contains(1001 + i * 17));
    }
}

TEST(LearnedIndexTest, ExactCorrectionSurvivesHighlyUnevenGaps) {
    const SortedDataset dataset({0, 1, 2, 3, 4, 5, 1000000, 9000000000ULL,
                                 9000000001ULL, 9000000002ULL});
    const LearnedIndex index(dataset);
    for (std::size_t i = 0; i < dataset.size(); ++i) {
        EXPECT_EQ(index.find(dataset[i]), i);
    }
    for (const Key missing : {6ULL, 999999ULL, 1000001ULL, 8999999999ULL}) {
        EXPECT_FALSE(index.contains(missing)) << missing;
    }
}

TEST(LearnedIndexTest, RandomQueriesMatchBinarySearchOracle) {
    std::vector<Key> keys;
    keys.reserve(5000);
    std::mt19937_64 generator(20261011);
    while (keys.size() < 5000) {
        keys.push_back(generator());
    }
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    const std::vector<Key> reference = keys;
    const SortedDataset dataset(std::move(keys));
    const LearnedIndex index(dataset);

    for (std::size_t i = 0; i < dataset.size(); ++i) {
        EXPECT_EQ(index.find(dataset[i]), i);
    }
    for (std::size_t i = 0; i < 20000; ++i) {
        const Key query = generator();
        const auto expected = std::lower_bound(reference.begin(), reference.end(), query);
        const auto found = index.find(query);
        if (expected == reference.end() || *expected != query) {
            EXPECT_FALSE(found.has_value());
        } else {
            EXPECT_EQ(found, static_cast<std::size_t>(expected - reference.begin()));
        }
    }
}

TEST(LearnedIndexTest, HandlesUnsignedKeyExtremes) {
    const SortedDataset dataset({0, 1, std::numeric_limits<Key>::max() - 1,
                                 std::numeric_limits<Key>::max()});
    const LearnedIndex index(dataset);
    EXPECT_EQ(index.find(0), 0);
    EXPECT_EQ(index.find(std::numeric_limits<Key>::max()), 3);
    EXPECT_FALSE(index.contains(2));
}

} // namespace learned_index
